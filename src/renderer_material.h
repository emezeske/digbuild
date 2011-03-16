#ifndef RENDERER_MATERIAL_H
#define RENDERER_MATERIAL_H

#include <GL/glew.h>

#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include "math.h"
#include "camera.h"
#include "world.h"
#include "shader.h"

struct Texture : public boost::noncopyable
{
    Texture();
    Texture( const std::string& filename );
    ~Texture();

    GLuint get_texture_id() const { return texture_id_; }
    const Vector2i& get_size() const { return size_; }

private:

    GLuint texture_id_;

    Vector2i size_;
};

struct RendererMaterial
{
    RendererMaterial( const std::string& name, ShaderSP shader );

    const Texture& get_texture() const { return texture_; }
    const Texture& get_specular_map() const { return specular_map_; }
    const Texture& get_bump_map() const { return bump_map_; }
    const ShaderSP get_shader() const { return shader_; }

private:

    Texture
        texture_,
        specular_map_,
        bump_map_;

    ShaderSP shader_;
};

typedef boost::shared_ptr<RendererMaterial> RendererMaterialSP;
typedef std::vector<RendererMaterialSP> RendererMaterialV;

struct RendererMaterialManager
{
    static const std::string
        TEXTURE_DIRECTORY,
        SHADER_DIRECTORY;

    RendererMaterialManager();

    void configure_block_material( const Vector3f& camera_position, const Sky& sky, const BlockMaterial material );
    void deconfigure_block_material();

protected:

    BlockMaterial current_material_;

    ShaderSP current_shader_;

    RendererMaterialV materials_;
};

#endif // RENDERER_MATERIAL_H
