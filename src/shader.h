#ifndef SHADER_H
#define SHADER_H

#include <GL/glew.h>
#include <string>

#include <boost/shared_ptr.hpp>

#include "math.h"

struct Shader
{
    Shader( const std::string& vertex_shader_filename, const std::string& fragment_shader_filename );
    ~Shader();

    void enable() const;
    void disable() const;

    void set_uniform_vec3f( const std::string& name, const Vector3f& value ) const;
    void set_uniform_vec2f( const std::string& name, const Vector2f& value ) const;
    void set_uniform_float( const std::string& name, const float v ) const;
    void set_uniform_int( const std::string& name, const int v ) const;

protected:

    GLuint
        gl_vertex_shader_,
        gl_fragment_shader_,
        gl_shader_program_;

    GLuint load_shader( const std::string& filename, const GLenum shader_type );
    void delete_program() const;
    void check_shader_status( const GLuint shader, const GLenum status_type ) const;
    int get_uniform_location( const std::string& name ) const;
};

typedef boost::shared_ptr<Shader> ShaderSP;

#endif // SHADER_H

