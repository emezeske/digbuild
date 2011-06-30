///////////////////////////////////////////////////////////////////////////
// Copyright 2011 Evan Mezeske.
//
// This file is part of Digbuild.
// 
// Digbuild is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.
// 
// Digbuild is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Digbuild.  If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////

#define GL_GLEXT_PROTOTYPES

#include <GL/glew.h>

#include <boost/numeric/conversion/cast.hpp>
#include <boost/foreach.hpp>

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

    static void configure_vertex_pointer()
    {
        glVertexPointer( 3, GL_FLOAT, sizeof( SimplePositionVertex ), reinterpret_cast<void*>( 0 ) );
    }

    GLfloat x_, y_, z_;

} __attribute__( ( packed ) );

typedef std::vector<SimplePositionVertex> SimplePositionVertexV;

template <typename T>
void set_buffer_data( const GLenum target, const std::vector<T>& vertices, const GLenum usage )
{
    glBufferData( target, vertices.size() * sizeof( T ), &vertices[0], usage );
}

bool face_material_order( const BlockFace& a, const BlockFace& b )
{
    return a.material_ < b.material_;
}

} // anonymous namespace

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for VertexBuffer::BindGuard:
//////////////////////////////////////////////////////////////////////////////////

VertexBuffer::BindGuard::BindGuard( VertexBuffer& vbo ) :
    vbo_( vbo )
{
    vbo_.bind();
}

VertexBuffer::BindGuard::~BindGuard()
{
    vbo_.unbind();
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for VertexBuffer::ClientStateGuard:
//////////////////////////////////////////////////////////////////////////////////

VertexBuffer::ClientStateGuard::ClientStateGuard( const GLenum state ) :
    state_( state )
{
    glEnableClientState( state_ );
}

VertexBuffer::ClientStateGuard::~ClientStateGuard()
{
    glDisableClientState( state_ );
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for VertexBuffer::TextureStateGuard:
//////////////////////////////////////////////////////////////////////////////////

VertexBuffer::TextureStateGuard::TextureStateGuard( const GLenum texture_unit, const GLenum state ) :
    texture_unit_( texture_unit ),
    state_( state )
{
    glClientActiveTexture( texture_unit_ );
    glEnableClientState( state_ );
}

VertexBuffer::TextureStateGuard::~TextureStateGuard()
{
    glClientActiveTexture( texture_unit_ );
    glDisableClientState( state_ );
}

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

void VertexBuffer::draw_elements()
{
    glDrawElements( GL_TRIANGLES, num_elements_, GL_UNSIGNED_INT, 0 );
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for ChunkVertexBuffer:
//////////////////////////////////////////////////////////////////////////////////

ChunkVertexBuffer::ChunkVertexBuffer( const BlockVertexV& vertices, const GLenum vertex_usage, const GLenum index_usage )
{
    assert( vertices.size() > 0 );

    std::vector<VertexBuffer::Index> indices;
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

    BindGuard bind_guard( *this );
    set_buffer_data( GL_ARRAY_BUFFER, vertices, vertex_usage );
    set_buffer_data( GL_ELEMENT_ARRAY_BUFFER, indices, index_usage );
    num_elements_ = indices.size();
}

void ChunkVertexBuffer::render()
{
    BindGuard bind_guard( *this );
    render_no_bind();
}

void ChunkVertexBuffer::render_no_bind()
{
    ClientStateGuard vertex_array_guard( GL_VERTEX_ARRAY );
    glVertexPointer( 3, GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 0 ) );

    ClientStateGuard normal_array_guard( GL_NORMAL_ARRAY );
    glNormalPointer( GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 12 ) );

    TextureStateGuard texture_coord_array_guard0( GL_TEXTURE0, GL_TEXTURE_COORD_ARRAY );
    glTexCoordPointer( 3, GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 24 ) );

    TextureStateGuard texture_coord_array_guard1( GL_TEXTURE1, GL_TEXTURE_COORD_ARRAY );
    glTexCoordPointer( 3, GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 36 ) );

    TextureStateGuard texture_coord_array_guard2( GL_TEXTURE2, GL_TEXTURE_COORD_ARRAY );
    glTexCoordPointer( 3, GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 48 ) );

    TextureStateGuard texture_coord_array_guard3( GL_TEXTURE3, GL_TEXTURE_COORD_ARRAY );
    glTexCoordPointer( 3, GL_FLOAT, sizeof( BlockVertex ), reinterpret_cast<void*>( 60 ) );

    draw_elements();
}

//////////////////////////////////////////////////////////////////////////////////
// Static constant definitions for SortableChunkVertexBuffer:
//////////////////////////////////////////////////////////////////////////////////

const unsigned SortableChunkVertexBuffer::VERTICES_PER_FACE;

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for SortableChunkVertexBuffer:
//////////////////////////////////////////////////////////////////////////////////

SortableChunkVertexBuffer::SortableChunkVertexBuffer( const BlockVertexV& vertices ) :
    ChunkVertexBuffer( vertices, GL_STATIC_DRAW, GL_DYNAMIC_DRAW )
{
    assert( vertices.size() > 0 );
    assert( vertices.size() % VERTICES_PER_FACE == 0 );

    for ( size_t i = 0; i < vertices.size(); i += VERTICES_PER_FACE )
    {
        Vector3f centroid;

        for ( size_t j = 0; j < VERTICES_PER_FACE; ++j )
        {
            const BlockVertex& v = vertices[i + j];
            centroid += Vector3f( v.x_, v.y_, v.z_ );
        }

        centroid /= VERTICES_PER_FACE;

        // Nudge the centroid slightly toward the center of the Block, so that
        // neighboring Blocks with different translucent materials won't Z fight.
        const BlockVertex& v = vertices[i];
        centroid -= 0.1f * Vector3f( v.nx_, v.ny_, v.nz_ );
        centroids_.push_back( centroid );
    }
}

void SortableChunkVertexBuffer::render( const Camera& camera )
{
    DistanceIndexV distance_indices;
    distance_indices.reserve( centroids_.size() );

    // Since these faces are translucent, they must be rendered strictly in back to front order.

    for ( unsigned i = 0; i < centroids_.size(); ++i )
    {
        const Vector3f camera_to_centroid = camera.get_position() - centroids_[i];
        const Scalar distance_squared = gmtl::lengthSquared( camera_to_centroid );
        distance_indices.push_back( std::make_pair( distance_squared, i ) );
    }

    std::sort( distance_indices.begin(), distance_indices.end() );
    render_sorted( distance_indices );
}

void SortableChunkVertexBuffer::render_sorted( const DistanceIndexV distance_indices )
{
    BindGuard bind_guard( *this );
    IndexV indices;
    indices.reserve( distance_indices.size() * 6 );

    BOOST_REVERSE_FOREACH( const DistanceIndex& distance_index, distance_indices )
    {
        const unsigned face_index = distance_index.second;
        const VertexBuffer::Index vertex_index = face_index * VERTICES_PER_FACE;

        indices.push_back( vertex_index + 0 );
        indices.push_back( vertex_index + 3 );
        indices.push_back( vertex_index + 2 );

        indices.push_back( vertex_index + 0 );
        indices.push_back( vertex_index + 2 );
        indices.push_back( vertex_index + 1 );
    }

    set_buffer_data( GL_ELEMENT_ARRAY_BUFFER, indices, GL_DYNAMIC_DRAW );
    num_elements_ = indices.size();
    render_no_bind();
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for AABoxVertexBuffer:
//////////////////////////////////////////////////////////////////////////////////

AABoxVertexBuffer::AABoxVertexBuffer( const AABoxf& aabb )
{
    const Vector3f& min = aabb.getMin();
    const Vector3f& max = aabb.getMax();

    const SimplePositionVertex vertices[] =
    {
        SimplePositionVertex( Vector3f( min[0], min[1], min[2] ) ),
        SimplePositionVertex( Vector3f( max[0], min[1], min[2] ) ),
        SimplePositionVertex( Vector3f( max[0], max[1], min[2] ) ),
        SimplePositionVertex( Vector3f( min[0], max[1], min[2] ) ),
        SimplePositionVertex( Vector3f( min[0], min[1], max[2] ) ),
        SimplePositionVertex( Vector3f( max[0], min[1], max[2] ) ),
        SimplePositionVertex( Vector3f( max[0], max[1], max[2] ) ),
        SimplePositionVertex( Vector3f( min[0], max[1], max[2] ) )
    };

    const VertexBuffer::Index indices[] =
    {
        0, 2, 1, 0, 3, 2,
        5, 7, 4, 5, 6, 7,
        1, 6, 5, 1, 2, 6,
        4, 3, 0, 4, 7, 3,
        3, 6, 2, 3, 7, 6,
        4, 1, 5, 4, 0, 1
    };

    BindGuard bind_guard( *this );
    glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW );
    num_elements_ = sizeof( indices ) / sizeof( VertexBuffer::Index );
}

void AABoxVertexBuffer::render()
{
    BindGuard bind_guard( *this );
    ClientStateGuard vertex_array_guard( GL_VERTEX_ARRAY );
    SimplePositionVertex::configure_vertex_pointer();
    draw_elements();
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for ChunkRenderer:
//////////////////////////////////////////////////////////////////////////////////

ChunkRenderer::ChunkRenderer( const Vector3f& centroid, const AABoxf& aabb ) :
    aabb_vbo_( aabb ),
    centroid_( centroid ),
    aabb_( aabb ),
    num_triangles_( 0 )
{
}

void ChunkRenderer::render_opaque()
{
    if ( opaque_vbo_ )
    {
        opaque_vbo_->render();
    }
}

void ChunkRenderer::render_translucent( const Camera& camera )
{
    if ( translucent_vbo_ )
    {
        translucent_vbo_->render( camera );
    }
}

void ChunkRenderer::render_aabb()
{
    aabb_vbo_.render();
}

void ChunkRenderer::rebuild( const Chunk& chunk )
{
    BlockFaceV faces = chunk.get_external_faces();
    num_triangles_ = faces.size() * 2; // Two triangles per (square) face.

    BlockVertexV
        opaque_vertices,
        translucent_vertices;

    // Although each vertex specifies its own texture ID, and thus the faces can be drawn in
    // any order, it makes sense to group them together by texture, under the assumption that
    // this will be more friendly to the GPU's texture cache.

    std::sort( faces.begin(), faces.end(), face_material_order );

    BOOST_FOREACH( const BlockFace& face, faces )
    {
        const BlockMaterial material = face.material_;

        if ( get_block_material_attributes( material ).translucent_ )
        {
            get_vertices_for_face( face, translucent_vertices );
        }
        else get_vertices_for_face( face, opaque_vertices );
    }

    if ( !opaque_vertices.empty() )
    {
        opaque_vbo_.reset( new ChunkVertexBuffer( opaque_vertices ) );
    }
    else opaque_vbo_.reset();

    if ( !translucent_vertices.empty() )
    {
        translucent_vbo_.reset( new SortableChunkVertexBuffer( translucent_vertices ) );
    }
    else translucent_vbo_.reset();
}

void ChunkRenderer::get_vertices_for_face( const BlockFace& face, BlockVertexV& vertices ) const
{
    const BlockFace::Vertex* v = face.vertices_;
    const Vector3f& n = face.normal_;
    const Vector3f& t = face.tangent_;
    const Scalar m = face.material_;

    vertices.push_back( BlockVertex( v[0].position_, n, t, Vector3f( 0.0f, 0.0f, m ), v[0].lighting_, v[0].sunlighting_ ) );
    vertices.push_back( BlockVertex( v[1].position_, n, t, Vector3f( 1.0f, 0.0f, m ), v[1].lighting_, v[1].sunlighting_ ) );
    vertices.push_back( BlockVertex( v[2].position_, n, t, Vector3f( 1.0f, 1.0f, m ), v[2].lighting_, v[2].sunlighting_ ) );
    vertices.push_back( BlockVertex( v[3].position_, n, t, Vector3f( 0.0f, 1.0f, m ), v[3].lighting_, v[3].sunlighting_ ) );
}

//////////////////////////////////////////////////////////////////////////////////
// Static constant definitions for SkydomeVertexBuffer:
//////////////////////////////////////////////////////////////////////////////////

const Scalar SkydomeVertexBuffer::RADIUS;

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for SkydomeVertexBuffer:
//////////////////////////////////////////////////////////////////////////////////

SkydomeVertexBuffer::SkydomeVertexBuffer()
{
    const int
        TESSELATION_BETA = 32,
        TESSELATION_PHI = 32;

    SimplePositionVertexV vertices;

    for ( int i = 0; i < TESSELATION_PHI; ++i )
    {
        const Scalar phi = Scalar( i ) / Scalar( TESSELATION_PHI - 1 ) * 2.0 * gmtl::Math::PI;

        for ( int j = 0; j < TESSELATION_BETA; ++j )
        {
            const Scalar beta = Scalar( j ) / Scalar( TESSELATION_BETA - 1 ) * gmtl::Math::PI;
            vertices.push_back( SimplePositionVertex( spherical_to_cartesian( Vector3f( RADIUS, beta, phi ) ) ) );
        }
    }

    std::vector<VertexBuffer::Index> indices;

    for ( int i = 0; i < TESSELATION_PHI - 1; ++i )
    {
        for ( int j = 0; j < TESSELATION_BETA - 1; ++j )
        {
            VertexBuffer::Index begin_index = i * TESSELATION_BETA + j;

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

    BindGuard bind_guard( *this );
    set_buffer_data( GL_ARRAY_BUFFER, vertices, GL_STATIC_DRAW );
    set_buffer_data( GL_ELEMENT_ARRAY_BUFFER, indices, GL_STATIC_DRAW );
    num_elements_ = indices.size();
}

void SkydomeVertexBuffer::render()
{
    BindGuard bind_guard( *this );
    ClientStateGuard vertex_array_guard( GL_VERTEX_ARRAY );
    SimplePositionVertex::configure_vertex_pointer();
    draw_elements();
}

//////////////////////////////////////////////////////////////////////////////////
// Static constant definitions for StarVertexBuffer:
//////////////////////////////////////////////////////////////////////////////////

const Scalar StarVertexBuffer::RADIUS;

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for StarVertexBuffer:
//////////////////////////////////////////////////////////////////////////////////

StarVertexBuffer::StarVertexBuffer( const Sky::StarV& stars )
{
    SimplePositionVertexV vertices;
    std::vector<VertexBuffer::Index> indices;

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

    BindGuard bind_guard( *this );
    set_buffer_data( GL_ARRAY_BUFFER, vertices, GL_STATIC_DRAW );
    set_buffer_data( GL_ELEMENT_ARRAY_BUFFER, indices, GL_STATIC_DRAW );
    num_elements_ = indices.size();
}

void StarVertexBuffer::render()
{
    BindGuard bind_guard( *this );
    ClientStateGuard vertex_array_guard( GL_VERTEX_ARRAY );
    SimplePositionVertex::configure_vertex_pointer();
    draw_elements();
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for SkyRenderer:
//////////////////////////////////////////////////////////////////////////////////

SkyRenderer::SkyRenderer() :
    sun_texture_( RendererMaterialManager::TEXTURE_DIRECTORY + "/sun.png" ),
    moon_texture_( RendererMaterialManager::TEXTURE_DIRECTORY + "/moon.png" ),
    skydome_shader_( RendererMaterialManager::SHADER_DIRECTORY + "/skydome.vertex.glsl",
                     RendererMaterialManager::SHADER_DIRECTORY + "/skydome.fragment.glsl" )
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

    if ( sky.get_star_intensity() > gmtl::GMTL_EPSILON )
    {
        glPushMatrix();
            rotate_sky( sky.get_moon_angle() );

            if ( !star_vbo_ )
            {
                star_vbo_.reset( new StarVertexBuffer( sky.get_stars() ) );
            }

            glColor4f( 1.0f, 1.0f, 1.0f, sky.get_star_intensity() );
            star_vbo_->render();
        glPopMatrix();
    }

    glPushMatrix();
        rotate_sky( sky.get_sun_angle() );
        render_celestial_body( sun_texture_.get_texture_id(), sky.get_sun_color() );
    glPopMatrix();

    glPushMatrix();
        rotate_sky( sky.get_moon_angle() );
        render_celestial_body( moon_texture_.get_texture_id(), sky.get_moon_color() );
    glPopMatrix();

    glDisable( GL_BLEND );
    glDisable( GL_TEXTURE_2D );
}

void SkyRenderer::rotate_sky( const Vector2f& angle ) const
{
    glRotatef( 180.0f * angle[1] / gmtl::Math::PI, 0.0f, 1.0f, 0.0f );
    glRotatef( -90.0f + 180.0f * angle[0] / gmtl::Math::PI, 1.0f, 0.0f, 0.0f );
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

Renderer::Renderer() :
    num_chunks_drawn_( 0 ),
    num_triangles_drawn_( 0 )
{
}

void Renderer::note_chunk_changes( const Chunk& chunk )
{
    ChunkRendererMap::iterator chunk_renderer_it = chunk_renderers_.find( chunk.get_position() );

    if ( chunk_renderer_it == chunk_renderers_.end() )
    {
        if ( !chunk.get_external_faces().empty() )
        {
            const Vector3f centroid =
                vector_cast<Scalar>( chunk.get_position() ) +
                vector_cast<Scalar>( Chunk::SIZE ) / 2.0f;

            const Vector3f chunk_min = vector_cast<Scalar>( chunk.get_position() );
            const Vector3f chunk_max = chunk_min + vector_cast<Scalar>( Chunk::SIZE );
            const AABoxf aabb( chunk_min, chunk_max );

            ChunkRendererSP renderer( new ChunkRenderer( centroid, aabb ) );
            renderer->rebuild( chunk );
            chunk_renderers_.insert( std::make_pair( chunk.get_position(), renderer ) );
        }
    }
    else if ( chunk.get_external_faces().empty() )
    {
        chunk_renderers_.erase( chunk_renderer_it );
    }
    else chunk_renderer_it->second->rebuild( chunk );
}

#ifdef DEBUG_COLLISIONS
void Renderer::render( const SDL_GL_Window& window, const Camera& camera, const World& world, const Player& player )
#else
void Renderer::render( const SDL_GL_Window& window, const Camera& camera, const World& world )
#endif
{
    glClear( GL_DEPTH_BUFFER_BIT );

    glPushMatrix();
        camera.rotate();
        render_sky( world.get_sky() );
        camera.translate();
        render_chunks( camera, world.get_sky() );
#ifdef DEBUG_COLLISIONS
        render_collisions( player );
#endif

    glPopMatrix();

    render_crosshairs( window );
}

void Renderer::render_sky( const Sky& sky )
{
    sky_renderer_.render( sky );
}

void Renderer::render_chunks( const Camera& camera, const Sky& sky )
{
    // TODO: Decompose this function.

    gmtl::Frustumf view_frustum(
        get_opengl_matrix( GL_MODELVIEW_MATRIX ),
        get_opengl_matrix( GL_PROJECTION_MATRIX )
    );

    typedef std::pair<Scalar, ChunkRenderer*> DistanceChunkPair;
    typedef std::vector<DistanceChunkPair> DistanceChunkPairV;

    DistanceChunkPairV
        opaque_chunks,
        translucent_chunks;

#ifdef DEBUG_CHUNKS
    DistanceChunkPairV debug_chunks;
#endif

    num_chunks_drawn_ = 0;
    num_triangles_drawn_ = 0;

    BOOST_FOREACH( const ChunkRendererMap::value_type& chunk_renderer_it, chunk_renderers_ )
    {
        ChunkRenderer& chunk_renderer = *chunk_renderer_it.second.get();

        // TODO: Arrange the chunks into some kind of hierarchy and cull based on that.
        if ( gmtl::isInVolume( view_frustum, chunk_renderer.get_aabb() ) )
        {
            const Vector3f camera_to_centroid = camera.get_position() - chunk_renderer.get_centroid();
            const Scalar distance_squared = gmtl::lengthSquared( camera_to_centroid );
            const DistanceChunkPair distance_chunk = std::make_pair( distance_squared, &chunk_renderer );

            opaque_chunks.push_back( distance_chunk );

            if ( chunk_renderer.has_translucent_materials() )
            {
                translucent_chunks.push_back( distance_chunk );
            }

#ifdef DEBUG_CHUNKS
            debug_chunks.push_back( distance_chunk );
#endif
            ++num_chunks_drawn_;
            num_triangles_drawn_ += chunk_renderer.get_num_triangles();
        }
    }

    material_manager_.configure_materials( camera, sky );

    glEnable( GL_CULL_FACE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    // The opaque materials are rendered first, in front-to-back order.  This results in
    // many of the farthest chunks being fully occluded, and thus their fragments will be
    // rejected without running any expensive fragment shaders.

    std::sort( opaque_chunks.begin(), opaque_chunks.end() );

    BOOST_FOREACH( const DistanceChunkPair& it, opaque_chunks )
    {
        it.second->render_opaque();
    }

    glDisable( GL_CULL_FACE );
    glDepthMask( GL_FALSE );

    // Now draw the translucent parts of the Chunks from back to front.

    std::sort( translucent_chunks.begin(), translucent_chunks.end() );

    BOOST_REVERSE_FOREACH( const DistanceChunkPair& it, translucent_chunks )
    {
        it.second->render_translucent( camera );
    }

    material_manager_.deconfigure_materials();

#ifdef DEBUG_CHUNKS
    glEnable( GL_BLEND );
    glEnable( GL_CULL_FACE );
    glEnable( GL_POLYGON_OFFSET_FILL );
    glPolygonOffset( 1.0f, 3.0f );
    glColor4f( 1.0f, 0.0f, 0.0f, 0.3f );

    std::sort( debug_chunks.begin(), debug_chunks.end() );

    BOOST_REVERSE_FOREACH( const DistanceChunkPair& it, debug_chunks )
    {
        it.second->render_aabb();
    }

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    glDisable( GL_POLYGON_OFFSET_FILL );
    glDisable( GL_CULL_FACE );
    glDisable( GL_BLEND );
#endif

    glDepthMask( GL_TRUE );
    glDisable( GL_DEPTH_TEST );
}

#ifdef DEBUG_COLLISIONS
void Renderer::render_collisions( const Player& player )
{
    BOOST_FOREACH( const Player::DebugCollision& collision, player.debug_collisions_ )
    {
        AABoxVertexBuffer
            obstructing_block_vbo( AABoxf( collision.block_position_, collision.block_position_ + Block::SIZE ) );

        glColor3f( 1.0f, 0.0f, 0.0f );
        obstructing_block_vbo.render();

        glColor3f( 0.0f, 1.0f, 0.0f );
        glPushMatrix();
            const Vector3f relation = vector_cast<Scalar>( cardinal_relation_vector( collision.block_face_ ) );
            const Vector3f face_center = collision.block_position_ + Block::SIZE * 0.5f + relation * 0.5f;
            const unsigned major = major_axis( relation );
            const Vector4f a( -0.5f, -0.5f, 0.5f, 0.5f );
            const Vector4f b( -0.5f, 0.5f, -0.5f, 0.5f );
            Vector4f x, y, z;
            switch ( major )
            {
                case 0: y = a; z = b; break;
                case 1: x = a; z = b; break;
                case 2: x = a; y = b; break;
            }
            glTranslatef( face_center[0], face_center[1], face_center[2] );
            glBegin( GL_TRIANGLE_STRIP );
                glVertex3f( x[0], y[0], z[0] );
                glVertex3f( x[1], y[1], z[1] );
                glVertex3f( x[2], y[2], z[2] );
                glVertex3f( x[3], y[3], z[3] );
            glEnd();
        glPopMatrix();
    }

    glEnable( GL_DEPTH_TEST );
    glColor3f( 0.0f, 0.0f, 1.0f );
    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    AABoxVertexBuffer player_vbo( player.get_aabb() );
    player_vbo.render();
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    glColor3f( 1.0f, 1.0f, 1.0f );
    glDisable( GL_DEPTH_TEST );
}
#endif

void Renderer::render_crosshairs( const SDL_GL_Window& window )
{
    const Vector2i
        size = window.get_resolution(),
        center = size / 2;

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho( 0, size[0], 0, size[1], 0, 1 );

    glEnable( GL_COLOR_LOGIC_OP );
    glLogicOp( GL_XOR );

    const int
        HALF_WIDTH = 1,
        HALF_LENGTH = 12;

    // The vertical hair.
    glBegin( GL_TRIANGLE_STRIP );
        glVertex2i( center[0] - HALF_WIDTH, center[1] - HALF_LENGTH );
        glVertex2i( center[0] - HALF_WIDTH, center[1] + HALF_LENGTH );
        glVertex2i( center[0] + HALF_WIDTH, center[1] - HALF_LENGTH );
        glVertex2i( center[0] + HALF_WIDTH, center[1] + HALF_LENGTH );
    glEnd();

    // The left part of the horizontal hair (the horizontal hair is split into
    // two parts that don't overlap with the vertical hair).
    glBegin( GL_TRIANGLE_STRIP );
        glVertex2i( center[0] - HALF_LENGTH, center[1] - HALF_WIDTH );
        glVertex2i( center[0] - HALF_LENGTH, center[1] + HALF_WIDTH );
        glVertex2i( center[0] -  HALF_WIDTH, center[1] - HALF_WIDTH );
        glVertex2i( center[0] -  HALF_WIDTH, center[1] + HALF_WIDTH );
    glEnd();

    // The right part of the horizontal hair.
    glBegin( GL_TRIANGLE_STRIP );
        glVertex2i( center[0] +  HALF_WIDTH, center[1] - HALF_WIDTH );
        glVertex2i( center[0] +  HALF_WIDTH, center[1] + HALF_WIDTH );
        glVertex2i( center[0] + HALF_LENGTH, center[1] - HALF_WIDTH );
        glVertex2i( center[0] + HALF_LENGTH, center[1] + HALF_WIDTH );
    glEnd();

    glDisable( GL_COLOR_LOGIC_OP );
}

gmtl::Matrix44f Renderer::get_opengl_matrix( const GLenum matrix )
{
    GLfloat m_data[16];
    glGetFloatv( matrix, m_data );
    gmtl::Matrix44f m;
    m.set( m_data );
    return m;
}
