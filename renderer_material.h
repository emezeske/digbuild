#ifndef RENDERER_MATERIAL_H
#define RENDERER_MATERIAL_H

#include <GL/glew.h>

#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include "math.h"
#include "shader.h"

struct Texture : public boost::noncopyable
{
    Texture();
    Texture( const std::string& filename );
    ~Texture();

    GLuint texture_id() const { return texture_id_; }
    const Vector2i& size() const { return size_; }

private:

    GLuint texture_id_;

    Vector2i size_;
};

struct RendererMaterial
{
    RendererMaterial( const std::string& name );

    const Texture& texture() const { return texture_; }
    const Shader& vertex_shader() const { return vertex_shader_; }
    const Shader& fragment_shader() const { return fragment_shader_; }

private:

    static const std::string
        TEXTURE_DIRECTORY,
        SHADER_DIRECTORY;

    Texture texture_;

    Shader
        vertex_shader_,
        fragment_shader_;
};

typedef boost::shared_ptr<RendererMaterial> RendererMaterialSP;
typedef std::vector<RendererMaterialSP> RendererMaterialV;

#endif // RENDERER_MATERIAL_H
