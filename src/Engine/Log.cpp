#include "Log.h"
#include "String.h"

#include <iostream>

namespace M4
{

void Log_Error(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    char buffer[1024];
    int count = String_Printf(buffer, sizeof(buffer), format, args);

    va_end(args);

    std::cerr << "ERROR: " << buffer << '\n';
}

}
