#ifndef SHADER_H
#define SHADER_H

#include <GL/glew.h>
#include <string>

#include "math.h"

struct Shader
{
    Shader( const std::string& shader_filename, const GLenum shader_type );
    ~Shader();

    void enable() const;
    void disable() const;

    void set_uniform_vec2d( const std::string& name, const Vector2f& value ) const;
    void set_uniform_float( const std::string& name, const float v ) const;
    void set_uniform_int( const std::string& name, const int v ) const;

protected:

    GLuint
        gl_shader_,
        gl_shader_program_;

    void delete_program() const;
    void check_shader_status( const GLenum which_status ) const;
    int get_uniform_location( const std::string& name ) const;
};

#endif // SHADER_H

