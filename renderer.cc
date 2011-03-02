#define GL_GLEXT_PROTOTYPES

#include <GL/gl.h>

#include <boost/numeric/conversion/cast.hpp>

#include "renderer.h"

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for ChunkVertexBuffer:
//////////////////////////////////////////////////////////////////////////////////

ChunkVertexBuffer::ChunkVertexBuffer() :
    vertex_count_( 0 )
{
}

ChunkVertexBuffer::ChunkVertexBuffer( const BlockVertexV& vertices ) :
    vertex_count_( boost::numeric_cast<GLsizei>( vertices.size() ) )
{
    assert( vertex_count_ > 0 );

    glGenBuffers( 1, &vbo_id_ );
    glGenBuffers( 1, &ibo_id_ );

    bind();
    glBufferData( GL_ARRAY_BUFFER, vertices.size() * sizeof( BlockVertex ), &vertices[0], GL_STATIC_DRAW );

    std::vector<GLuint> index_buffer;
    index_buffer.resize( vertices.size() );

    for ( int i = 0; i < int( vertices.size() ); ++i )
    {
        index_buffer[i] = i;
    }

    glBufferData( GL_ELEMENT_ARRAY_BUFFER, vertices.size() * sizeof( GLuint ), &index_buffer[0], GL_STATIC_DRAW );
}

ChunkVertexBuffer::~ChunkVertexBuffer()
{
    if ( vertex_count_ )
    {
        glDeleteBuffers( 1, &ibo_id_ );
        glDeleteBuffers( 1, &vbo_id_ );
    }
}

void ChunkVertexBuffer::render()
{
    glEnableClientState( GL_VERTEX_ARRAY );
    glEnableClientState( GL_NORMAL_ARRAY );
    glEnableClientState( GL_TEXTURE_COORD_ARRAY );

    bind();

    glVertexPointer( 3, GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 0 ) );
    glNormalPointer( GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 12 ) );

    glClientActiveTexture( GL_TEXTURE0 );
    glTexCoordPointer( 2, GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 24 ) );

    glDrawElements( GL_QUADS, vertex_count_, GL_UNSIGNED_INT, 0 );

    glDisableClientState( GL_TEXTURE_COORD_ARRAY );
    glDisableClientState( GL_NORMAL_ARRAY );
    glDisableClientState( GL_VERTEX_ARRAY );
}

void ChunkVertexBuffer::bind()
{
    glBindBuffer( GL_ARRAY_BUFFER, vbo_id_ );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo_id_ );
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for ChunkRenderer:
//////////////////////////////////////////////////////////////////////////////////

ChunkRenderer::ChunkRenderer() :
    initialized_( false )
{
}

void ChunkRenderer::render( const RendererMaterialV& materials )
{
    for ( ChunkVertexBufferMap::iterator it = vbos_.begin(); it != vbos_.end(); ++it )
    {
        const BlockMaterial material = it->first;
        ChunkVertexBuffer& vbo = *it->second;

        assert( material >= 0 && material < static_cast<int>( materials.size() ) );
        const RendererMaterial& renderer_material = *materials[material];

        glEnable( GL_TEXTURE_2D );
        glClientActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, renderer_material.texture().texture_id() );
        vbo.render();
    }
}

void ChunkRenderer::initialize( const Chunk& chunk )
{
    const BlockFaceV& faces = chunk.get_external_faces();

    typedef std::map<BlockMaterial, BlockVertexV> MaterialVertexMap;

    MaterialVertexMap material_vertices;

    for ( BlockFaceV::const_iterator face_it = faces.begin(); face_it != faces.end(); ++face_it )
    {
        // FIXME:
        // for ( size_t i = 0; i < BlockFace::NUM_VERTICES; ++i )
        // {
        //     // TODO: Texture coordinates
        //     material_vertices[face_it->material_].push_back( BlockVertex( face_it->vertices_[i], face_it->normal_, Vector2f( 0.5f, 0.5f ) ) );
        // }

        material_vertices[face_it->material_].push_back( BlockVertex( face_it->vertices_[0], face_it->normal_, Vector2f( 0.0f, 0.0f ) ) );
        material_vertices[face_it->material_].push_back( BlockVertex( face_it->vertices_[1], face_it->normal_, Vector2f( 1.0f, 0.0f ) ) );
        material_vertices[face_it->material_].push_back( BlockVertex( face_it->vertices_[2], face_it->normal_, Vector2f( 1.0f, 1.0f ) ) );
        material_vertices[face_it->material_].push_back( BlockVertex( face_it->vertices_[3], face_it->normal_, Vector2f( 0.0f, 1.0f ) ) );
    }

    for ( MaterialVertexMap::const_iterator it = material_vertices.begin(); it != material_vertices.end(); ++it )
    {
        ChunkVertexBufferSP vbo( new ChunkVertexBuffer( it->second ) );
        vbos_[it->first] = vbo;
    }

    initialized_ = true;
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for Renderer:
//////////////////////////////////////////////////////////////////////////////////

Renderer::Renderer()
{
    materials_.resize( BLOCK_MATERIAL_SIZE );
    materials_[BLOCK_MATERIAL_GRASS].reset  ( new RendererMaterial( "grass" ) );
    materials_[BLOCK_MATERIAL_DIRT].reset   ( new RendererMaterial( "dirt" ) );
    materials_[BLOCK_MATERIAL_CLAY].reset   ( new RendererMaterial( "clay" ) );
    materials_[BLOCK_MATERIAL_STONE].reset  ( new RendererMaterial( "stone" ) );
    materials_[BLOCK_MATERIAL_BEDROCK].reset( new RendererMaterial( "bedrock" ) );
    materials_[BLOCK_MATERIAL_MAGMA].reset  ( new RendererMaterial( "magma" ) );
}

void Renderer::render( const ChunkMap& chunks )
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

    // TODO: Arrange the chunks into some kind of hierarchy and cull based on that.

    // TODO: Sort (using an ordering table) the chunks, and render them from front to back.  Eventually,
    //       use an ARB_occlusion_query to avoid rendering fully occluded chunks.

    // TODO: Render the translucent parts of the chunks from back to front.

    // TODO: Look into glMultiDrawElements or display lists to reduce the number of OpenGL library calls.

    int chunks_rendered = 0;
    int chunks_total = 0;

    for ( ChunkMap::const_iterator chunk_it = chunks.begin(); chunk_it != chunks.end(); ++chunk_it )
    {
        const Vector3i chunk_position = chunk_it->first;
        const Vector3f chunk_min = vector_cast<Vector3f>( chunk_position );
        const Vector3f chunk_max = chunk_min + Vector3f( Chunk::CHUNK_SIZE, Chunk::CHUNK_SIZE, Chunk::CHUNK_SIZE );
        const gmtl::AABoxf chunk_box( chunk_min, chunk_max );

        if ( gmtl::isInVolume( view_frustum, chunk_box ) )
        {
            ChunkRenderer& chunk_renderer = chunk_renderers_[chunk_position];

            if ( !chunk_renderer.initialized() )
            {
                // TODO: Do Chunk initialization at the time when a new Region is
                //       generated/modified, rather than when the Chunk is first seen.
                chunk_renderer.initialize( *chunk_it->second );
            }

            chunk_renderer.render( materials_ );
            ++chunks_rendered;
        }
        ++chunks_total;
    }

    // TODO: Just for development.
    // static unsigned c = 0;
    // if ( ++c % 60 == 0 )
    //     std::cout << "Chunks rendered: " << chunks_rendered << " / " << chunks_total << std::endl;
}
