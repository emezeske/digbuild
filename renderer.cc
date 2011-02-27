#define GL_GLEXT_PROTOTYPES

#include <GL/gl.h>

#include <boost/lexical_cast.hpp>

#include <stdexcept>
#include <assert.h>

#include "renderer.h"

//////////////////////////////////////////////////////////////////////////////////
// Local definitions:
//////////////////////////////////////////////////////////////////////////////////

namespace {

// TODO: Just using simple colors for materials right now, for testing.  Add textures,
//       bump maps, specular maps, magic, etc.
struct MaterialInfo
{
    GLfloat r_, g_, b_;
};

// FIXME: This is pretty nasty -- the elements of this lookup array have to be in the same order
//        that the BlockMaterial enumerations are defined.  But it's fast, and lookups into this
//        table are a per-block operation...
//
//        Could probably get the same performance with a constant std::vector, and correct
//        BlockMaterial mappings could be ensured.  Do that.
MaterialInfo MATERIAL_INFO[] =
{
    { 0.00f, 0.60f, 0.30f }, // BLOCK_MATERIAL_GRASS
    { 0.40f, 0.20f, 0.00f }, // BLOCK_MATERIAL_DIRT
    { 0.84f, 0.42f, 0.00f }, // BLOCK_MATERIAL_CLAY
    { 0.64f, 0.64f, 0.64f }, // BLOCK_MATERIAL_STONE
    { 0.34f, 0.34f, 0.34f }  // BLOCK_MATERIAL_BEDROCK
};

const Vector3f
    NORTH_NORMAL(  0,  0,  1 ),
    SOUTH_NORMAL(  0,  0, -1 ),
    EAST_NORMAL (  1,  0,  0 ),
    WEST_NORMAL ( -1,  0,  0 ),
    UP_NORMAL   (  0,  1,  0 ),
    DOWN_NORMAL (  0, -1,  0 );

enum FaceDirection
{
    FACE_DIRECTION_NORTH,
    FACE_DIRECTION_SOUTH,
    FACE_DIRECTION_EAST,
    FACE_DIRECTION_WEST,
    FACE_DIRECTION_UP,
    FACE_DIRECTION_DOWN
};

struct BlockVertex
{
    BlockVertex()
    {
    }

    void set( const Vector3f position, const Vector3f normal, const MaterialInfo& material_info )
    {
        x_ = position[0]; y_ = position[1]; z_ = position[2];
        nx_ = normal[0]; ny_ = normal[1]; nz_ = normal[2];
        r_ = material_info.r_; g_ = material_info.g_; b_ = material_info.b_;
    }

    GLfloat x_, y_, z_;
    GLfloat nx_, ny_, nz_;
    GLfloat r_, g_, b_;
} __attribute__((packed));

typedef std::vector<BlockVertex> BlockVertexV;

const Region* find_region( const RegionMap& regions, const Vector2i position )
{
    RegionMap::const_iterator it = regions.find( position );
    return ( it == regions.end() ? 0 : it->second.get() );
}

const Chunk* find_chunk( const Region& region, const Vector3i& index )
{
    ChunkMap::const_iterator it = region.chunks().find( index );
    return ( it == region.chunks().end() ? 0 : it->second.get() );
}

const Chunk* find_adjacent_chunk( const Region& region, const Region* neighbor_region, const Vector3i& neighbor_index )
{
    if ( neighbor_index[0] >= 0 &&
         neighbor_index[2] >= 0 &&
         neighbor_index[0] < Region::CHUNKS_PER_EDGE &&
         neighbor_index[2] < Region::CHUNKS_PER_EDGE )
    {
        return find_chunk( region, neighbor_index );
    }
    else if ( neighbor_region )
    {
        return find_chunk( *neighbor_region, Vector3i(
            ( Region::CHUNKS_PER_EDGE + neighbor_index[0] ) % Region::CHUNKS_PER_EDGE,
            neighbor_index[1],
            ( Region::CHUNKS_PER_EDGE + neighbor_index[2] ) % Region::CHUNKS_PER_EDGE
        ) );
    }
    
    return 0;
}

const Block* find_colinear_column( const Chunk* neighbor_chunk, const Vector2i& column_index )
{
    if ( neighbor_chunk )
    {
        return neighbor_chunk->get_column( column_index );
    }

    return 0;
}

const Block* find_adjacent_column( const Chunk& chunk, const Chunk* neighbor_chunk, const Vector2i& neighbor_index )
{
    if ( neighbor_index[0] >= 0 &&
         neighbor_index[1] >= 0 &&
         neighbor_index[0] < Chunk::CHUNK_SIZE &&
         neighbor_index[1] < Chunk::CHUNK_SIZE )
    {
        return chunk.get_column( neighbor_index );
    }
    else if ( neighbor_chunk )
    {
        return neighbor_chunk->get_column( Vector2i(
            ( Chunk::CHUNK_SIZE + neighbor_index[0] ) % Chunk::CHUNK_SIZE,
            ( Chunk::CHUNK_SIZE + neighbor_index[1] ) % Chunk::CHUNK_SIZE 
        ) );
    }
    
    return 0;
}

void add_block_face(
    const Vector3i column_world_position,
    Block::HeightT block_bottom, 
    Block::HeightT block_top, 
    const BlockMaterial material,
    const FaceDirection direction,
    BlockVertexV& vertices
)
{
    const Vector3f block_world_position = vector_cast<Vector3f>( column_world_position ) + Vector3f( 0.0f, Scalar( block_bottom ), 0.0f );
    const Scalar height = Scalar( block_top - block_bottom );

    assert( material >= 0 && material < int( sizeof( MATERIAL_INFO ) / sizeof( MaterialInfo ) ) );
    const MaterialInfo& material_info = MATERIAL_INFO[material];

    const int VERTICES_PER_FACE = 4;
    vertices.resize( vertices.size() + VERTICES_PER_FACE );
    BlockVertexV::iterator vertex_it = vertices.end() - VERTICES_PER_FACE;

    #define V( x, y, z, n ) do { ( vertex_it++ )->set( block_world_position + Vector3f( x, y, z ), n, material_info ); } while ( false )
    switch ( direction )
    {
        case FACE_DIRECTION_NORTH:
            V( 1, 0,      1, NORTH_NORMAL );
            V( 1, height, 1, NORTH_NORMAL );
            V( 0, height, 1, NORTH_NORMAL );
            V( 0, 0,      1, NORTH_NORMAL );
            break;

        case FACE_DIRECTION_SOUTH:
            V( 0, height, 0, SOUTH_NORMAL );
            V( 1, height, 0, SOUTH_NORMAL );
            V( 1, 0,      0, SOUTH_NORMAL );
            V( 0, 0,      0, SOUTH_NORMAL );
            break;

        case FACE_DIRECTION_EAST:
            V( 1, height, 0, EAST_NORMAL);
            V( 1, height, 1, EAST_NORMAL);
            V( 1, 0,      1, EAST_NORMAL);
            V( 1, 0,      0, EAST_NORMAL);
            break;

        case FACE_DIRECTION_WEST:
            V( 0, 0,      1, WEST_NORMAL );
            V( 0, height, 1, WEST_NORMAL );
            V( 0, height, 0, WEST_NORMAL );
            V( 0, 0,      0, WEST_NORMAL );
            break;

        case FACE_DIRECTION_UP:
            V( 0, 0, 1, UP_NORMAL );
            V( 1, 0, 1, UP_NORMAL );
            V( 1, 0, 0, UP_NORMAL );
            V( 0, 0, 0, UP_NORMAL );
            break;

        case FACE_DIRECTION_DOWN:
            V( 1, 0, 0, DOWN_NORMAL );
            V( 1, 0, 1, DOWN_NORMAL );
            V( 0, 0, 1, DOWN_NORMAL );
            V( 0, 0, 0, DOWN_NORMAL );
            break;

        default:
            throw std::runtime_error( "Invalid FaceDirection: " + boost::lexical_cast<std::string>( direction ) );
    }
    #undef V
}

void add_block_faces(
    const Vector3i column_world_position,
    const Block& block,
    const Block* adjacent_column,
    const FaceDirection direction,
    BlockVertexV& vertices
)
{
    Block::HeightT sliding_bottom = block.bottom_;

    while ( adjacent_column )
    {
        if ( adjacent_column->bottom_ < block.top_ && adjacent_column->top_ > sliding_bottom )
        {
            // Create individual faces for each unit block, instead of one big face for the whole
            // block, to avoid the T-junctions that might otherwise occur (and cause seams).
            for ( Block::HeightT b = sliding_bottom; b < adjacent_column->bottom_; ++b )
            {
                add_block_face( column_world_position, b, Block::HeightT( b + 1 ), block.material_, direction, vertices );
            }

            if ( adjacent_column->top_ < block.top_ )
            {
                sliding_bottom = adjacent_column->top_;
            }
            else
            {
                sliding_bottom = block.top_;
                break;
            }
        }
        else if ( adjacent_column->bottom_ > block.top_ )
        {
            break;
        }

        adjacent_column = adjacent_column->next_;
    }

    for ( Block::HeightT b = sliding_bottom; b < block.top_; ++b )
    {
        add_block_face( column_world_position, b, Block::HeightT( b + 1 ), block.material_, direction, vertices );
    }
}

void add_block_endcaps(
    const Vector3i column_world_position,
    const Block& block,
    const Block* lower_block,
    const Block* neighbor_columns[NEIGHBOR_RELATION_SIZE],
    BlockVertexV& vertices
)
{
    bool bottom_cap = false;

    if ( lower_block )
    {
        if ( block.bottom_ > lower_block->top_ )
        {
            bottom_cap = true;
        }
    }
    else if ( block.bottom_ == 0 && neighbor_columns[NEIGHBOR_BELOW] )
    {
        const Block* lower_column = neighbor_columns[NEIGHBOR_BELOW];

        while ( lower_column->next_ )
        {
            lower_column = lower_column->next_;
        }

        if ( lower_column->top_ != Chunk::CHUNK_SIZE )
        {
            bottom_cap = true;
        }
    }
    else bottom_cap = true;

    if ( bottom_cap )
        add_block_face( column_world_position, block.bottom_, block.bottom_, block.material_, FACE_DIRECTION_DOWN, vertices ); 

    bool top_cap = false;

    if ( block.next_ )
    {
        if ( block.top_ < block.next_->bottom_ )
        {
            top_cap = true;
        }
    }
    else if ( block.top_ == Chunk::CHUNK_SIZE && neighbor_columns[NEIGHBOR_ABOVE] )
    {
        if ( neighbor_columns[NEIGHBOR_ABOVE]->bottom_ != 0 )
        {
            top_cap = true;
        }
    }
    else top_cap = true;

    if ( top_cap )
        add_block_face( column_world_position, block.top_, block.top_, block.material_, FACE_DIRECTION_UP, vertices ); 
}

} // anonymous namespace

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for ChunkRenderer:
//////////////////////////////////////////////////////////////////////////////////

ChunkRenderer::ChunkRenderer() :
    initialized_( false )
{
}

void ChunkRenderer::render()
{
    bind();

    glEnableClientState( GL_VERTEX_ARRAY );
    glEnableClientState( GL_NORMAL_ARRAY );
    glEnableClientState( GL_COLOR_ARRAY );

    glVertexPointer( 3, GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 0 ) );
    glNormalPointer( GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 12 ) );
    glColorPointer( 3, GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 24 ) );

    glDrawElements( GL_QUADS, vertex_count_, GL_UNSIGNED_INT, 0 );

    glDisableClientState( GL_COLOR_ARRAY );
    glDisableClientState( GL_NORMAL_ARRAY );
    glDisableClientState( GL_VERTEX_ARRAY );
}

void ChunkRenderer::initialize(
    const Vector3i& chunk_position,
    const Chunk& chunk,
    const Chunk* neighbor_chunks[NEIGHBOR_RELATION_SIZE]
)
{
    BlockVertexV vertices;

    // For performance, it is best if this is an overestimate rather than an underestimate.
    vertices.reserve( Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE * 128 );

    for ( int x = 0; x < Chunk::CHUNK_SIZE; ++x )
    {
        for ( int z = 0; z < Chunk::CHUNK_SIZE; ++z )
        {
            const Vector2i column_index( x, z );
            const Block* column = chunk.get_column( column_index );
            const Block* neighbor_columns[NEIGHBOR_RELATION_SIZE];
            neighbor_columns[NEIGHBOR_ABOVE] = find_colinear_column( neighbor_chunks[NEIGHBOR_ABOVE], column_index );
            neighbor_columns[NEIGHBOR_BELOW] = find_colinear_column( neighbor_chunks[NEIGHBOR_BELOW], column_index );
            neighbor_columns[NEIGHBOR_NORTH] = find_adjacent_column( chunk, neighbor_chunks[NEIGHBOR_NORTH], column_index + Vector2i(  0,  1 ) );
            neighbor_columns[NEIGHBOR_SOUTH] = find_adjacent_column( chunk, neighbor_chunks[NEIGHBOR_SOUTH], column_index + Vector2i(  0, -1 ) );
            neighbor_columns[NEIGHBOR_EAST]  = find_adjacent_column( chunk, neighbor_chunks[NEIGHBOR_EAST],  column_index + Vector2i(  1,  0 ) );
            neighbor_columns[NEIGHBOR_WEST]  = find_adjacent_column( chunk, neighbor_chunks[NEIGHBOR_WEST],  column_index + Vector2i( -1,  0 ) );

            const Vector3i column_world_position = chunk_position + Vector3i( column_index[0], 0, column_index[1] );
            const Block* lower_block = 0;

            while ( column )
            {
                add_block_faces( column_world_position, *column, neighbor_columns[NEIGHBOR_NORTH], FACE_DIRECTION_NORTH, vertices );
                add_block_faces( column_world_position, *column, neighbor_columns[NEIGHBOR_SOUTH], FACE_DIRECTION_SOUTH, vertices );
                add_block_faces( column_world_position, *column, neighbor_columns[NEIGHBOR_EAST],  FACE_DIRECTION_EAST,  vertices );
                add_block_faces( column_world_position, *column, neighbor_columns[NEIGHBOR_WEST],  FACE_DIRECTION_WEST,  vertices );

                add_block_endcaps( column_world_position, *column, lower_block, neighbor_columns, vertices );

                lower_block = column;
                column = column->next_;
            }
        }
    }

    create_buffers();
    bind();
    vertex_count_ = GLsizei( vertices.size() );

    glBufferData( GL_ARRAY_BUFFER, vertices.size() * sizeof( BlockVertex ), &vertices[0], GL_STATIC_DRAW );

    std::vector<GLuint> index_buffer;
    index_buffer.resize( vertices.size() );

    for ( int i = 0; i < int( vertices.size() ); ++i )
    {
        index_buffer[i] = i;
    }

    glBufferData( GL_ELEMENT_ARRAY_BUFFER, vertices.size() * sizeof( GLuint ), &index_buffer[0], GL_STATIC_DRAW );
}

void ChunkRenderer::create_buffers()
{
    glGenBuffers( 1, &vbo_id_ );
    glGenBuffers( 1, &ibo_id_ );
    initialized_ = true;
}

void ChunkRenderer::destroy_buffers()
{
    glDeleteBuffers( 1, &ibo_id_ );
    glDeleteBuffers( 1, &vbo_id_ );
    initialized_ = false;
}

void ChunkRenderer::bind()
{
    glBindBuffer( GL_ARRAY_BUFFER, vbo_id_ );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo_id_ );
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for Renderer:
//////////////////////////////////////////////////////////////////////////////////

Renderer::Renderer()
{
}

void Renderer::render( const RegionMap& regions )
{
    glCullFace( GL_BACK );
    glEnable( GL_CULL_FACE );

    // TODO: Abstract out
    GLfloat
        m_data[16],
        p_data[16];

    glGetFloatv( GL_MODELVIEW_MATRIX, m_data );
    glGetFloatv( GL_PROJECTION_MATRIX, p_data );

    gmtl::Matrix<Scalar, 4, 4> m;
    m.set( m_data );

    gmtl::Matrix<Scalar, 4, 4> p;
    p.set( p_data );

    gmtl::Frustumf view_frustum( m, p );

    int chunks_rendered = 0;

    for ( RegionMap::const_iterator region_it = regions.begin(); region_it != regions.end(); ++region_it )
    {
        const Region& region = *region_it->second;
        const Region* neighbor_regions[NEIGHBOR_RELATION_SIZE];
        neighbor_regions[NEIGHBOR_ABOVE] = 0;
        neighbor_regions[NEIGHBOR_BELOW] = 0;
        neighbor_regions[NEIGHBOR_NORTH] = find_region( regions, region.position() + Vector2i(  0,  Region::REGION_SIZE ) );
        neighbor_regions[NEIGHBOR_SOUTH] = find_region( regions, region.position() + Vector2i(  0, -Region::REGION_SIZE ) ); 
        neighbor_regions[NEIGHBOR_EAST]  = find_region( regions, region.position() + Vector2i(  Region::REGION_SIZE, 0 ) );
        neighbor_regions[NEIGHBOR_WEST]  = find_region( regions, region.position() + Vector2i( -Region::REGION_SIZE, 0 ) );

        // TODO: Frustum cull entire regions? (This requires that regions keep track of their AABoxes).
        //       Maybe even do some kind of hierarchical culling (This requires that regions keep a hierarchy).

        for ( ChunkMap::const_iterator chunk_it = region.chunks().begin(); chunk_it != region.chunks().end(); ++chunk_it )
        {
            const Vector3i chunk_index = chunk_it->first;
            const Vector3i chunk_position = Vector3i( region.position()[0], 0, region.position()[1] ) + chunk_index * int( Chunk::CHUNK_SIZE );
            const Vector3f chunk_min = vector_cast<Vector3f>( chunk_position );
            const Vector3f chunk_max = chunk_min + Vector3f( Chunk::CHUNK_SIZE, Chunk::CHUNK_SIZE, Chunk::CHUNK_SIZE );
            const gmtl::AABoxf chunk_box( chunk_min, chunk_max );

            if ( gmtl::isInVolume( view_frustum, chunk_box ) )
            {
                const Chunk* neighbor_chunks[NEIGHBOR_RELATION_SIZE];
                neighbor_chunks[NEIGHBOR_ABOVE] = find_adjacent_chunk( region, neighbor_regions[NEIGHBOR_ABOVE], chunk_index + Vector3i(  0,  1,  0 ) );
                neighbor_chunks[NEIGHBOR_BELOW] = find_adjacent_chunk( region, neighbor_regions[NEIGHBOR_BELOW], chunk_index + Vector3i(  0, -1,  0 ) );
                neighbor_chunks[NEIGHBOR_NORTH] = find_adjacent_chunk( region, neighbor_regions[NEIGHBOR_NORTH], chunk_index + Vector3i(  0,  0,  1 ) );
                neighbor_chunks[NEIGHBOR_SOUTH] = find_adjacent_chunk( region, neighbor_regions[NEIGHBOR_SOUTH], chunk_index + Vector3i(  0,  0, -1 ) );
                neighbor_chunks[NEIGHBOR_EAST]  = find_adjacent_chunk( region, neighbor_regions[NEIGHBOR_EAST],  chunk_index + Vector3i(  1,  0,  0 ) );
                neighbor_chunks[NEIGHBOR_WEST]  = find_adjacent_chunk( region, neighbor_regions[NEIGHBOR_WEST],  chunk_index + Vector3i( -1,  0,  0 ) );

                ChunkRenderer& chunk_renderer = chunk_renderers_[chunk_position];

                if ( !chunk_renderer.initialized_ )
                {
                    // TODO: Consider doing Chunk initialization at the time when a new Region is
                    //       generated, rather than when the Chunk is first seen.
                    chunk_renderer.initialize( chunk_position, *chunk_it->second, neighbor_chunks );
                }

                chunk_renderer.render();
                ++chunks_rendered;
            }
        }
    }
    // FIXME: std::cout << "Chunks rendered: " << chunks_rendered << std::endl;
}
