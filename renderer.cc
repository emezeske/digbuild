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
    BlockVertex( const Vector3f position, const Vector3f normal, const MaterialInfo& material_info ) :
        x_( position[0] ), y_( position[1] ), z_( position[2] ),
        nx_( normal[0] ), ny_( normal[1] ), nz_( normal[2] ),
        r_( material_info.r_ ), g_( material_info.g_ ), b_( material_info.b_ )
    {

    }

    GLfloat x_, y_, z_;
    GLfloat nx_, ny_, nz_;
    GLfloat r_, g_, b_;
} __attribute__((packed));

typedef std::vector<BlockVertex> BlockVertexV;

// TODO: Consider using a position_ map for the regions instead of a vector.
RegionV::const_iterator find_region( const RegionV& regions, const Vector2i position )
{
    for ( RegionV::const_iterator region_it = regions.begin(); region_it != regions.end(); ++region_it )
    {
        if ( region_it->position_ == position )
        {
            return region_it;
        }
    }

    return regions.end();
}

// TODO: Now that all position coordinates are global and use block units, it might be possible
//       to make these find_neighboring_* functions less bizarre.

const Chunk* find_neighboring_chunk(
    const RegionV& regions,
    const Region& region,
    const Vector2i& chunk_index,
    const Vector2i& neighbor_offset
)
{
    Vector2i neighbor_index = chunk_index + neighbor_offset;

    if ( neighbor_index[0] >= 0 &&
         neighbor_index[1] >= 0 &&
         neighbor_index[0] < Region::CHUNKS_PER_EDGE &&
         neighbor_index[1] < Region::CHUNKS_PER_EDGE )
    {
        return &region.get_chunk( neighbor_index );
    }
    else
    {
        const Vector2i neighbor_region_position = region.position_  + neighbor_offset * int( Region::BLOCKS_PER_EDGE );
        RegionV::const_iterator region_it = find_region( regions, neighbor_region_position );

        if ( region_it != regions.end() )
        {
            neighbor_index[0] = ( Region::CHUNKS_PER_EDGE + neighbor_index[0] ) % Region::CHUNKS_PER_EDGE;
            neighbor_index[1] = ( Region::CHUNKS_PER_EDGE + neighbor_index[1] ) % Region::CHUNKS_PER_EDGE;
            return &region_it->get_chunk( neighbor_index );
        }
    }

    return 0;
}

const BlockV* find_neighboring_column( const Chunk& chunk, const Chunk* neighbor_chunk, Vector2i neighbor_index )
{
    if ( neighbor_index[0] >= 0 &&
         neighbor_index[1] >= 0 &&
         neighbor_index[0] < Chunk::BLOCKS_PER_EDGE &&
         neighbor_index[1] < Chunk::BLOCKS_PER_EDGE )
    {
        return &chunk.get_column( neighbor_index );
    }
    else if ( neighbor_chunk )
    {
        neighbor_index[0] = ( Chunk::BLOCKS_PER_EDGE + neighbor_index[0] ) % Chunk::BLOCKS_PER_EDGE;
        neighbor_index[1] = ( Chunk::BLOCKS_PER_EDGE + neighbor_index[1] ) % Chunk::BLOCKS_PER_EDGE;
        return &neighbor_chunk->get_column( neighbor_index );
    }
    
    return 0;
}

void add_visible_block_vertex(
    const Vector2i column_world_position,
    uint8_t block_bottom, 
    uint8_t block_top, 
    const BlockMaterial material,
    const FaceDirection direction,
    BlockVertexV& vertices
)
{
    const Vector3f block_world_position(
        Scalar( column_world_position[0] ),
        block_bottom,
        Scalar( column_world_position[1] )
    );
    const Scalar height = Scalar( block_top - block_bottom );

    assert( material >= 0 && material < int( sizeof( MATERIAL_INFO ) / sizeof( MaterialInfo ) ) );
    const MaterialInfo& material_info = MATERIAL_INFO[material];

    #define V( x, y, z, n ) do { vertices.push_back( BlockVertex( block_world_position + Vector3f( x, y, z ), n, material_info ) ); } while ( false )
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

void add_visible_block_faces(
    const Vector2i column_world_position,
    const Block& block,
    BlockV::const_iterator adjacent_it,
    const BlockV::const_iterator adjacent_end,
    const FaceDirection direction,
    BlockVertexV& vertices
)
{
    uint8_t sliding_bottom = block.bottom_;

    while ( adjacent_it != adjacent_end )
    {
        if ( adjacent_it->bottom_ < block.top_ && adjacent_it->top_ > sliding_bottom )
        {
            // Create individual faces for each unit block, instead of one big face for the whole
            // block, to avoid the T-junctions that might otherwise occur (and cause seams).
            for ( uint8_t b = sliding_bottom; b < adjacent_it->bottom_; ++b )
            {
                add_visible_block_vertex( column_world_position, b, uint8_t( b + 1 ), block.material(), direction, vertices );
            }

            if ( adjacent_it->top_ < block.top_ )
            {
                sliding_bottom = adjacent_it->top_;
            }
            else
            {
                sliding_bottom = block.top_;
                break;
            }
        }
        else if ( adjacent_it->bottom_ > block.top_ )
        {
            break;
        }

        ++adjacent_it;
    }

    for ( uint8_t b = sliding_bottom; b < block.top_; ++b )
    {
        add_visible_block_vertex( column_world_position, b, uint8_t( b + 1 ), block.material(), direction, vertices );
    }
}

} // anonymous namespace

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for VertexBuffer:
//////////////////////////////////////////////////////////////////////////////////

VertexBuffer::VertexBuffer() :
    initialized_( false )
{
}

void VertexBuffer::create_buffers()
{
    glGenBuffers( 1, &vbo_id_ );
    glGenBuffers( 1, &ibo_id_ );
    initialized_ = true;
}

void VertexBuffer::destroy_buffers()
{
    glDeleteBuffers( 1, &ibo_id_ );
    glDeleteBuffers( 1, &vbo_id_ );
    initialized_ = false;
}

void VertexBuffer::bind()
{
    glBindBuffer( GL_ARRAY_BUFFER, vbo_id_ );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo_id_ );
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for Renderer::ComparableChunkPosition:
//////////////////////////////////////////////////////////////////////////////////

Renderer::ComparableChunkPosition::ComparableChunkPosition( const Vector2i& position ) :
    position_( position )
{
}

bool Renderer::ComparableChunkPosition::operator<( const Renderer::ComparableChunkPosition& other ) const
{
    if ( position_[0] < other.position_[0] )
        return true;
    if ( position_[0] > other.position_[0] )
        return false;
    if ( position_[1] < other.position_[1] )
        return true;
    return false;
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for Renderer:
//////////////////////////////////////////////////////////////////////////////////

Renderer::Renderer()
{
}

void Renderer::render( const RegionV& regions )
{
    glCullFace( GL_BACK );
    glEnable( GL_CULL_FACE );

    for ( RegionV::const_iterator region_it = regions.begin(); region_it != regions.end(); ++region_it )
    {
        for ( int chunk_x = 0; chunk_x < Region::CHUNKS_PER_EDGE; ++chunk_x )
        {
            for ( int chunk_z = 0; chunk_z < Region::CHUNKS_PER_EDGE; ++chunk_z )
            {
                const Vector2i chunk_index( chunk_x, chunk_z );
                const Chunk& chunk = region_it->get_chunk( chunk_index );

                const Chunk* chunk_north = find_neighboring_chunk( regions, *region_it, chunk_index, Vector2i(  0,  1 ) );
                const Chunk* chunk_south = find_neighboring_chunk( regions, *region_it, chunk_index, Vector2i(  0, -1 ) ); 
                const Chunk* chunk_east  = find_neighboring_chunk( regions, *region_it, chunk_index, Vector2i(  1,  0 ) );
                const Chunk* chunk_west  = find_neighboring_chunk( regions, *region_it, chunk_index, Vector2i( -1,  0 ) );

                const Vector2i chunk_position = region_it->position_ + Vector2i( chunk_x * Chunk::BLOCKS_PER_EDGE, chunk_z * Chunk::BLOCKS_PER_EDGE );
                render_chunk( chunk_position, chunk, chunk_north, chunk_south, chunk_east, chunk_west );
            }
        }
    }
}

void Renderer::render_chunk(
    const Vector2i& chunk_position,
    const Chunk& chunk,
    const Chunk* chunk_north,
    const Chunk* chunk_south,
    const Chunk* chunk_east,
    const Chunk* chunk_west
)
{
    // TODO: Decompose this function a bit more.

    VertexBuffer& vertex_buffer = chunk_vbos_[ComparableChunkPosition( chunk_position )];

    if ( !vertex_buffer.initialized_ )
    {
        BlockVertexV vertices;
        vertices.reserve( Chunk::BLOCKS_PER_EDGE * Chunk::BLOCKS_PER_EDGE * 4 ); // TODO: Measure & refine this estimate.

        for ( int cx = 0; cx < Chunk::BLOCKS_PER_EDGE; ++cx )
        {
            for ( int cz = 0; cz < Chunk::BLOCKS_PER_EDGE; ++cz )
            {
                const Vector2i column_position( cx, cz );
                const BlockV& column = chunk.get_column( column_position );

                const BlockV* column_north = find_neighboring_column( chunk, chunk_north, column_position + Vector2i(  0,  1 ) );
                const BlockV* column_south = find_neighboring_column( chunk, chunk_south, column_position + Vector2i(  0, -1 ) );
                const BlockV* column_east  = find_neighboring_column( chunk, chunk_east,  column_position + Vector2i(  1,  0 ) );
                const BlockV* column_west  = find_neighboring_column( chunk, chunk_west,  column_position + Vector2i( -1,  0 ) );

                const BlockV empty;

                BlockV::const_iterator
                    north_it  = column_north ? column_north->begin() : empty.end(),
                    north_end = column_north ? column_north->end()   : empty.end(),
                    south_it  = column_south ? column_south->begin() : empty.end(),
                    south_end = column_south ? column_south->end()   : empty.end(),
                    east_it   = column_east  ? column_east->begin()  : empty.end(),
                    east_end  = column_east  ? column_east->end()    : empty.end(),
                    west_it   = column_west  ? column_west->begin()  : empty.end(),
                    west_end  = column_west  ? column_west->end()    : empty.end();

                const Vector2i column_world_position( chunk_position[0] + cx, chunk_position[1] + cz );

                for ( BlockV::const_iterator block_it = column.begin(); block_it < column.end(); ++block_it )
                {
                    add_visible_block_faces( column_world_position, *block_it, north_it, north_end, FACE_DIRECTION_NORTH, vertices );
                    add_visible_block_faces( column_world_position, *block_it, south_it, south_end, FACE_DIRECTION_SOUTH, vertices );
                    add_visible_block_faces( column_world_position, *block_it, east_it,  east_end,  FACE_DIRECTION_EAST,  vertices );
                    add_visible_block_faces( column_world_position, *block_it, west_it,  west_end,  FACE_DIRECTION_WEST,  vertices );

                    bool
                        bottom_cap = false,
                        top_cap = false;

                    if ( block_it > column.begin() )
                    {
                        const BlockV::const_iterator lower_block = block_it - 1;

                        if ( block_it->bottom_ > lower_block->top_ )
                        {
                            bottom_cap = true;
                        }
                    }
                    else if ( block_it->bottom_ != 0 ) // Don't bother drawing bottom caps for the bottom-most blocks.
                    {
                        bottom_cap = true;
                    }

                    if ( block_it + 1 < column.end() )
                    {
                        const BlockV::const_iterator upper_block = block_it + 1;

                        if ( block_it->top_ < upper_block->bottom_ )
                        {
                            top_cap = true;
                        }
                    }
                    else top_cap = true;

                    if ( bottom_cap )
                        add_visible_block_vertex( column_world_position, block_it->bottom_, block_it->bottom_, block_it->material(), FACE_DIRECTION_DOWN, vertices ); 

                    if ( top_cap )
                        add_visible_block_vertex( column_world_position, block_it->top_, block_it->top_, block_it->material(), FACE_DIRECTION_UP, vertices ); 
                }
            }
        }

        vertex_buffer.create_buffers();
        vertex_buffer.bind();
        vertex_buffer.vertex_count_ = GLsizei( vertices.size() );

        glBufferData( GL_ARRAY_BUFFER, vertices.size() * sizeof( BlockVertex ), &vertices[0], GL_STATIC_DRAW );

        std::vector<GLuint> index_buffer;
        index_buffer.reserve( vertices.size() );

        for ( int i = 0; i < int( vertices.size() ); ++i )
        {
            index_buffer.push_back( i );
        }

        glBufferData( GL_ELEMENT_ARRAY_BUFFER, vertices.size() * sizeof( GLuint ), &index_buffer[0], GL_STATIC_DRAW );
    }
    else vertex_buffer.bind();

    glEnableClientState( GL_VERTEX_ARRAY );
    glEnableClientState( GL_NORMAL_ARRAY );
    glEnableClientState( GL_COLOR_ARRAY );

    glVertexPointer( 3, GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 0 ) );
    glNormalPointer( GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 12 ) );
    glColorPointer( 3, GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 24 ) );

    glDrawElements( GL_QUADS, vertex_buffer.vertex_count_, GL_UNSIGNED_INT, 0 );

    glDisableClientState( GL_COLOR_ARRAY );
    glDisableClientState( GL_NORMAL_ARRAY );
    glDisableClientState( GL_VERTEX_ARRAY );
}
