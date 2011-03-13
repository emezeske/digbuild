#define GL_GLEXT_PROTOTYPES

#include <GL/glew.h>

#include <set>

#include <boost/numeric/conversion/cast.hpp>

#include "renderer.h"

//////////////////////////////////////////////////////////////////////////////////
// Local definitions:
//////////////////////////////////////////////////////////////////////////////////

namespace {

struct SimplePositionVertex
{
    SimplePositionVertex( const Vector3f& position = Vector3f() ) :
        x_( position[0] ), y_( position[1] ), z_( position[2] )
    {
    }

    GLfloat x_, y_, z_;

} __attribute__( ( packed ) );

Vector3f spherical_to_cartesian( const Vector3f& spherical )
{
    return Vector3f(
        spherical[0] * gmtl::Math::sin( spherical[1] ) * gmtl::Math::sin( spherical[2] ),
        spherical[0] * gmtl::Math::cos( spherical[1] ),
        spherical[0] * gmtl::Math::sin( spherical[1] ) * gmtl::Math::cos( spherical[2] )
    );
}

} // anonymous namespace

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

void VertexBuffer::unbind()
{
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for ChunkVertexBuffer:
//////////////////////////////////////////////////////////////////////////////////

ChunkVertexBuffer::ChunkVertexBuffer( const BlockVertexV& vertices )
{
    assert( vertices.size() > 0 );

    std::vector<GLuint> indices;
    indices.reserve( vertices.size() + vertices.size() / 2 );

    for ( int i = 0; i < int( vertices.size() ); i += 4 )
    {
        indices.push_back( i + 0 );
        indices.push_back( i + 3 );
        indices.push_back( i + 2 );

        indices.push_back( i + 0 );
        indices.push_back( i + 2 );
        indices.push_back( i + 1 );
    }

    bind();
    glBufferData( GL_ARRAY_BUFFER, vertices.size() * sizeof( BlockVertex ), &vertices[0], GL_STATIC_DRAW );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof( GLuint ), &indices[0], GL_STATIC_DRAW );
    num_elements_ = indices.size();
    unbind();
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
    glTexCoordPointer( 3, GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 24 ) );

    glClientActiveTexture( GL_TEXTURE1 );
    glEnableClientState( GL_TEXTURE_COORD_ARRAY );
    glTexCoordPointer( 2, GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 36 ) );

    glClientActiveTexture( GL_TEXTURE2 );
    glEnableClientState( GL_TEXTURE_COORD_ARRAY );
    glTexCoordPointer( 4, GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 44 ) );

    glDrawElements( GL_TRIANGLES, num_elements_, GL_UNSIGNED_INT, 0 );
    glDisableClientState( GL_TEXTURE_COORD_ARRAY );
    glDisableClientState( GL_NORMAL_ARRAY );
    glDisableClientState( GL_VERTEX_ARRAY );

    unbind();
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for ChunkRenderer:
//////////////////////////////////////////////////////////////////////////////////

ChunkRenderer::ChunkRenderer( const Vector3f& centroid ) :
    centroid_( centroid )
{
}

void ChunkRenderer::render_opaque( const Vector3f& camera_position, const Sky& sky, const RendererMaterialV& materials )
{
    for ( ChunkVertexBufferMap::iterator it = vbos_.begin(); it != vbos_.end(); ++it )
    {
        const BlockMaterial material = it->first;
        ChunkVertexBuffer& vbo = *it->second;

        assert( material >= 0 && material < static_cast<int>( materials.size() ) );
        const RendererMaterial& renderer_material = *materials[material];

        renderer_material.shader().enable();

        const Vector3f
            sun_direction = spherical_to_cartesian( Vector3f( 1.0f, sky.get_sun_angle()[0], sky.get_sun_angle()[1] ) ),
            moon_direction = spherical_to_cartesian( Vector3f( 1.0f, sky.get_moon_angle()[0], sky.get_moon_angle()[1] ) );

        renderer_material.shader().set_uniform_vec3f( "camera_position", camera_position );

        renderer_material.shader().set_uniform_vec3f( "sun_direction", sun_direction );
        renderer_material.shader().set_uniform_vec3f( "moon_direction", moon_direction );

        renderer_material.shader().set_uniform_vec3f( "sun_light_color", sky.get_sun_light_color() );
        renderer_material.shader().set_uniform_vec3f( "moon_light_color", sky.get_moon_light_color() );

        glEnable( GL_TEXTURE_2D );

        glActiveTexture( GL_TEXTURE0 );
        glClientActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, renderer_material.texture().texture_id() );
        renderer_material.shader().set_uniform_int( "material_texture", 0 );

        glActiveTexture( GL_TEXTURE1 );
        glClientActiveTexture( GL_TEXTURE1 );
        glBindTexture( GL_TEXTURE_2D, renderer_material.specular_map().texture_id() );
        renderer_material.shader().set_uniform_int( "material_specular_map", 1 );

        glActiveTexture( GL_TEXTURE2 );
        glClientActiveTexture( GL_TEXTURE2 );
        glBindTexture( GL_TEXTURE_2D, renderer_material.bump_map().texture_id() );
        renderer_material.shader().set_uniform_int( "material_bump_map", 2 );

        vbo.render();

        glBindTexture( GL_TEXTURE_2D, 0 );

        glActiveTexture( GL_TEXTURE1 );
        glClientActiveTexture( GL_TEXTURE1 );
        glBindTexture( GL_TEXTURE_2D, 0 );

        glActiveTexture( GL_TEXTURE0 );
        glClientActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, 0 );

        glDisable( GL_TEXTURE_2D );

        renderer_material.shader().disable();
    }
}

void ChunkRenderer::rebuild( const Chunk& chunk )
{
    const BlockFaceV& faces = chunk.get_external_faces();

    typedef std::map<BlockMaterial, BlockVertexV> MaterialVertexMap;

    MaterialVertexMap material_vertices;

    for ( BlockFaceV::const_iterator face_it = faces.begin(); face_it != faces.end(); ++face_it )
    {
        BlockVertexV& v = material_vertices[face_it->material_];

        v.push_back( BlockVertex( face_it->vertices_[0].position_, face_it->normal_, face_it->tangent_, Vector2f( 0.0f, 0.0f ), face_it->vertices_[0].lighting_ ) );
        v.push_back( BlockVertex( face_it->vertices_[1].position_, face_it->normal_, face_it->tangent_, Vector2f( 1.0f, 0.0f ), face_it->vertices_[1].lighting_ ) );
        v.push_back( BlockVertex( face_it->vertices_[2].position_, face_it->normal_, face_it->tangent_, Vector2f( 1.0f, 1.0f ), face_it->vertices_[2].lighting_ ) );
        v.push_back( BlockVertex( face_it->vertices_[3].position_, face_it->normal_, face_it->tangent_, Vector2f( 0.0f, 1.0f ), face_it->vertices_[3].lighting_ ) );
    }

    vbos_.clear();

    for ( MaterialVertexMap::const_iterator it = material_vertices.begin(); it != material_vertices.end(); ++it )
    {
        ChunkVertexBufferSP vbo( new ChunkVertexBuffer( it->second ) );
        vbos_[it->first] = vbo;
    }
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for SkydomeVertexBuffer:
//////////////////////////////////////////////////////////////////////////////////

SkydomeVertexBuffer::SkydomeVertexBuffer()
{
    const int
        TESSELATION_BETA = 32,
        TESSELATION_PHI = 32;

    std::vector<SimplePositionVertex> vertices;

    for ( int i = 0; i < TESSELATION_PHI; ++i )
    {
        const Scalar phi = Scalar( i ) / Scalar( TESSELATION_PHI - 1 ) * 2.0 * gmtl::Math::PI;

        for ( int j = 0; j < TESSELATION_BETA; ++j )
        {
            const Scalar beta = Scalar( j ) / Scalar( TESSELATION_BETA - 1 ) * gmtl::Math::PI;
            vertices.push_back( SimplePositionVertex( spherical_to_cartesian( Vector3f( RADIUS, beta, phi ) ) ) );
        }
    }

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

    for ( size_t i = 0; i < indices.size(); ++i )
    {
        indices[i] %= vertices.size();
    }

    bind();
    glBufferData( GL_ARRAY_BUFFER, vertices.size() * sizeof( SimplePositionVertex ), &vertices[0], GL_STATIC_DRAW );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof( GLuint ), &indices[0], GL_STATIC_DRAW );
    num_elements_ = indices.size();
    unbind();
}

void SkydomeVertexBuffer::render()
{
    bind();
    glEnableClientState( GL_VERTEX_ARRAY );
    glVertexPointer( 3, GL_FLOAT, sizeof( SimplePositionVertex ), reinterpret_cast<void*>( 0 ) );
    glDrawElements( GL_TRIANGLES, num_elements_, GL_UNSIGNED_INT, 0 );
    glDisableClientState( GL_VERTEX_ARRAY );
    unbind();
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for StarVertexBuffer:
//////////////////////////////////////////////////////////////////////////////////

StarVertexBuffer::StarVertexBuffer( const Sky::StarV& stars )
{
    std::vector<SimplePositionVertex> vertices;
    std::vector<GLuint> indices;

    for ( size_t i = 0; i < stars.size(); ++i )
    {
        const Vector3f& star = stars[i];

        const Vector3f
            star_center = spherical_to_cartesian( Vector3f( RADIUS, star[1], star[2] ) );

        Vector3f
            basis_a = spherical_to_cartesian( Vector3f( RADIUS, star[1] - gmtl::Math::PI_OVER_2, star[2] - gmtl::Math::PI_OVER_2 ) ),
            basis_b;

        gmtl::cross( basis_b, basis_a, star_center );
        gmtl::normalize( basis_a );
        gmtl::normalize( basis_b );

        #define V( da, db ) vertices.push_back( SimplePositionVertex( star_center + da * basis_a + db * basis_b ) )
        V( 0.0f,    star[0] );
        V( star[0], star[0] );
        V( 0.0f,    0.0f    );
        V( star[0], 0.0f    );
        #undef V

        const size_t index = i * 4;
        indices.push_back( index + 0 );
        indices.push_back( index + 1 );
        indices.push_back( index + 2 );
        indices.push_back( index + 2 );
        indices.push_back( index + 1 );
        indices.push_back( index + 3 );
    }

    bind();
    glBufferData( GL_ARRAY_BUFFER, vertices.size() * sizeof( SimplePositionVertex ), &vertices[0], GL_STATIC_DRAW );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof( GLuint ), &indices[0], GL_STATIC_DRAW );
    num_elements_ = indices.size();
    unbind();
}

void StarVertexBuffer::render()
{
    bind();
    glEnableClientState( GL_VERTEX_ARRAY );
    glVertexPointer( 3, GL_FLOAT, sizeof( SimplePositionVertex ), reinterpret_cast<void*>( 0 ) );
    glDrawElements( GL_TRIANGLES, num_elements_, GL_UNSIGNED_INT, 0 );
    glDisableClientState( GL_VERTEX_ARRAY );
    unbind();
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for SkyRenderer:
//////////////////////////////////////////////////////////////////////////////////

SkyRenderer::SkyRenderer() :
    sun_texture_( RendererMaterial::TEXTURE_DIRECTORY + "/sun.png" ),
    moon_texture_( RendererMaterial::TEXTURE_DIRECTORY + "/moon.png" ),
    skydome_shader_( RendererMaterial::SHADER_DIRECTORY + "/skydome.vertex.glsl",
                     RendererMaterial::SHADER_DIRECTORY + "/skydome.fragment.glsl" )
{
}

void SkyRenderer::render( const Sky& sky )
{
    skydome_shader_.enable();
    skydome_shader_.set_uniform_float( "skydome_radius", SkydomeVertexBuffer::RADIUS );
    skydome_shader_.set_uniform_vec3f( "zenith_color", sky.get_zenith_color() );
    skydome_shader_.set_uniform_vec3f( "horizon_color", sky.get_horizon_color() );
    skydome_vbo_.render();
    skydome_shader_.disable();

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable( GL_TEXTURE_2D );

    glPushMatrix();
        rotate_sky( sky.get_moon_angle() );
        if ( sky.get_star_intensity() > gmtl::GMTL_EPSILON )
        {
            if ( !star_vbo_ )
            {
                star_vbo_.reset( new StarVertexBuffer( sky.get_stars() ) );
            }

            glColor4f( 1.0f, 1.0f, 1.0f, sky.get_star_intensity() );
            star_vbo_->render();
        }
        render_celestial_body( moon_texture_.texture_id(), sky.get_moon_color() );
    glPopMatrix();

    glPushMatrix();
        rotate_sky( sky.get_sun_angle() );
        render_celestial_body( sun_texture_.texture_id(), sky.get_sun_color() );
    glPopMatrix();

    glDisable( GL_BLEND );
    glDisable( GL_TEXTURE_2D );
}

void SkyRenderer::rotate_sky( const Vector2f& angle ) const
{
    glRotatef( 180 * angle[1] / gmtl::Math::PI, 0.0f, 1.0f, 0.0f );
    glRotatef( -90 + 180 * angle[0] / gmtl::Math::PI, 1.0f, 0.0f, 0.0f );
}

void SkyRenderer::render_celestial_body( const GLuint texture_id, const Vector3f& color ) const
{
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, texture_id);
    glColor3f( color[0], color[1], color[2] );
    glBegin( GL_TRIANGLE_STRIP );
        glTexCoord2f( 0.0f, 0.0f );
        glVertex3f( -0.5f, -0.5f, 3.0f );
        glTexCoord2f( 0.0f, 1.0f );
        glVertex3f( -0.5f, 0.5f, 3.0f );
        glTexCoord2f( 1.0f, 0.0f );
        glVertex3f( 0.5f, -0.5f, 3.0f );
        glTexCoord2f( 1.0f, 1.0f );
        glVertex3f( 0.5f, 0.5f, 3.0f );
    glEnd();
    glBindTexture( GL_TEXTURE_2D, 0 );
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
    materials_[BLOCK_MATERIAL_TREE_TRUNK].reset( new RendererMaterial( "tree-trunk" ) );
    materials_[BLOCK_MATERIAL_TREE_LEAF].reset( new RendererMaterial( "tree-leaf" ) );
    materials_[BLOCK_MATERIAL_MAGMA].reset  ( new RendererMaterial( "magma" ) );
}

void Renderer::note_chunk_changes( const Chunk& chunk )
{
    ChunkRendererMap::iterator chunk_renderer_it = chunk_renderers_.find( chunk.get_position() );

    if ( chunk_renderer_it == chunk_renderers_.end() )
    {
        const Vector3f centroid = vector_cast<Scalar>( chunk.get_position() ) + Vector3f(
            Scalar( Chunk::CHUNK_SIZE ) / 2.0f,
            Scalar( Chunk::CHUNK_SIZE ) / 2.0f,
            Scalar( Chunk::CHUNK_SIZE ) / 2.0f
        );

        chunk_renderer_it =
            chunk_renderers_.insert( std::make_pair( chunk.get_position(), ChunkRenderer( centroid ) ) ).first;
    }

    chunk_renderer_it->second.rebuild( chunk );
}

void Renderer::render( const Camera& camera, const World& world )
{
    glPushMatrix();
        camera.rotate();
        render_sky( world.get_sky() );
        camera.translate();
        render_chunks( camera.get_position(), world.get_sky(), world.get_chunks() );
    glPopMatrix();
}

void Renderer::render_sky( const Sky& sky )
{
    sky_renderer_.render( sky );
}

void Renderer::render_chunks( const Vector3f& camera_position, const Sky& sky, const ChunkMap& chunks )
{
    // TODO: Arrange the chunks into some kind of hierarchy and cull based on that.

    // TODO: Look into glMultiDrawElements or display lists to reduce the number of OpenGL library calls.

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

    gmtl::Frustumf view_frustum(
        get_opengl_matrix( GL_MODELVIEW_MATRIX ),
        get_opengl_matrix( GL_PROJECTION_MATRIX )
    );

    typedef std::pair<Scalar, ChunkRenderer*> DistanceChunkPair;
    typedef std::set<DistanceChunkPair> ChunkRendererSet;
    ChunkRendererSet visible_chunks;

    for ( ChunkMap::const_iterator chunk_it = chunks.begin(); chunk_it != chunks.end(); ++chunk_it )
    {
        const Vector3i chunk_position = chunk_it->first;
        const Vector3f chunk_min = vector_cast<Scalar>( chunk_position );
        const Vector3f chunk_max = chunk_min + Vector3f( Chunk::CHUNK_SIZE, Chunk::CHUNK_SIZE, Chunk::CHUNK_SIZE );
        const gmtl::AABoxf chunk_box( chunk_min, chunk_max );

        if ( gmtl::isInVolume( view_frustum, chunk_box ) )
        {
            ChunkRendererMap::iterator chunk_renderer_it = chunk_renderers_.find( chunk_position );

            if ( chunk_renderer_it != chunk_renderers_.end() )
            {
                const Vector3f camera_to_centroid =
                    camera_position - chunk_renderer_it->second.get_centroid();
                const Scalar distance_squared =
                    gmtl::dot( camera_to_centroid, camera_to_centroid );

                visible_chunks.insert( std::make_pair( distance_squared, &chunk_renderer_it->second ) );
            }
        }
    }

    // TODO: Just for development.
    // static unsigned c = 0;
    // if ( ++c % 60 == 0 )
    //     std::cout << "Visible chunks: " << visible_chunks.size() << " / " << chunks.size() << std::endl;

    glEnable( GL_CULL_FACE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    // Draw the Chunks from nearest to farthest.  This will result in many of the farthest
    // chunks being fully occluded, and thus their fragments will be rejected without running
    // any expensive fragment shaders.
    //
    // TODO: Use an ARB_occlusion_query to avoid rendering fully occluded chunks.

    for ( ChunkRendererSet::iterator it = visible_chunks.begin(); it != visible_chunks.end(); ++it )
    {
        it->second->render_opaque( camera_position, sky, materials_ );
    }

    // TODO: Render the translucent parts of the chunks from back to front.

    // for ( ChunkRendererSet::reverse_iterator it = visible_chunks.rbegin(); it != visible_chunks.rend(); ++it )
    // {
    //     it->second->render_translucent( sky, materials_ );
    // }

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_BLEND );
    glDisable( GL_CULL_FACE );
}

gmtl::Matrix44f Renderer::get_opengl_matrix( const GLenum matrix )
{
    GLfloat m_data[16];
    glGetFloatv( matrix, m_data );
    gmtl::Matrix44f m;
    m.set( m_data );
    return m;
}
