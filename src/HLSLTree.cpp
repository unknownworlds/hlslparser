#include "Engine/Assert.h"

#include "HLSLTree.h"

namespace M4
{

HLSLTree::HLSLTree(Allocator* allocator) :
    m_allocator(allocator), m_stringPool(allocator)
{
    m_firstPage         = m_allocator->New<NodePage>();
    m_firstPage->next   = NULL;

    m_currentPage       = m_firstPage;
    m_currentPageOffset = 0;

    m_root              = AddNode<HLSLRoot>(NULL, 1);
}

HLSLTree::~HLSLTree()
{
    NodePage* page = m_firstPage;
    while (page != NULL)
    {
        NodePage* next = page->next;
        m_allocator->Delete(page);
        page = next;
    }
}

void HLSLTree::AllocatePage()
{
    NodePage* newPage    = m_allocator->New<NodePage>();
    newPage->next        = NULL;
    m_currentPage->next  = newPage;
    m_currentPageOffset  = 0;
    m_currentPage        = newPage;
}

const char* HLSLTree::AddString(const char* string)
{   
    return m_stringPool.AddString(string);
}

bool HLSLTree::GetContainsString(const char* string) const
{
    return m_stringPool.GetContainsString(string);
}

HLSLRoot* HLSLTree::GetRoot() const
{
    return m_root;
}

void* HLSLTree::AllocateMemory(size_t size)
{
    if (m_currentPageOffset + size > s_nodePageSize)
    {
        AllocatePage();
    }
    void* buffer = m_currentPage->buffer + m_currentPageOffset;
    m_currentPageOffset += size;
    return buffer;
}

}