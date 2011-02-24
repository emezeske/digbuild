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

const int
    CHUNKS_PER_REGION_EDGE = 4,
    CHUNK_SIZE = Region::REGION_SIZE / CHUNKS_PER_REGION_EDGE;

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
    RegionMap::const_iterator region_it = regions.find( position );
    return ( region_it == regions.end() ? 0 : region_it->second.get() );
}

const Block* find_neighboring_column( const Region& region, const Region* neighbor_region, Vector2i neighbor_index )
{
    if ( neighbor_index[0] >= 0 &&
         neighbor_index[1] >= 0 &&
         neighbor_index[0] < Region::REGION_SIZE &&
         neighbor_index[1] < Region::REGION_SIZE )
    {
        return region.get_column( neighbor_index );
    }
    else if ( neighbor_region )
    {
        neighbor_index[0] = ( Region::REGION_SIZE + neighbor_index[0] ) % Region::REGION_SIZE;
        neighbor_index[1] = ( Region::REGION_SIZE + neighbor_index[1] ) % Region::REGION_SIZE;
        return neighbor_region->get_column( neighbor_index );
    }
    
    return 0;
}

void add_block_face
(
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

void add_block_faces
(
    const Vector2i column_world_position,
    const Block& block,
    const Block* adjacent_column,
    const FaceDirection direction,
    BlockVertexV& vertices
)
{
    uint8_t sliding_bottom = block.bottom_;

    while ( adjacent_column )
    {
        if ( adjacent_column->bottom_ < block.top_ && adjacent_column->top_ > sliding_bottom )
        {
            // Create individual faces for each unit block, instead of one big face for the whole
            // block, to avoid the T-junctions that might otherwise occur (and cause seams).
            for ( uint8_t b = sliding_bottom; b < adjacent_column->bottom_; ++b )
            {
                add_block_face( column_world_position, b, uint8_t( b + 1 ), block.material(), direction, vertices );
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

    for ( uint8_t b = sliding_bottom; b < block.top_; ++b )
    {
        add_block_face( column_world_position, b, uint8_t( b + 1 ), block.material(), direction, vertices );
    }
}

void add_block_endcaps
(
    const Vector2i column_world_position,
    const Block& block,
    const Block* lower_block,
    BlockVertexV& vertices
)
{
    bool
        bottom_cap = false,
        top_cap = false;

    if ( lower_block )
    {
        if ( block.bottom_ > lower_block->top_ )
        {
            bottom_cap = true;
        }
    }
    else if ( block.bottom_ != 0 ) // Don't bother drawing bottom caps for the bottom-most blocks.
    {
        bottom_cap = true;
    }

    if ( block.next_ )
    {
        if ( block.top_ < block.next_->bottom_ )
        {
            top_cap = true;
        }
    }
    else top_cap = true;

    if ( bottom_cap )
        add_block_face( column_world_position, block.bottom_, block.bottom_, block.material(), FACE_DIRECTION_DOWN, vertices ); 

    if ( top_cap )
        add_block_face( column_world_position, block.top_, block.top_, block.material(), FACE_DIRECTION_UP, vertices ); 
}

void draw_vertex_buffer( const VertexBuffer& buffer )
{
    glEnableClientState( GL_VERTEX_ARRAY );
    glEnableClientState( GL_NORMAL_ARRAY );
    glEnableClientState( GL_COLOR_ARRAY );

    glVertexPointer( 3, GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 0 ) );
    glNormalPointer( GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 12 ) );
    glColorPointer( 3, GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 24 ) );

    glDrawElements( GL_QUADS, buffer.vertex_count_, GL_UNSIGNED_INT, 0 );

    glDisableClientState( GL_COLOR_ARRAY );
    glDisableClientState( GL_NORMAL_ARRAY );
    glDisableClientState( GL_VERTEX_ARRAY );
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
// Function definitions for Renderer:
//////////////////////////////////////////////////////////////////////////////////

Renderer::Renderer()
{
}

void Renderer::render( const RegionMap& regions )
{
    glCullFace( GL_BACK );
    glEnable( GL_CULL_FACE );

    for ( RegionMap::const_iterator region_it = regions.begin(); region_it != regions.end(); ++region_it )
    {
        const Region& region = *region_it->second;
        const Region* region_north = find_region( regions, region.position_ + Vector2i(  0,  Region::REGION_SIZE ) );
        const Region* region_south = find_region( regions, region.position_ + Vector2i(  0, -Region::REGION_SIZE ) ); 
        const Region* region_east  = find_region( regions, region.position_ + Vector2i(  Region::REGION_SIZE, 0 ) );
        const Region* region_west  = find_region( regions, region.position_ + Vector2i( -Region::REGION_SIZE, 0 ) );

        for ( int x = 0; x < CHUNKS_PER_REGION_EDGE; ++x )
        {
            for ( int z = 0; z < CHUNKS_PER_REGION_EDGE; ++z )
            {
                render_chunk( Vector2i( x, z ), region, region_north, region_south, region_east, region_west );
            }
        }
    }
}

void Renderer::render_chunk(
    const Vector2i& chunk_index,
    const Region& region,
    const Region* region_north,
    const Region* region_south,
    const Region* region_east,
    const Region* region_west
)
{
    const Vector2i chunk_position = region.position_ + chunk_index * CHUNK_SIZE;
    VertexBuffer& vertex_buffer = chunk_vbos_[chunk_position];

    if ( !vertex_buffer.initialized_ )
    {
        BlockVertexV vertices;

        // For performance, it is best if this is an overestimate rather than an underestimate.
        vertices.reserve( CHUNK_SIZE * CHUNK_SIZE * 256 );

        for ( int x = 0; x < CHUNK_SIZE; ++x )
        {
            for ( int z = 0; z < CHUNK_SIZE; ++z )
            {
                const Vector2i block_index( x, z );
                const Vector2i column_index = chunk_index * CHUNK_SIZE + block_index;
                const Block* column = region.get_column( column_index );
                const Block* column_north = find_neighboring_column( region, region_north, column_index + Vector2i(  0,  1 ) );
                const Block* column_south = find_neighboring_column( region, region_south, column_index + Vector2i(  0, -1 ) );
                const Block* column_east  = find_neighboring_column( region, region_east,  column_index + Vector2i(  1,  0 ) );
                const Block* column_west  = find_neighboring_column( region, region_west,  column_index + Vector2i( -1,  0 ) );

                const Vector2i column_world_position = chunk_position + block_index;
                const Block* lower_block = 0;

                while ( column )
                {
                    add_block_faces( column_world_position, *column, column_north, FACE_DIRECTION_NORTH, vertices );
                    add_block_faces( column_world_position, *column, column_south, FACE_DIRECTION_SOUTH, vertices );
                    add_block_faces( column_world_position, *column, column_east,  FACE_DIRECTION_EAST,  vertices );
                    add_block_faces( column_world_position, *column, column_west,  FACE_DIRECTION_WEST,  vertices );
                    add_block_endcaps( column_world_position, *column, lower_block, vertices );

                    lower_block = column;
                    column = column->next_;
                }
            }
        }

        vertex_buffer.create_buffers();
        vertex_buffer.bind();
        vertex_buffer.vertex_count_ = GLsizei( vertices.size() );

        glBufferData( GL_ARRAY_BUFFER, vertices.size() * sizeof( BlockVertex ), &vertices[0], GL_STATIC_DRAW );

        std::vector<GLuint> index_buffer;
        index_buffer.resize( vertices.size() );

        for ( int i = 0; i < int( vertices.size() ); ++i )
        {
            index_buffer[i] = i;
        }

        glBufferData( GL_ELEMENT_ARRAY_BUFFER, vertices.size() * sizeof( GLuint ), &index_buffer[0], GL_STATIC_DRAW );
    }
    else vertex_buffer.bind();

    draw_vertex_buffer( vertex_buffer );
}
