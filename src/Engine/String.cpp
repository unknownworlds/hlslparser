#include "String.h"

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <locale>
#include <algorithm>

namespace M4
{

int String_Printf(char* buffer, int bufferSize, const char* format, va_list args)
{
#ifdef _MSC_VER
    return vsnprintf(buffer, bufferSize, format, args);
#else
    return std::vsnprintf(buffer, bufferSize, format, args);
#endif
}

int String_Printf(char* buffer, int bufferSize, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    int count = String_Printf(buffer, bufferSize, format, args);

    va_end(args);
    return count;
}

bool String_Equal(const char* first, const char* second)
{
    return std::strcmp(first, second) == 0;
}

bool String_EqualNoCase(const char* first, const char* second)
{
    for (; (*first != '\0') && (*second != '\0'); ++first, ++second)
    {
        if (std::toupper(*first) != std::toupper(*second))
        {
            return false;
        }
    }

    return (*first == '\0') && (*second == '\0');
}

double String_ToDouble(const char* buffer, char** end)
{
    return std::strtod(buffer, end);
}

int String_ToInteger(const char* buffer, char** end)
{
    long value = std::strtol(buffer, end, 0);

    if (value > INT_MAX)
    {
        return INT_MAX;
    }
    if (value < INT_MIN)
    {
        return INT_MIN;
    }

    return static_cast<int>(value);
}

int String_FormatFloat(char* buffer, int bufferSize, float value)
{
    std::stringstream stream;
    stream.imbue(std::locale("C"));
    stream << value;

    const std::string str = stream.str();
    const int length = str.length();

    const int charsToCopy = std::min(length, bufferSize - 1);
    std::memcpy(buffer, str.data(), charsToCopy);
    buffer[charsToCopy] = '\0';

    return length;
}

}
