#define GL_GLEXT_PROTOTYPES

#include <GL/glew.h>

#include <boost/numeric/conversion/cast.hpp>

#include "renderer.h"

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for VertexBuffer:
//////////////////////////////////////////////////////////////////////////////////

VertexBuffer::VertexBuffer( const GLsizei num_elements ) :
    num_elements_( num_elements )
{
    glGenBuffers( 1, &vbo_id_ );
    glGenBuffers( 1, &ibo_id_ );
}

VertexBuffer::~VertexBuffer()
{
    glDeleteBuffers( 1, &ibo_id_ );
    glDeleteBuffers( 1, &vbo_id_ );
}

void VertexBuffer::bind()
{
    glBindBuffer( GL_ARRAY_BUFFER, vbo_id_ );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo_id_ );
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for ChunkVertexBuffer:
//////////////////////////////////////////////////////////////////////////////////

ChunkVertexBuffer::ChunkVertexBuffer( const BlockVertexV& vertices ) :
    VertexBuffer( boost::numeric_cast<GLsizei>( vertices.size() ) )
{
    assert( vertices.size() > 0 );

    bind();
    glBufferData( GL_ARRAY_BUFFER, vertices.size() * sizeof( BlockVertex ), &vertices[0], GL_STATIC_DRAW );

    std::vector<GLuint> indices;
    indices.resize( vertices.size() );

    for ( int i = 0; i < int( vertices.size() ); ++i )
    {
        indices[i] = i;
    }

    glBufferData( GL_ELEMENT_ARRAY_BUFFER, vertices.size() * sizeof( GLuint ), &indices[0], GL_STATIC_DRAW );
}

void ChunkVertexBuffer::render()
{
    bind();

    glEnableClientState( GL_VERTEX_ARRAY );
    glVertexPointer( 3, GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 0 ) );

    glEnableClientState( GL_NORMAL_ARRAY );
    glNormalPointer( GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 12 ) );

    glClientActiveTexture( GL_TEXTURE0 );
    glEnableClientState( GL_TEXTURE_COORD_ARRAY );
    glTexCoordPointer( 2, GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 24 ) );

    glClientActiveTexture( GL_TEXTURE1 );
    glEnableClientState( GL_TEXTURE_COORD_ARRAY );
    glTexCoordPointer( 4, GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 32 ) );

    glDrawElements( GL_TRIANGLES, num_elements_, GL_UNSIGNED_INT, 0 );
    glDisableClientState( GL_TEXTURE_COORD_ARRAY );
    glDisableClientState( GL_NORMAL_ARRAY );
    glDisableClientState( GL_VERTEX_ARRAY );
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

        renderer_material.shader().enable();
        Vector3f sun_direction( 0.0f, 1.0f, 1.0f );
        gmtl::normalize( sun_direction );
        renderer_material.shader().set_uniform_vec3f( "sun_color", Vector3f( 1.0f, 1.0f, 1.0f ) );
        renderer_material.shader().set_uniform_vec3f( "sun_direction", sun_direction );

        glEnable( GL_TEXTURE_2D );
        glActiveTexture( GL_TEXTURE0 );
        glClientActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, renderer_material.texture().texture_id() );
        renderer_material.shader().set_uniform_int( "texture", 0 );

        vbo.render();

        glBindTexture( GL_TEXTURE_2D, 0 );
        glDisable( GL_TEXTURE_2D );

        // TODO: Use a guard object instead.
        renderer_material.shader().disable();
    }
}

void ChunkRenderer::initialize( const Chunk& chunk )
{
    const BlockFaceV& faces = chunk.get_external_faces();

    typedef std::map<BlockMaterial, BlockVertexV> MaterialVertexMap;

    MaterialVertexMap material_vertices;

    for ( BlockFaceV::const_iterator face_it = faces.begin(); face_it != faces.end(); ++face_it )
    {
        BlockVertexV& v = material_vertices[face_it->material_];

        // TODO: Clean this up
        v.push_back( BlockVertex( face_it->vertices_[2].position_, face_it->normal_, Vector2f( 1.0f, 1.0f ), face_it->vertices_[2].lighting_ ) );
        v.push_back( BlockVertex( face_it->vertices_[1].position_, face_it->normal_, Vector2f( 1.0f, 0.0f ), face_it->vertices_[1].lighting_ ) );
        v.push_back( BlockVertex( face_it->vertices_[0].position_, face_it->normal_, Vector2f( 0.0f, 0.0f ), face_it->vertices_[0].lighting_ ) );

        v.push_back( BlockVertex( face_it->vertices_[0].position_, face_it->normal_, Vector2f( 0.0f, 0.0f ), face_it->vertices_[0].lighting_ ) );
        v.push_back( BlockVertex( face_it->vertices_[3].position_, face_it->normal_, Vector2f( 0.0f, 1.0f ), face_it->vertices_[3].lighting_ ) );
        v.push_back( BlockVertex( face_it->vertices_[2].position_, face_it->normal_, Vector2f( 1.0f, 1.0f ), face_it->vertices_[2].lighting_ ) );
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

void Renderer::render_chunks( const ChunkMap& chunks )
{
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

    glEnable( GL_CULL_FACE );

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    // glEnable( GL_BLEND );
    // glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    // const GLfloat fog_color[] = { 0.81f, 0.89f, 0.89f, 1.0f };
    // glEnable( GL_FOG );
    // glFogi( GL_FOG_MODE, GL_LINEAR );
    // glFogfv( GL_FOG_COLOR, fog_color );
    // glFogf( GL_FOG_DENSITY, 1.0f );
    // glHint( GL_FOG_HINT, GL_NICEST );
    // glFogf( GL_FOG_START, 0.0f );
    // glFogf( GL_FOG_END, 1000.0f );
    // glFogi( GL_FOG_COORD_SRC, GL_FRAGMENT_DEPTH );

    for ( ChunkMap::const_iterator chunk_it = chunks.begin(); chunk_it != chunks.end(); ++chunk_it )
    {
        const Vector3i chunk_position = chunk_it->first;
        const Vector3f chunk_min = vector_cast<Scalar>( chunk_position );
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

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_BLEND );
    glDisable( GL_CULL_FACE );

    // TODO: Just for development.
    // static unsigned c = 0;
    // if ( ++c % 60 == 0 )
    //     std::cout << "Chunks rendered: " << chunks_rendered << " / " << chunks_total << std::endl;
}

struct SkydomeVertexBuffer : public VertexBuffer
{
    SkydomeVertexBuffer();

    void render();
};

struct SkydomeVertex
{
    SkydomeVertex()
    {
    }

    SkydomeVertex( const Vector3f& position, const Vector3f& color ) :
        x_( position[0] ), y_( position[1] ), z_( position[2] ),
        r_( color[0] ), g_( color[1] ), b_( color[2] )
    {
    }

    GLfloat x_, y_, z_;
    GLfloat r_, g_, b_;

} __attribute__( ( packed ) );

SkydomeVertexBuffer::SkydomeVertexBuffer()
{
    const int
        TESSELATION_BETA = 32,
        TESSELATION_PHI = 32;

    const Scalar RADIUS = 10.0f;

    const Vector3f
        horizon_color( 0.81f, 0.89f, 0.89f ),
        zenith_color( 0.10f, 0.36f, 0.61f );

    std::vector<SkydomeVertex> vertices;

    for ( int i = 0; i < TESSELATION_PHI; ++i )
    {
        Scalar phi = Scalar( i ) / Scalar( TESSELATION_PHI - 1 ) * 2.0 * gmtl::Math::PI;

        for ( int j = 0; j < TESSELATION_BETA; ++j )
        {
            Scalar
                beta_factor = Scalar( j ) / Scalar( TESSELATION_BETA - 1 ),
                beta = beta_factor * gmtl::Math::PI;

            const Vector3f terminus(
                RADIUS * gmtl::Math::sin( beta ) * gmtl::Math::cos( phi ),
                RADIUS * gmtl::Math::cos( beta ),
                RADIUS * gmtl::Math::sin( beta ) * gmtl::Math::sin( phi )
            );

            Vector3f color;
            gmtl::lerp( color, gmtl::Math::cos( beta ), horizon_color, zenith_color );
            vertices.push_back( SkydomeVertex( terminus, color ) );
        }
    }

    bind();
    glBufferData( GL_ARRAY_BUFFER, vertices.size() * sizeof( SkydomeVertex ), &vertices[0], GL_STATIC_DRAW );

    std::vector<GLuint> indices;

    for ( int i = 0; i < TESSELATION_PHI - 1; ++i )
    {
        for ( int j = 0; j < TESSELATION_BETA - 1; ++j )
        {
            GLuint begin_index = i * TESSELATION_BETA + j;

            indices.push_back( begin_index + 1 );
            indices.push_back( begin_index + TESSELATION_BETA );
            indices.push_back( begin_index );

            indices.push_back( begin_index + TESSELATION_BETA + 1 );
            indices.push_back( begin_index + TESSELATION_BETA );
            indices.push_back( begin_index + 1 );
        }
    }

    for ( int i = 0; i < indices.size(); ++i )
    {
        indices[i] %= vertices.size();
    }

    num_elements_ = indices.size();
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof( GLuint ), &indices[0], GL_STATIC_DRAW );
}

void SkydomeVertexBuffer::render()
{
    bind();

    glEnableClientState( GL_VERTEX_ARRAY );
    glVertexPointer( 3, GL_FLOAT, sizeof( SkydomeVertex ), reinterpret_cast<void*>( 0 ) );

    glEnableClientState( GL_COLOR_ARRAY );
    glColorPointer( 3, GL_FLOAT, sizeof( SkydomeVertex ), reinterpret_cast<void*>( 12 ) );

    glDrawElements( GL_TRIANGLES, num_elements_, GL_UNSIGNED_INT, 0 );
    glDisableClientState( GL_COLOR_ARRAY );
    glDisableClientState( GL_VERTEX_ARRAY );
}

void Renderer::render_skydome()
{
    SkydomeVertexBuffer skydome_vbo;
    skydome_vbo.render();
}
