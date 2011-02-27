#ifndef RENDERER_H
#define RENDERER_H

#include <GL/gl.h>

#include <map>

#include "region.h"

enum NeighborRelation
{
    NEIGHBOR_ABOVE,
    NEIGHBOR_BELOW,
    NEIGHBOR_NORTH,
    NEIGHBOR_SOUTH,
    NEIGHBOR_EAST,
    NEIGHBOR_WEST,
    NEIGHBOR_RELATION_SIZE
};

struct ChunkRenderer
{
    ChunkRenderer();

    void render();
    void initialize(
        const Vector3i& chunk_position,
        const Chunk& chunk,
        const Chunk* neighbor_chunks[NEIGHBOR_RELATION_SIZE]
    );

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
