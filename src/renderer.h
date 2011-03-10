#ifndef RENDERER_H
#define RENDERER_H

#include <GL/glew.h>

#include <boost/utility.hpp>

#include "camera.h"
#include "world.h"
#include "renderer_material.h"

struct VertexBuffer : public boost::noncopyable
{
    VertexBuffer( const GLsizei num_elements = 0 );
    virtual ~VertexBuffer();

    void bind();

    GLuint
        vbo_id_,
        ibo_id_;

    GLsizei num_elements_;
};

struct BlockVertex
{
    BlockVertex()
    {
    }

    BlockVertex( const Vector3f& position, const Vector3f& normal, const Vector2f& texcoords, const Vector4f& lighting ) :
        x_( position[0] ), y_( position[1] ), z_( position[2] ),
        nx_( normal[0] ), ny_( normal[1] ), nz_( normal[2] ),
        s_( texcoords[0] ), t_( texcoords[1] ),
        lr_( lighting[0] ), lg_( lighting[1] ), lb_( lighting[2] ), ls_( lighting[3] )
    {
    }

    GLfloat x_, y_, z_;
    GLfloat nx_, ny_, nz_;
    GLfloat s_, t_;
    GLfloat lr_, lg_, lb_, ls_;

} __attribute__( ( packed ) );

typedef std::vector<BlockVertex> BlockVertexV;

struct ChunkVertexBuffer : public VertexBuffer
{
    ChunkVertexBuffer( const BlockVertexV& vertices );

    void render();
};

typedef boost::shared_ptr<ChunkVertexBuffer> ChunkVertexBufferSP;
typedef std::map<BlockMaterial, ChunkVertexBufferSP> ChunkVertexBufferMap;

struct ChunkRenderer
{
    ChunkRenderer( const Vector3f& centroid = Vector3f() );

    void render_opaque( const Sky& sky, const RendererMaterialV& materials );
    void rebuild( const Chunk& chunk );
    const Vector3f& get_centroid() const { return centroid_; }

protected:

    ChunkVertexBufferMap vbos_;

    Vector3f centroid_;
};

struct SkydomeVertexBuffer : public VertexBuffer
{
    static const Scalar RADIUS = 10.0f;

    SkydomeVertexBuffer();

    void render();
};

struct StarVertexBuffer : public VertexBuffer
{
    static const Scalar RADIUS = 10.0f;

    StarVertexBuffer( const Sky::StarV& stars );

    void render();
};

typedef boost::shared_ptr<StarVertexBuffer> StarVertexBufferSP;

struct SkyRenderer
{
    SkyRenderer();

    void render( const Sky& sky );

protected:

    void rotate_sky( const Vector2f& angle ) const;
    void render_celestial_body( const GLuint texture_id, const Vector3f& color ) const;

    Texture
        sun_texture_,
        moon_texture_;

    SkydomeVertexBuffer skydome_vbo_;

    Shader skydome_shader_;

    StarVertexBufferSP star_vbo_;
};

struct Renderer
{
    Renderer();

    void note_chunk_changes( const Chunk& chunk );

    void render( const Camera& camera, const World& world );

protected:

    void render_chunks( const Vector3f& camera_position, const Sky& sky, const ChunkMap& chunks );
    void render_sky( const Sky& sky );
    gmtl::Matrix44f get_opengl_matrix( const GLenum matrix );

    typedef std::map<Vector3i, ChunkRenderer, Vector3LexicographicLess<Vector3i> > ChunkRendererMap;
    ChunkRendererMap chunk_renderers_;

    SkyRenderer sky_renderer_;

    RendererMaterialV materials_;
};

#endif // RENDERER_H
