#include <queue>

#include <string.h>

#include "chunk.h"

#define FOREACH_BLOCK( x_name, y_name, z_name )\
    for ( int x_name = 0; x_name < Chunk::SIZE_X; ++x_name )\
        for ( int y_name = 0; y_name < Chunk::SIZE_Y; ++y_name )\
            for ( int z_name = 0; z_name < Chunk::SIZE_Z; ++z_name )

#define FOREACH_RELATION( x_name, y_name, z_name )\
    for ( int x_name = -1; x_name < 2; ++x_name )\
        for ( int y_name = -1; y_name < 2; ++y_name )\
            for ( int z_name = -1; z_name < 2; ++z_name )

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
bool mix_light_component( int& current, const int& incoming )
{
    if ( current < incoming )
    {
        current = incoming;
        return false;
    }

    return true;
}

// This function returns true if the light becomes fully attenuated.
bool attenuate_light_component( int& light )
{
    light -= 1;

    if ( light < Block::MIN_LIGHT_COMPONENT_LEVEL )
    {
        light = Block::MIN_LIGHT_COMPONENT_LEVEL;
    }
    else if ( light > Block::MIN_LIGHT_COMPONENT_LEVEL )
    {
        return false;
    }

    return true;
}

struct ColorLightStrategy
{
    typedef Vector3i LightLevel;
    typedef std::pair<const BlockIterator, const Vector3i> FloodFillBlock;
    typedef std::queue<FloodFillBlock> FloodFillQueue;

    static LightLevel get_light( const Block& block )
    {
        return block.get_light_level();
    }

    static void set_light( Block& block, const LightLevel& light )
    {
        block.set_light_level( light );
    }

    static bool mix_light( LightLevel& current, const LightLevel& incoming )
    {
        bool already_lit = true;

        for ( int i = 0; i < LightLevel::Size; ++i )
        {
            if ( !mix_light_component( current[i], incoming[i] ) )
            {
                already_lit = false;
            }
        }

        return already_lit;
    }

    static bool attenuate_light( LightLevel& light )
    {
        bool fully_attenuated = true;

        for ( int i = 0; i < LightLevel::Size; ++i )
        {
            if ( !attenuate_light_component( light[i] ) )
            {
                fully_attenuated = false;
            }
        }

        return fully_attenuated;
    }

    static bool neighbor_needs_visit( const Block& neighbor )
    {
        return !neighbor.is_visited() &&
               neighbor.get_material_attributes().translucent_;
    }
};

struct SunLightStrategy
{
    typedef int LightLevel;
    typedef std::pair<const BlockIterator, const int> FloodFillBlock;
    typedef std::queue<FloodFillBlock> FloodFillQueue;

    static LightLevel get_light( const Block& block )
    {
        return block.get_sunlight_level();
    }

    static void set_light( Block& block, const LightLevel light )
    {
        block.set_sunlight_level( light );
    }

    static bool mix_light( LightLevel& current, const LightLevel incoming )
    {
        return mix_light_component( current, incoming );
    }

    static bool attenuate_light( LightLevel& light )
    {
        return attenuate_light_component( light );
    }

    static bool neighbor_needs_visit( const Block& neighbor )
    {
        return !neighbor.is_sunlight_source() &&
               !neighbor.is_visited() &&
               neighbor.get_material_attributes().translucent_;
    }
};

struct TraverseNeighborStrategy
{
    static BlockIterator get_block_neighbor( const BlockIterator block_it, const Vector3i& relation )
    {
        return block_it.chunk_->get_block_neighbor( block_it.index_, relation );
    }
};

struct LocalNeighborStrategy
{
    static BlockIterator get_block_neighbor( const BlockIterator block_it, const Vector3i& relation )
    {
        const Vector3i neighbor_index = block_it.index_ + relation;

        for ( int i = 0; i < 3; ++i )
        {
            if ( neighbor_index[i] == -1 )
            {
                return BlockIterator();
            }
            else if ( neighbor_index[i] == Chunk::SIZE[i] )
            {
                return BlockIterator();
            }
        }

        return BlockIterator( block_it.chunk_, &block_it.chunk_->get_block( neighbor_index ), neighbor_index );
    }
};

typedef std::vector<Block*> BlockV;

template <typename LightStrategy, typename NeighborStrategy>
void flood_fill_light( typename LightStrategy::FloodFillQueue& queue, BlockV& blocks_visited )
{
    while ( !queue.empty() )
    {
        const typename LightStrategy::FloodFillBlock flood_block = queue.front();
        Block& block = *flood_block.first.block_;
        queue.pop();

        if ( !block.is_visited() )
        {
            blocks_visited.push_back( &block );
            block.set_visited( true );

            typename LightStrategy::LightLevel block_light_level = LightStrategy::get_light( block );
            if ( LightStrategy::mix_light( block_light_level, flood_block.second ) )
                continue;

            LightStrategy::set_light( block, block_light_level );

            FOR_EACH_CARDINAL_RELATION( relation )
            {
                const Vector3i relation_vector = cardinal_relation_vector( relation );
                const BlockIterator neighbor = NeighborStrategy::get_block_neighbor( flood_block.first, relation_vector );
                typename LightStrategy::LightLevel attenuated_light_level;
                bool attenuation_calculated = false;

                if ( neighbor.block_ && LightStrategy::neighbor_needs_visit( *neighbor.block_ ) )
                {
                    if ( !attenuation_calculated )
                    {
                        attenuated_light_level = flood_block.second;
                        if ( LightStrategy::attenuate_light( attenuated_light_level ) )
                            break;

                        attenuation_calculated = true;
                    }

                    queue.push( std::make_pair( neighbor, attenuated_light_level ) );
                }
            }
        }
    }

    for ( BlockV::iterator it = blocks_visited.begin(); it != blocks_visited.end(); ++it )
    {
        ( *it )->set_visited( false );
    }

    blocks_visited.clear();
}

} // anonymous namespace

//////////////////////////////////////////////////////////////////////////////////
// Static constant definitions for Chunk:
//////////////////////////////////////////////////////////////////////////////////

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
            bool above_ground = !block_above || block_above->is_sunlight_source();

            for ( int y = y_max; y >= 0; --y )
            {
                Block& block = get_block( Vector3i( x, y, z ) );
                block.set_light_level( Block::MIN_LIGHT_LEVEL );
                block.set_sunlight_level( Block::MIN_LIGHT_COMPONENT_LEVEL );

                if ( above_ground )
                {
                    if ( block.get_material_attributes().translucent_ )
                    {
                        block.set_sunlight_source( true );
                    }
                    else
                    {
                        above_ground = false;
                        block.set_sunlight_source( false );
                    }
                }
                else block.set_sunlight_source( false );
            }
        }
    }
}

void Chunk::update_geometry()
{
    external_faces_.clear();

    Chunk* column = get_column_bottom();
    Chunk* neighbor_columns[NUM_CARDINAL_RELATIONS];
    FOR_EACH_CARDINAL_RELATION( relation )
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

            FOR_EACH_CARDINAL_RELATION( relation )
            {
                const Vector3i relation_vector = cardinal_relation_vector( relation );
                const Block* block_neighbor = get_block_neighbor( block_index, relation_vector ).block_;

                bool add_face = false;

                if ( block_neighbor )
                {
                    // FIXME: Neighboring tranclucent materials currently result in z-fighting.
                    add_face = ( block_neighbor->get_material_attributes().translucent_ &&
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

    #define V( x, y, z, nax, nay, naz, nbx, nby, nbz )\
        BlockFace::Vertex( block_position + Vector3f( x, y, z ),\
        calculate_vertex_lighting( block_index, relation_vector, Vector3i( nax, nay, naz ), Vector3i( nbx, nby, nbz ) ) )

    switch ( relation )
    {
        case CARDINAL_RELATION_ABOVE:
            external_faces_.back().vertices_[0] = V( 0, 1, 0, -1, 0, 0, 0, 0, -1 );
            external_faces_.back().vertices_[1] = V( 1, 1, 0,  1, 0, 0, 0, 0, -1 );
            external_faces_.back().vertices_[2] = V( 1, 1, 1,  1, 0, 0, 0, 0,  1 );
            external_faces_.back().vertices_[3] = V( 0, 1, 1, -1, 0, 0, 0, 0,  1 );
            break;

        case CARDINAL_RELATION_BELOW:
            external_faces_.back().vertices_[0] = V( 0, 0, 0, -1, 0, 0, 0, 0, -1 );
            external_faces_.back().vertices_[1] = V( 0, 0, 1, -1, 0, 0, 0, 0,  1 );
            external_faces_.back().vertices_[2] = V( 1, 0, 1,  1, 0, 0, 0, 0,  1 );
            external_faces_.back().vertices_[3] = V( 1, 0, 0,  1, 0, 0, 0, 0, -1 );
            break;

        case CARDINAL_RELATION_NORTH:
            external_faces_.back().vertices_[0] = V( 1, 0, 1,  1, 0, 0, 0, -1, 0 );
            external_faces_.back().vertices_[1] = V( 0, 0, 1, -1, 0, 0, 0, -1, 0 );
            external_faces_.back().vertices_[2] = V( 0, 1, 1, -1, 0, 0, 0,  1, 0 );
            external_faces_.back().vertices_[3] = V( 1, 1, 1,  1, 0, 0, 0,  1, 0 );
            break;

        case CARDINAL_RELATION_SOUTH:
            external_faces_.back().vertices_[0] = V( 0, 0, 0, -1, 0, 0, 0, -1, 0 );
            external_faces_.back().vertices_[1] = V( 1, 0, 0,  1, 0, 0, 0, -1, 0 );
            external_faces_.back().vertices_[2] = V( 1, 1, 0,  1, 0, 0, 0,  1, 0 );
            external_faces_.back().vertices_[3] = V( 0, 1, 0, -1, 0, 0, 0,  1, 0 );
            break;

        case CARDINAL_RELATION_EAST:
            external_faces_.back().vertices_[0] = V( 1, 0, 0, 0, 0, -1, 0, -1, 0 );
            external_faces_.back().vertices_[1] = V( 1, 0, 1, 0, 0,  1, 0, -1, 0 );
            external_faces_.back().vertices_[2] = V( 1, 1, 1, 0, 0,  1, 0,  1, 0 );
            external_faces_.back().vertices_[3] = V( 1, 1, 0, 0, 0, -1, 0,  1, 0 );
            break;

        case CARDINAL_RELATION_WEST:
            external_faces_.back().vertices_[0] = V( 0, 0, 0, 0, 0, -1, 0, -1, 0 );
            external_faces_.back().vertices_[1] = V( 0, 1, 0, 0, 0, -1, 0,  1, 0 );
            external_faces_.back().vertices_[2] = V( 0, 1, 1, 0, 0,  1, 0,  1, 0 );
            external_faces_.back().vertices_[3] = V( 0, 0, 1, 0, 0,  1, 0, -1, 0 );
            break;

        default:
            throw std::runtime_error( "Invalid cardinal relation." );
    }
    #undef V
}

Vector4f Chunk::calculate_vertex_lighting(
    const Vector3i& primary_index,
    const Vector3i& primary_relation,
    const Vector3i& neighbor_relation_a,
    const Vector3i& neighbor_relation_b
)
{
    const size_t NUM_NEIGHBORS = 4;
    BlockIterator neighbors[NUM_NEIGHBORS];
    neighbors[0] = get_block_neighbor( primary_index, primary_relation );
    neighbors[1] = get_block_neighbor( primary_index, primary_relation + neighbor_relation_a );
    neighbors[2] = get_block_neighbor( primary_index, primary_relation + neighbor_relation_b );

    bool neighbor_ab_may_contribute = false;

    if ( !neighbors[1].block_ || neighbors[1].block_->get_material_attributes().translucent_ ||
         !neighbors[2].block_ || neighbors[2].block_->get_material_attributes().translucent_ )
    {
        neighbor_ab_may_contribute = true;
        neighbors[3] = get_block_neighbor( primary_index, primary_relation + neighbor_relation_a + neighbor_relation_b );
    }

    Vector3i total_lighting = Block::MIN_LIGHT_LEVEL;
    int total_sunlighting = Block::MIN_LIGHT_COMPONENT_LEVEL;
    int num_contributors = 0;

    for ( size_t i = 0; i < NUM_NEIGHBORS; ++i )
    {
        const Block* block = neighbors[i].block_;

        if ( block )
        {
            if ( block->get_material_attributes().translucent_ )
            {
                total_lighting += block->get_light_level();
                total_sunlighting += block->get_sunlight_level();
                ++num_contributors;
            }
        }
        else if ( i != 3 || neighbor_ab_may_contribute )
        {
            total_sunlighting += Block::MAX_LIGHT_COMPONENT_LEVEL;
            ++num_contributors;
        }
    }

    Vector4f average_lighting( total_lighting[0], total_lighting[1], total_lighting[2], total_sunlighting ); 
    average_lighting /= Scalar( num_contributors );
    const int ambient_occlusion_power = NUM_NEIGHBORS - neighbor_ab_may_contribute - num_contributors;
    Vector4f attenuated_lighting;
    
    for ( int i = 0; i < Vector4i::Size; ++i )
    {
        if ( average_lighting[i] > gmtl::GMTL_EPSILON )
        {
            Scalar power = Block::MAX_LIGHT_COMPONENT_LEVEL - average_lighting[i] + ambient_occlusion_power * 2;
            attenuated_lighting[i] = get_lighting_attenuation( power );
        }
        else attenuated_lighting[i] = 0.0f;
    }

    return attenuated_lighting;
}

//////////////////////////////////////////////////////////////////////////////////
// Free function definitions:
//////////////////////////////////////////////////////////////////////////////////

void chunk_stitch_into_map( ChunkSP chunk, ChunkMap& chunks )
{
    FOREACH_RELATION( x, y, z )
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
    FOREACH_RELATION( x, y, z )
    {
        const Vector3i relation( x, y, z );
        chunk->set_neighbor( relation, 0 );
    }

    chunks.erase( chunk->get_position() );
}

// If the 'local' parameter is set to 'true', lights will only be flood filled within
// the selected Chunk.  If it is 'false', lights will flood across to other Chunks.
void chunk_apply_lighting( Chunk& chunk, const bool local )
{
    SunLightStrategy::FloodFillQueue sun_flood_queue;
    ColorLightStrategy::FloodFillQueue color_flood_queue;
    BlockV blocks_visited;

    FOREACH_BLOCK( x, y, z )
    {
        const Vector3i index( x, y, z );
        Block& block = chunk.get_block( index );
        BlockIterator block_it( &chunk, &block, index );

        if ( block.is_sunlight_source() )
        {
            sun_flood_queue.push( std::make_pair( block_it, Block::MAX_LIGHT_COMPONENT_LEVEL ) );

            if ( local )
            {
                flood_fill_light<SunLightStrategy, LocalNeighborStrategy>( sun_flood_queue, blocks_visited );
            }
            else flood_fill_light<SunLightStrategy, TraverseNeighborStrategy>( sun_flood_queue, blocks_visited );
        }
        else if ( block.get_material() == BLOCK_MATERIAL_MAGMA )
        {
            // TODO: use a material attribute

            color_flood_queue.push( std::make_pair( block_it, Vector3i( 14, 4, 0 ) ) );

            if ( local )
            {
                flood_fill_light<ColorLightStrategy, LocalNeighborStrategy>( color_flood_queue, blocks_visited );
            }
            else flood_fill_light<ColorLightStrategy, TraverseNeighborStrategy>( color_flood_queue, blocks_visited );
        }
    }
}
