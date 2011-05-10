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
