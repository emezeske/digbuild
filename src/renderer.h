#ifndef RENDERER_H
#define RENDERER_H

#include <GL/glew.h>

#include <set>

#include <boost/utility.hpp>

#include "camera.h"
#include "world.h"
#include "renderer_material.h"

struct VertexBuffer : public boost::noncopyable
{
    VertexBuffer( const GLsizei num_elements = 0 );
    virtual ~VertexBuffer();

    void bind();
    void unbind();

protected:

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

    BlockVertex(
        const Vector3f& position,
        const Vector3f& normal,
        const Vector3f& tangent,
        const Vector2f& texcoords,
        const Vector4f& lighting
    ) :
        x_( position[0] ), y_( position[1] ), z_( position[2] ),
        nx_( normal[0] ), ny_( normal[1] ), nz_( normal[2] ),
        tx_( tangent[0] ), ty_( tangent[1] ), tz_( tangent[2] ),
        s_( texcoords[0] ), t_( texcoords[1] ),
        lr_( lighting[0] ), lg_( lighting[1] ), lb_( lighting[2] ), ls_( lighting[3] )
    {
    }

    GLfloat x_, y_, z_;
    GLfloat nx_, ny_, nz_;
    GLfloat tx_, ty_, tz_;
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
typedef std::vector<BlockMaterial> BlockMaterialV;
typedef std::vector<Vector3f> Vector3fV;

struct SortableChunkVertexBuffer : public ChunkVertexBuffer
{
    SortableChunkVertexBuffer( const BlockMaterialV& materials, const BlockVertexV& vertices );

    void render( const Vector3f& camera_position, const Sky& sky, RendererMaterialManager& material_manager );

private:

    static const unsigned VERTICES_PER_FACE = 4;

    // TODO: While using a std::set is convenient here, it is very slow.
    typedef std::pair<Scalar, GLuint> DistanceIndexPair;
    typedef std::set<DistanceIndexPair> DistanceIndexSet;

    void render_sorted(
        const DistanceIndexSet distance_indices,
        const Vector3f& camera_position,
        const Sky& sky,
        const BlockMaterial material,
        RendererMaterialManager& material_manager
    );

    BlockMaterialV materials_;

    Vector3fV centroids_;
};

typedef boost::shared_ptr<SortableChunkVertexBuffer> SortableChunkVertexBufferSP;
typedef std::set<BlockMaterial> BlockMaterialSet;

struct ChunkRenderer
{
    ChunkRenderer( const Vector3f& centroid = Vector3f(), const gmtl::AABoxf& aabb = gmtl::AABoxf() );

    void render_opaque( const BlockMaterial material, RendererMaterialManager& material_manager );
    void render_translucent( const Vector3f& camera_position, const Sky& sky, RendererMaterialManager& material_manager );
    void rebuild( const Chunk& chunk );

    const BlockMaterialSet& get_opaque_materials() const { return opaque_materials_; }
    const Vector3f& get_centroid() const { return centroid_; }
    const gmtl::AABoxf& get_aabb() const { return aabb_; }
    unsigned get_num_triangles() const { return num_triangles_; }

protected:

    void get_vertices_for_face( const BlockFace& face, BlockVertexV& vertices ) const;

    BlockMaterialSet opaque_materials_;

    ChunkVertexBufferMap opaque_vbos_;

    SortableChunkVertexBufferSP translucent_vbo_;

    Vector3f centroid_;

    gmtl::AABoxf aabb_;

    unsigned num_triangles_;
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

    unsigned get_num_chunks_drawn() const { return num_chunks_drawn_; }
    unsigned get_num_triangles_drawn() const { return num_triangles_drawn_; }

protected:

    void render_chunks( const Vector3f& camera_position, const Sky& sky );
    void render_sky( const Sky& sky );
    gmtl::Matrix44f get_opengl_matrix( const GLenum matrix );

    RendererMaterialManager material_manager_;

    typedef std::map<Vector3i, ChunkRenderer, Vector3LexicographicLess<Vector3i> > ChunkRendererMap;
    ChunkRendererMap chunk_renderers_;

    SkyRenderer sky_renderer_;

    unsigned num_chunks_drawn_;

    unsigned num_triangles_drawn_;
};

#endif // RENDERER_H
