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

    void render( const RegionV& regions );

protected:

    void render_chunk(
        const Vector2i& region_position,
        const Vector2i& chunk_position,
        const Chunk& chunk,
        const Chunk* chunk_north,
        const Chunk* chunk_south,
        const Chunk* chunk_east,
        const Chunk* chunk_west
    );

    struct GlobalChunkPosition
    {
        GlobalChunkPosition( const Vector2i& region_position, const Vector2i& chunk_position );

        bool operator<( const GlobalChunkPosition& other ) const; 

        const Vector2i
            region_position_,
            chunk_position_;
    };

    typedef std::map<GlobalChunkPosition, VertexBuffer> ChunkVertexBufferMap;

    ChunkVertexBufferMap chunk_vbos_;
};

#endif // RENDERER_H
