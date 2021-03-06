///////////////////////////////////////////////////////////////////////////
// Copyright 2011 Evan Mezeske.
//
// This file is part of Digbuild.
// 
// Digbuild is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.
// 
// Digbuild is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Digbuild.  If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////

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

struct RendererMaterialManager : public boost::noncopyable
{
    static const std::string
        TEXTURE_DIRECTORY,
        SHADER_DIRECTORY;

    static const size_t
        TEXTURE_SIZE          = 512,
        BUMP_MAP_SIZE         = 512,
        SPECULAR_MAP_SIZE     = 512,
        TEXTURE_CHANNELS      = 4,
        BUMP_MAP_CHANNELS     = 3,
        SPECULAR_MAP_CHANNELS = 1;

    RendererMaterialManager();
    ~RendererMaterialManager();

    void configure_materials( const Camera& camera, const Sky& sky );
    void deconfigure_materials();

protected:

    void read_texture_data(
        const std::string& filename,
        const int size,
        const int num_colors,
        std::vector<unsigned char>& texture_data
    );

    void create_texture_array(
        const std::string& filename_postfix,
        const int texture_size,
        const int texture_channels,
        GLuint& id
    );

    GLuint
        texture_array_id_,
        bump_map_array_id_,
        specular_map_array_id_;

    ShaderSP material_shader_;
};

#endif // RENDERER_MATERIAL_H
