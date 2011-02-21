#ifndef LOG_H
#define LOG_H

#include <iostream>

#define LOG( message )\
    do\
    {\
        std::cerr << __FILE__ << " +" << __LINE__ << " " << __func__ << "(): " << message << std::endl;\
    }\
    while ( false )

#endif // LOG_H
