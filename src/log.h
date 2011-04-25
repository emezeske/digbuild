#ifndef LOG_H
#define LOG_H

#include <string>
#include <iostream>
#include <sstream>

//! Provide a quick and easy way to create a string with stream operations without having to
//! create a named variable.
struct make_string
{
    template <typename T>
    make_string& operator<<( T const& datum )
    {
        stream_ << datum;
        return *this;
    }

    operator std::string() const
    {
        return stream_.str();
    }

private:

    std::ostringstream stream_;
};

#define LOG( message )\
    do\
    {\
        std::cerr << __FILE__ << " +" << __LINE__ << " " << __func__ << "(): " << message << std::endl;\
    }\
    while ( false )

#endif // LOG_H
