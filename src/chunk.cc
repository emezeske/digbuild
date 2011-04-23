#include <queue>

#include <boost/foreach.hpp>

#include <string.h>

#include "chunk.h"

//////////////////////////////////////////////////////////////////////////////////
// Local definitions:
//////////////////////////////////////////////////////////////////////////////////

namespace {

Scalar get_lighting_attenuation( const Scalar power )
{
    const int MAX_POWER = 32;
    const int GRANULARITY = 10;

    static Scalar lighting_attenuation_table[MAX_POWER * GRANULARITY + 1];
    static bool initialized = false;

    if ( !initialized )
    {
        for ( int i = 0; i <= MAX_POWER * GRANULARITY; ++i )
        {
            lighting_attenuation_table[i] = gmtl::Math::pow( 0.75f, Scalar( i ) / Scalar( GRANULARITY ) );
        }

        initialized = true;
    }

    int index = int( roundf( power * Scalar( GRANULARITY ) ) );
    index = std::max( index, 0 );
    index = std::min( index, MAX_POWER * GRANULARITY );
    return lighting_attenuation_table[index];
}

// This function returns true if the incoming light affected the current light.
bool mix_light( Vector3i& current, const Vector3i& incoming )
{
    bool affected = false;

    for ( int i = 0; i < Vector3i::Size; ++i )
    {
        if ( current[i] < incoming[i] )
        {
            current[i] = incoming[i];
            affected = true;
        }
    }

    return affected;
}

void filter_light( Vector3i& current, const Block& block )
{
    if ( !block.is_color_saturated() )
    {
        const Vector3f& filter_color = block.get_color();

        for ( unsigned i = 0; i < Vector3i::Size; ++i )
        {
            // TODO: The following lookup of filter_color[i] is the main bottleneck
            //       in the whole lighting system.  Why?  Does it cause cache thrashing?
            //
            //       I wonder if all the convenience functions for getting at material
            //       attributes are a bad thing?
            current[i] = static_cast<int>( roundf( filter_color[i] * current[i] ) );
        }

        // TODO: The following is equivalent but much slower:
        //
        // current = vector_cast<int>(
        //     pointwise_round( pointwise_product( filter_color, vector_cast<Scalar>( current ) ) ) );
    }
}

// This function returns true if the light becomes fully attenuated.
bool attenuate_light( Vector3i& light )
{
    bool fully_attenuated = true;

    for ( int i = 0; i < Vector3i::Size; ++i )
    {
        light[i] -= 1;

        if ( light[i] > Block::MIN_LIGHT_COMPONENT_LEVEL )
        {
            fully_attenuated = false;
        }
        else if ( light[i] < Block::MIN_LIGHT_COMPONENT_LEVEL )
        {
            light[i] = Block::MIN_LIGHT_COMPONENT_LEVEL;
        }
    }

    return fully_attenuated;
}

// This function returns true if the incoming light would affect the current light.
bool light_would_be_affected( const Vector3i& current, const Vector3i& incoming )
{
    for ( int i = 0; i < Vector3i::Size; ++i )
    {
        if ( current[i] < incoming[i] )
        {
            return true;
        }
    }

    return false;
}

struct ColorLightStrategy
{
    static Vector3i get_light( const Block& block )
    {
        return block.get_light_level();
    }

    static void set_light( Block& block, const Vector3i& light )
    {
        block.set_light_level( light );
    }
};

struct SunLightStrategy
{
    static Vector3i get_light( const Block& block )
    {
        return block.get_sunlight_level();
    }

    static void set_light( Block& block, const Vector3i& light )
    {
        block.set_sunlight_level( light );
    }
};

struct ExternalNeighborStrategy
{
    static BlockIterator get_block_neighbor( const BlockIterator& block_it, const Vector3i& relation )
    {
        return block_it.chunk_->get_block_neighbor( block_it.index_, relation );
    }
};

struct InternalNeighborStrategy
{
    static BlockIterator get_block_neighbor( const BlockIterator& block_it, const Vector3i& relation )
    {
        const Vector3i neighbor_index = block_it.index_ + relation;
        Block* neighbor = block_it.chunk_->maybe_get_block( neighbor_index );

        if ( neighbor )
        {
            return BlockIterator( block_it.chunk_, neighbor, neighbor_index );
        }
        else return BlockIterator();
    }
};

typedef std::vector<Block*> BlockV;
typedef std::pair<const BlockIterator, const Vector3i> FloodFillBlock;
typedef std::queue<FloodFillBlock> FloodFillQueue;

// The 'queue' and 'blocks_visited' parameters here could be local variables
// (with the queue seed passed in instead).  The reason they are parameters is
// so that if flood_fill_light() is called many times, they will not have to be
// allocated repeatedly.  This gives a significant (and measured) performance gain.
template <typename LightStrategy, typename NeighborStrategy>
void flood_fill_light( const bool skip_source_block, FloodFillQueue& queue, BlockV& blocks_visited )
{
    bool source_block = true;

    while ( !queue.empty() )
    {
        const FloodFillBlock flood_block = queue.front();
        Block& block = *flood_block.first.block_;
        queue.pop();

        if ( !block.is_visited() )
        {
            blocks_visited.push_back( &block );
            block.set_visited( true );

            if ( !skip_source_block || !source_block )
            {
                Vector3i filtered_light_level = flood_block.second;
                filter_light( filtered_light_level, block );

                Vector3i block_light_level = LightStrategy::get_light( block );
                if ( !mix_light( block_light_level, filtered_light_level ) )
                {
                    continue; // The incoming light had no effect on this block.
                }

                LightStrategy::set_light( block, block_light_level );
            }
            else source_block = false;

            Vector3i attenuated_light_level = flood_block.second;
            if ( attenuate_light( attenuated_light_level ) )
            {
                continue; // The light has been attenuated down to zero.
            }

            FOREACH_CARDINAL_RELATION( relation )
            {
                const Vector3i relation_vector = cardinal_relation_vector( relation );
                const BlockIterator neighbor = NeighborStrategy::get_block_neighbor( flood_block.first, relation_vector );

                if ( neighbor.block_ &&
                     !neighbor.block_->is_visited() &&
                     neighbor.block_->is_translucent() )
                {
                    if ( light_would_be_affected(
                             LightStrategy::get_light( *neighbor.block_ ), attenuated_light_level ) )
                    {
                        queue.push( std::make_pair( neighbor, attenuated_light_level ) );
                    }
                }
            }
        }
    }

    BOOST_FOREACH( Block* block, blocks_visited )
    {
        block->set_visited( false );
    }

    blocks_visited.clear();
}

} // anonymous namespace

//////////////////////////////////////////////////////////////////////////////////
// Static constant definitions for Chunk:
//////////////////////////////////////////////////////////////////////////////////

const int
    Chunk::SIZE_X,
    Chunk::SIZE_Y,
    Chunk::SIZE_Z;

const Vector3i Chunk::SIZE( SIZE_X, SIZE_Y, SIZE_Z );

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for Chunk:
//////////////////////////////////////////////////////////////////////////////////

Chunk::Chunk( const Vector3i& position ) :
    position_( position )
{
    memset( neighbors_, 0, sizeof( neighbors_ ) );
    get_neighbor_impl( Vector3i( 0, 0, 0 ) ) = this;
}

void Chunk::reset_lighting()
{
    for ( int x = 0; x < SIZE_X; ++x )
    {
        for ( int z = 0; z < SIZE_Z; ++z )
        {
            const int y_max = SIZE_Y - 1;
            const Vector3i top_block_index( x, y_max, z );
            const Block* block_above = get_block_neighbor( top_block_index, Vector3i( 0, 1, 0 ) ).block_;

            Vector3i sunlight_level = Block::MIN_LIGHT_LEVEL;
            bool sunlight_above = false;
           
            if ( !block_above )
            {
                sunlight_above = true;
                sunlight_level = Block::MAX_LIGHT_LEVEL;
            }
            else if ( block_above->is_sunlight_source() )
            {
                sunlight_above = true;
                sunlight_level = block_above->get_sunlight_level();
            }

            for ( int y = y_max; y >= 0; --y )
            {
                Block& block = get_block( Vector3i( x, y, z ) );
                block.set_light_level( Block::MIN_LIGHT_LEVEL );

                if ( sunlight_above && block.is_translucent() )
                {
                    filter_light( sunlight_level, block );
                    block.set_sunlight_source( true );
                    block.set_sunlight_level( sunlight_level );
                }
                else sunlight_above = false;

                if ( !sunlight_above )
                {
                    block.set_sunlight_source( false );
                    block.set_sunlight_level( Block::MIN_LIGHT_LEVEL );
                }
            }
        }
    }

    unset_nop_sunlight_sources();
}

void Chunk::unset_nop_sunlight_sources()
{
    // Unset sunlight sources that will not contribute any lighting, because they
    // are surrounded by other equivalent sunlight sources.  This saves time later,
    // in apply_lighting_to_xxx(), because these sources will not have to be considered.
    for ( int x = 0; x < SIZE_X; ++x )
    {
        for ( int z = 0; z < SIZE_Z; ++z )
        {
            for ( int y = SIZE_Y - 1; y >= 0; --y )
            {
                const Vector3i index( x, y, z );
                Block& block = get_block( index );

                if ( block.is_sunlight_source() )
                {
                    bool is_surrounded = true;
                    Vector3i attenuated_sunlight_level = block.get_sunlight_level();
                    attenuate_light( attenuated_sunlight_level );

                    FOREACH_CARDINAL_RELATION( relation )
                    {
                        const Block* neighbor =
                            maybe_get_block( index + cardinal_relation_vector( relation ) );

                        if ( neighbor &&
                             ( !neighbor->is_sunlight_source() ||
                               light_would_be_affected(
                                   neighbor->get_sunlight_level(), attenuated_sunlight_level ) ) )
                        {
                            is_surrounded = false;
                            break;
                        }
                    }

                    if ( is_surrounded )
                    {
                        block.set_sunlight_source( false );
                    }
                }
                else break;
            }
        }
    }
}

void Chunk::apply_lighting_to_self()
{
    FloodFillQueue sun_flood_queue;
    FloodFillQueue color_flood_queue;
    BlockV blocks_visited;

    FOREACH_BLOCK( x, y, z )
    {
        const Vector3i index( x, y, z );
        Block& block = get_block( index );
        BlockIterator block_it( this, &block, index );

        if ( block.is_sunlight_source() )
        {
            sun_flood_queue.push( std::make_pair( block_it, block.get_sunlight_level() ) );
            flood_fill_light<SunLightStrategy, InternalNeighborStrategy>( true, sun_flood_queue, blocks_visited );
        }

        if ( block.is_light_source() )
        {
            const Vector3i light_color =
                vector_cast<int>(
                    pointwise_round( Vector3f( block.get_color() * Scalar( Block::MAX_LIGHT_COMPONENT_LEVEL ) ) ) );
            color_flood_queue.push( std::make_pair( block_it, light_color ) );
            flood_fill_light<ColorLightStrategy, InternalNeighborStrategy>( false, color_flood_queue, blocks_visited );
        }
    }
}

void Chunk::apply_lighting_to_neighbors()
{
    FloodFillQueue sun_flood_queue;
    FloodFillQueue color_flood_queue;
    BlockV blocks_visited;

    FOREACH_BLOCK( x, y, z )
    {
        // Throw out any Block that is not on the edge of this Chunk.
        //
        // FIXME: This is super ugly and slow.  This _NEEDS_ to be rewritten with a
        //        loop that only generates the edge coordinates.
        //
        if ( x > 0 && x < SIZE_X - 1 &&
             y > 0 && y < SIZE_Y - 1 &&
             z > 0 && z < SIZE_Z - 1 )
        {
            continue;
        }

        const Vector3i index( x, y, z );
        Block& block = get_block( index );
        BlockIterator block_it( this, &block, index );

        if ( block.get_sunlight_level() != Block::MIN_LIGHT_LEVEL )
        {
            sun_flood_queue.push( std::make_pair( block_it, block.get_sunlight_level() ) );
            flood_fill_light<SunLightStrategy, ExternalNeighborStrategy>( true, sun_flood_queue, blocks_visited );
        }

        if ( block.get_light_level() != Block::MIN_LIGHT_LEVEL )
        {
            color_flood_queue.push( std::make_pair( block_it, block.get_light_level() ) );
            flood_fill_light<ColorLightStrategy, ExternalNeighborStrategy>( true, color_flood_queue, blocks_visited );
        }
    }
}

void Chunk::update_geometry()
{
    external_faces_.clear();

    Chunk* column = get_column_bottom();
    Chunk* neighbor_columns[NUM_CARDINAL_RELATIONS];
    FOREACH_CARDINAL_RELATION( relation )
    {
        neighbor_columns[relation] = 
            column->get_neighbor( cardinal_relation_vector( relation ) );
    }

    FOREACH_BLOCK( x, y, z )
    {
        const Vector3i block_index( x, y, z );
        const Block& block = get_block( block_index );

        if ( block.get_material() != BLOCK_MATERIAL_AIR )
        {
            const Vector3f block_position = vector_cast<Scalar>( Vector3i( position_ + block_index ) );

            FOREACH_CARDINAL_RELATION( relation )
            {
                const Vector3i relation_vector = cardinal_relation_vector( relation );
                const Block* block_neighbor = get_block_neighbor( block_index, relation_vector ).block_;

                bool add_face = false;

                if ( block_neighbor )
                {
                    add_face = ( block_neighbor->is_translucent() &&
                                 block.get_material() != block_neighbor->get_material() );
                }
                else
                {
                    // Don't add faces on the sides of the chunk in which there is not presently a column
                    // of chunks.  Also, don't add faces on the bottom of the column, facing downward.
                    add_face = ( relation == CARDINAL_RELATION_ABOVE ||
                               ( relation != CARDINAL_RELATION_BELOW && neighbor_columns[relation] ) );
                }

                if ( add_face )
                {
                    add_external_face( block_index, block_position, block, relation, relation_vector );
                }
            }
        }
    }
}

void Chunk::add_external_face( const Vector3i& block_index, const Vector3f& block_position, const Block& block, const CardinalRelation relation, const Vector3i& relation_vector )
{
    external_faces_.push_back(
        BlockFace(
            vector_cast<Scalar>( relation_vector ),
            vector_cast<Scalar>( cardinal_relation_vector( cardinal_relation_tangent( relation ) ) ),
            block.get_material()
        )
    );

    Vector3f
        average_lighting,
        average_sunlighting;

    #define V( vertex, x, y, z, nax, nay, naz, nbx, nby, nbz )\
        {\
            calculate_vertex_lighting( block_index, relation_vector, Vector3i( nax, nay, naz ), Vector3i( nbx, nby, nbz ), average_lighting, average_sunlighting );\
            external_faces_.back().vertices_[vertex] =\
                BlockFace::Vertex( block_position + Vector3f( x, y, z ), average_lighting, average_sunlighting );\
        }

    switch ( relation )
    {
        case CARDINAL_RELATION_ABOVE:
            V( 0, 0, 1, 0, -1, 0, 0, 0, 0, -1 );
            V( 1, 1, 1, 0,  1, 0, 0, 0, 0, -1 );
            V( 2, 1, 1, 1,  1, 0, 0, 0, 0,  1 );
            V( 3, 0, 1, 1, -1, 0, 0, 0, 0,  1 );
            break;

        case CARDINAL_RELATION_BELOW:
            V( 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 );
            V( 1, 0, 0, 1, -1, 0, 0, 0, 0,  1 );
            V( 2, 1, 0, 1,  1, 0, 0, 0, 0,  1 );
            V( 3, 1, 0, 0,  1, 0, 0, 0, 0, -1 );
            break;

        case CARDINAL_RELATION_NORTH:
            V( 0, 1, 0, 1,  1, 0, 0, 0, -1, 0 );
            V( 1, 0, 0, 1, -1, 0, 0, 0, -1, 0 );
            V( 2, 0, 1, 1, -1, 0, 0, 0,  1, 0 );
            V( 3, 1, 1, 1,  1, 0, 0, 0,  1, 0 );
            break;

        case CARDINAL_RELATION_SOUTH:
            V( 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 );
            V( 1, 1, 0, 0,  1, 0, 0, 0, -1, 0 );
            V( 2, 1, 1, 0,  1, 0, 0, 0,  1, 0 );
            V( 3, 0, 1, 0, -1, 0, 0, 0,  1, 0 );
            break;

        case CARDINAL_RELATION_EAST:
            V( 0, 1, 0, 0, 0, 0, -1, 0, -1, 0 );
            V( 1, 1, 0, 1, 0, 0,  1, 0, -1, 0 );
            V( 2, 1, 1, 1, 0, 0,  1, 0,  1, 0 );
            V( 3, 1, 1, 0, 0, 0, -1, 0,  1, 0 );
            break;

        case CARDINAL_RELATION_WEST:
            V( 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 );
            V( 1, 0, 1, 0, 0, 0, -1, 0,  1, 0 );
            V( 2, 0, 1, 1, 0, 0,  1, 0,  1, 0 );
            V( 3, 0, 0, 1, 0, 0,  1, 0, -1, 0 );
            break;

        default:
            throw std::runtime_error( "Invalid cardinal relation." );
    }
    #undef V
}

void Chunk::calculate_vertex_lighting(
    const Vector3i& primary_index,
    const Vector3i& primary_relation,
    const Vector3i& neighbor_relation_a,
    const Vector3i& neighbor_relation_b,
    Vector3f& vertex_lighting,
    Vector3f& vertex_sunlighting
)
{
    const int NUM_NEIGHBORS = 4;
    BlockIterator neighbors[NUM_NEIGHBORS];
    neighbors[0] = get_block_neighbor( primary_index, primary_relation );
    neighbors[1] = get_block_neighbor( primary_index, primary_relation + neighbor_relation_a );
    neighbors[2] = get_block_neighbor( primary_index, primary_relation + neighbor_relation_b );

    // The 'ab' neighbor cannot contribute light to the vertex if both neighbors 'a' and 'b'
    // are opaque, because they would fully block any light from 'ab'.
    bool neighbor_ab_contributes = false;

    if ( !neighbors[1].block_ || neighbors[1].block_->is_translucent() ||
         !neighbors[2].block_ || neighbors[2].block_->is_translucent() )
    {
        neighbor_ab_contributes = true;
        neighbors[3] = get_block_neighbor( primary_index, primary_relation + neighbor_relation_a + neighbor_relation_b );
    }

    // The lighting value for this vertex will be an average of the lighting provided by
    // all the translucent blocks that may contribute to it.  This gives a smooth lighting
    // effect, instead of the blocky lighting that per-face unaveraged lighting gives.
    Vector3i total_lighting = Block::MIN_LIGHT_LEVEL;
    Vector3i total_sunlighting = Block::MIN_LIGHT_LEVEL;

    // The number of translucent blocks that may contribute light to the vertex
    // is used to determine an ambient occlusion factor.  Less contributors means
    // more ambient occlusion.
    int num_contributors = 0;

    for ( int i = 0; i < NUM_NEIGHBORS; ++i )
    {
        const Block* block = neighbors[i].block_;

        if ( block )
        {
            if ( block->is_translucent() )
            {
                total_lighting += block->get_light_level();
                total_sunlighting += block->get_sunlight_level();
                ++num_contributors;
            }
        }
        else if ( i != 3 || neighbor_ab_contributes )
        {
            total_sunlighting += Block::MAX_LIGHT_LEVEL;
            ++num_contributors;
        }
    }

    const Vector3f average_lighting = vector_cast<Scalar>( total_lighting ) / Scalar( num_contributors );
    const Vector3f average_sunlighting = vector_cast<Scalar>( total_sunlighting ) / Scalar( num_contributors );
    const int ambient_occlusion_power = NUM_NEIGHBORS - neighbor_ab_contributes - num_contributors;
    const int power_base = Block::MAX_LIGHT_COMPONENT_LEVEL + 2 * ambient_occlusion_power;
    
    for ( int i = 0; i < Vector3i::Size; ++i )
    {
        // Don't bother computing the attenuation if the lighting is nearly zero.
        if ( average_lighting[i] > gmtl::GMTL_EPSILON )
        {
            const Scalar power = power_base - average_lighting[i];
            vertex_lighting[i] = get_lighting_attenuation( power );
        }
        else vertex_lighting[i] = 0.0f;

        // TODO: Remove duplication.
        if ( average_sunlighting[i] > gmtl::GMTL_EPSILON )
        {
            const Scalar power = power_base - average_sunlighting[i];
            vertex_sunlighting[i] = get_lighting_attenuation( power );
        }
        else vertex_sunlighting[i] = 0.0f;
    }
}

//////////////////////////////////////////////////////////////////////////////////
// Free function definitions:
//////////////////////////////////////////////////////////////////////////////////

void chunk_stitch_into_map( ChunkSP chunk, ChunkMap& chunks )
{
    FOREACH_SURROUNDING( x, y, z )
    {
        const Vector3i relation( x, y, z );

        ChunkMap::iterator neighbor_it =
            chunks.find( chunk->get_position() + pointwise_product( relation, Chunk::SIZE ) );

        if ( neighbor_it != chunks.end() )
        {
            chunk->set_neighbor( relation, neighbor_it->second.get() );
        }
    }

    chunks[chunk->get_position()] = chunk;
}

void chunk_unstich_from_map( ChunkSP chunk, ChunkMap& chunks )
{
    FOREACH_SURROUNDING( x, y, z )
    {
        const Vector3i relation( x, y, z );
        chunk->set_neighbor( relation, 0 );
    }

    chunks.erase( chunk->get_position() );
}
