#include <fstream>
#include <vector>
#include <stdexcept>

#include "shader.h"

Shader::Shader( const std::string& shader_filename, const GLenum shader_type )
{
    std::ifstream stream( shader_filename.c_str() );

    if ( !stream.is_open() )
    {
        throw std::runtime_error( "Unable to open shader program file " + shader_filename );
    }

    const std::string program_text = std::string( std::istreambuf_iterator<char>( stream ), std::istreambuf_iterator<char>() );
    const GLchar* program_source = program_text.c_str();

    gl_shader_ = glCreateShader( shader_type );

    if ( gl_shader_ != 0 )
    {
        glShaderSource( gl_shader_, 1, &program_source, NULL );
        glCompileShader( gl_shader_ );

        check_shader_status( GL_COMPILE_STATUS );

        gl_shader_program_ = glCreateProgram();

        if ( gl_shader_program_ != 0 )
        {
            glAttachShader( gl_shader_program_, gl_shader_ );
            glLinkProgram( gl_shader_program_ );
            check_shader_status( GL_LINK_STATUS );
        }
        else throw std::runtime_error( "glCreateProgram() failed" );
    }
    else throw std::runtime_error( "glCreateShader() failed" );
}

Shader::~Shader()
{
    disable();
    delete_program();
}

void Shader::enable() const
{
    if ( gl_shader_program_ )
    {
        glUseProgram( gl_shader_program_ );
    }
}

void Shader::disable() const
{
    glUseProgram( 0 );
}

void Shader::delete_program() const
{
    if ( gl_shader_program_ != 0 )
    {
        glDeleteProgram( gl_shader_program_ );
    }
}

void Shader::check_shader_status( const GLenum which_status ) const
{
    int status;

    glGetShaderiv( gl_shader_, which_status, &status );

    if ( status == GL_FALSE )
    {
        int length;

        glGetShaderiv( gl_shader_, GL_INFO_LOG_LENGTH, &length );

        if ( length > 0 )
        {
            int written;
            std::vector<char> log( length );
            glGetShaderInfoLog( gl_shader_, length, &written, &log[0] );
            throw std::runtime_error( "Shader compilation/linking failed: " + std::string( log.begin(), log.end() ) );
        }
        else throw std::runtime_error( "Shader compilation/linking failed" );
    }
}

void Shader::set_uniform_vec2d( const std::string& name, const Vector2f& value ) const
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
