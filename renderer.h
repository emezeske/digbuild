#ifndef RENDERER_H
#define RENDERER_H

#include <GL/gl.h>

#include <map>

#include "region.h"

struct ChunkRenderer
{
    ChunkRenderer();

    void render();
    void initialize( const Chunk& chunk );

    bool initialized_;

private:

    void create_buffers();
    void destroy_buffers();
    void bind();

    GLuint
        vbo_id_,
        ibo_id_;

    GLsizei vertex_count_;
};

struct Renderer
{
    Renderer();

    void render( const RegionMap& regions );

protected:

    typedef std::map<Vector3i, ChunkRenderer, Vector3LexicographicLess<Vector3i> > ChunkRendererMap;
    ChunkRendererMap chunk_renderers_;
};

#endif // RENDERER_H
