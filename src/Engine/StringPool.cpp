#include "StringPool.h"

namespace M4
{

StringPool::StringPool(Allocator* allocator)
{
}

const char* StringPool::AddString(const char* string)
{
    return m_strings.insert(string).first->c_str();
}

bool StringPool::GetContainsString(const char* string) const
{
    return m_strings.find(string) != m_strings.end();
}

}
