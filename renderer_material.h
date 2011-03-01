#ifndef RENDERER_MATERIAL_H
#define RENDERER_MATERIAL_H

#include <GL/gl.h>

#include <vector>

#include <boost/shared_ptr.hpp>

#include "math.h"

struct Texture
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
    RendererMaterial();
    RendererMaterial( const std::string& name );

    const Texture& texture() const { return texture_; }

private:

    static const std::string TEXTURES_DIRECTORY;

    Texture texture_;
};

typedef boost::shared_ptr<RendererMaterial> RendererMaterialSP;
typedef std::vector<RendererMaterialSP> RendererMaterialV;

#endif // RENDERER_MATERIAL_H
