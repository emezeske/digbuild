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

#include <fstream>
#include <vector>
#include <stdexcept>

#include "shader.h"

Shader::Shader( const std::string& vertex_shader_filename, const std::string& fragment_shader_program )
{
    gl_shader_program_ = glCreateProgram();

    if ( gl_shader_program_ != 0 )
    {
        gl_vertex_shader_ = load_shader( vertex_shader_filename, GL_VERTEX_SHADER );
        gl_fragment_shader_ = load_shader( fragment_shader_program, GL_FRAGMENT_SHADER );
    }
    else throw std::runtime_error( "glCreateProgram() failed" );
}

Shader::~Shader()
{
    disable();
    delete_program();
}

void Shader::enable() const
{
    glUseProgram( gl_shader_program_ );
}

void Shader::disable() const
{
    glUseProgram( 0 );
}

GLuint Shader::load_shader( const std::string& filename, const GLenum shader_type )
{
    std::ifstream stream( filename.c_str() );

    if ( !stream.is_open() )
    {
        throw std::runtime_error( "Unable to open shader file " + filename );
    }

    const std::string _text = std::string( std::istreambuf_iterator<char>( stream ), std::istreambuf_iterator<char>() );

    if ( !stream.good() )
    {
        throw std::runtime_error( "Error reading shader file " + filename );
    }

    const GLchar* text = _text.c_str();

    GLuint gl_shader = glCreateShader( shader_type );

    if ( gl_shader == 0 )
    {
        throw std::runtime_error( "glCreateShader() failed" );
    }

    glShaderSource( gl_shader, 1, &text, NULL );
    glCompileShader( gl_shader );
    check_shader_status( gl_shader, GL_COMPILE_STATUS );
    glAttachShader( gl_shader_program_, gl_shader );
    glLinkProgram( gl_shader_program_ );
    check_shader_status( gl_shader, GL_LINK_STATUS );
    return gl_shader;
}

void Shader::delete_program() const
{
    glDeleteProgram( gl_shader_program_ );
}

void Shader::check_shader_status( const GLuint shader, const GLenum status_type ) const
{
    int status;

    glGetShaderiv( shader, status_type, &status );

    if ( status == GL_FALSE )
    {
        int length;

        glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &length );

        if ( length > 0 )
        {
            int written;
            std::vector<char> log( length );
            glGetShaderInfoLog( shader, length, &written, &log[0] );
            throw std::runtime_error( "Shader compilation/linking failed: " + std::string( log.begin(), log.end() ) );
        }
        else throw std::runtime_error( "Shader compilation/linking failed" );
    }
}

void Shader::set_uniform_vec3f( const std::string& name, const Vector3f& value ) const
{
    glUniform3f( get_uniform_location( name ), value[0], value[1], value[2] );
}

void Shader::set_uniform_vec2f( const std::string& name, const Vector2f& value ) const
{
    glUniform2f( get_uniform_location( name ), value[0], value[1] );
}

void Shader::set_uniform_float( const std::string& name, const float value ) const
{
    glUniform1f( get_uniform_location( name ), value );
}

void Shader::set_uniform_int( const std::string& name, const int value ) const
{
    glUniform1i( get_uniform_location( name ), value );
}

int Shader::get_uniform_location( const std::string& name ) const
{
    const int location = glGetUniformLocation( gl_shader_program_, name.c_str() );
    
    if ( location == -1 )
    {
        throw std::runtime_error( "Could not find uniform location: " + name );
    }

    return location;
}
