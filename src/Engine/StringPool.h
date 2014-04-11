#ifndef ENGINE_STRING_POOL_H
#define ENGINE_STRING_POOL_H

#include <set>
#include <string>

namespace M4
{

class Allocator;

class StringPool
{

public:

    StringPool(Allocator* allocator);

    const char* AddString(const char* string);
    bool GetContainsString(const char* string) const;

private:

    std::set<std::string> m_strings;

};

}

#endif
