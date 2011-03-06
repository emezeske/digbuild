#ifndef RENDERER_H
#define RENDERER_H

#include <GL/glew.h>

#include <boost/utility.hpp>

#include "chunk.h"
#include "renderer_material.h"

struct BlockVertex
{
    BlockVertex()
    {
    }

    BlockVertex( const Vector3f& position, const Vector3f& normal, const Vector2f& texcoords, const Vector3f& lighting ) :
        x_( position[0] ), y_( position[1] ), z_( position[2] ),
        nx_( normal[0] ), ny_( normal[1] ), nz_( normal[2] ),
        s_( texcoords[0] ), t_( texcoords[1] ),
        lr_( lighting[0] ), lg_( lighting[1] ), lb_( lighting[2] )
    {
    }

    GLfloat x_, y_, z_;
    GLfloat nx_, ny_, nz_;
    GLfloat s_, t_;
    GLfloat lr_, lg_, lb_;

} __attribute__( ( packed ) );

typedef std::vector<BlockVertex> BlockVertexV;

struct ChunkVertexBuffer : public boost::noncopyable
{
    ChunkVertexBuffer();
    ChunkVertexBuffer( const BlockVertexV& vertices );
    ~ChunkVertexBuffer();

    void render();

private:

    void bind();

    GLuint
        vbo_id_,
        ibo_id_;

    GLsizei vertex_count_;
};

typedef boost::shared_ptr<ChunkVertexBuffer> ChunkVertexBufferSP;
typedef std::map<BlockMaterial, ChunkVertexBufferSP> ChunkVertexBufferMap;

struct ChunkRenderer
{
    ChunkRenderer();

    void render( const RendererMaterialV& materials );
    void initialize( const Chunk& chunk );
    bool initialized() const { return initialized_; }

private:

    bool initialized_;

    ChunkVertexBufferMap vbos_;
};

struct Renderer
{
    Renderer();

    void render( const ChunkMap& chunks );

protected:

    typedef std::map<Vector3i, ChunkRenderer, Vector3LexicographicLess<Vector3i> > ChunkRendererMap;
    ChunkRendererMap chunk_renderers_;

    RendererMaterialV materials_;
};

#endif // RENDERER_H
