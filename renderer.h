#ifndef RENDERER_H
#define RENDERER_H

#include <GL/gl.h>

#include <map>

#include "region.h"

struct VertexBuffer
{
    VertexBuffer();

    void create_buffers();
    void destroy_buffers();
    void bind();

    GLuint
        vbo_id_,
        ibo_id_;

    GLsizei vertex_count_;

    bool initialized_;
};

struct Renderer
{
    Renderer();

    void render( const RegionMap& regions );

protected:

    void render_chunk(
        const Vector2i& chunk_index,
        const Region& region,
        const Region* region_north,
        const Region* region_south,
        const Region* region_east,
        const Region* region_west
    );

    typedef std::map<Vector2i, VertexBuffer, Vector2LexicographicLess<Vector2i> > ChunkVertexBufferMap;

    ChunkVertexBufferMap chunk_vbos_;
};

#endif // RENDERER_H
