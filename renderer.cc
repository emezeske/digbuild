#define GL_GLEXT_PROTOTYPES

#include <GL/gl.h>

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
    { 0.34f, 0.34f, 0.34f }, // BLOCK_MATERIAL_BEDROCK
    { 0.96f, 0.24f, 0.00f }  // BLOCK_MATERIAL_MAGMA
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

void ChunkRenderer::initialize( const Chunk& chunk )
{
    const BlockFaceV& faces = chunk.external_faces();

    BlockVertexV vertices;
    vertices.resize( faces.size() * BlockFace::NUM_VERTICES );
    BlockVertexV::iterator vertex_it = vertices.begin();

    // FIXME: Split faces into squares to avoid seams.

    // TODO: Once translucent materials are added, any such faces must NOT be added to the
    //       VBO here -- they need to be added to a separate buffer, and when they are rendered
    //       they must be sorted.  Consider using an ordering table if a regular sort is not fast enough.

    for ( BlockFaceV::const_iterator face_it = faces.begin(); face_it != faces.end(); ++face_it )
    {
        for ( size_t i = 0; i < BlockFace::NUM_VERTICES; ++i )
        {
            assert( face_it->material_ >= 0 && face_it->material_ < int( sizeof( MATERIAL_INFO ) / sizeof( MaterialInfo ) ) );
            const MaterialInfo& material_info = MATERIAL_INFO[face_it->material_];
            ( vertex_it++ )->set( face_it->vertices_[i], face_it->normal_, material_info );
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
    int chunks_total = 0;

    for ( RegionMap::const_iterator region_it = regions.begin(); region_it != regions.end(); ++region_it )
    {
        const Region& region = *region_it->second;

        // TODO: Frustum cull entire regions? (This requires that regions keep track of their AABoxes).
        //       Maybe even do some kind of hierarchical culling (This requires that regions keep a hierarchy).

        // TODO: Sort (using an ordering table) the chunks, and render them from front to back.  Eventually,
        //       use an ARB_occlusion_query to avoid rendering fully occluded chunks.

        // TODO: Render the translucent parts of the chunks from back to front.

        for ( ChunkMap::const_iterator chunk_it = region.chunks().begin(); chunk_it != region.chunks().end(); ++chunk_it )
        {
            const Vector3i chunk_index = chunk_it->first;
            const Vector3i chunk_position = Vector3i( region.position()[0], 0, region.position()[1] ) + chunk_index * int( Chunk::CHUNK_SIZE );
            const Vector3f chunk_min = vector_cast<Vector3f>( chunk_position );
            const Vector3f chunk_max = chunk_min + Vector3f( Chunk::CHUNK_SIZE, Chunk::CHUNK_SIZE, Chunk::CHUNK_SIZE );
            const gmtl::AABoxf chunk_box( chunk_min, chunk_max );

            if ( gmtl::isInVolume( view_frustum, chunk_box ) )
            {
                ChunkRenderer& chunk_renderer = chunk_renderers_[chunk_position];

                if ( !chunk_renderer.initialized_ )
                {
                    // TODO: Consider doing Chunk initialization at the time when a new Region is
                    //       generated, rather than when the Chunk is first seen.
                    chunk_renderer.initialize( *chunk_it->second );
                }

                chunk_renderer.render();
                ++chunks_rendered;
            }
            ++chunks_total;
        }
    }

    // TODO: Just for development.
    static unsigned c = 0;
    if ( ++c % 60 == 0 )
        std::cout << "Chunks rendered: " << chunks_rendered << " / " << chunks_total << std::endl;
}
