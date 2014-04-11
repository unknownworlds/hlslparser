#ifndef ENGINE_STRING_H
#define ENGINE_STRING_H

#include <cstdarg>

namespace M4
{

int String_Printf(char* buffer, int bufferSize, const char* format, va_list args);

int String_Printf(char* buffer, int bufferSize, const char* format, ...);

bool String_Equal(const char* first, const char* second);

bool String_EqualNoCase(const char* first, const char* second);

double String_ToDouble(const char* buffer, char** end);

int String_ToInteger(const char* buffer, char** end);

int String_FormatFloat(char* buffer, int bufferSize, float value);

}

#endif
