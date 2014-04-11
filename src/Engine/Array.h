#ifndef ENGINE_ARRAY_H
#define ENGINE_ARRAY_H

#include <vector>

namespace M4
{

class Allocator;

template<typename T>
class Array
{

public:

    Array(Allocator* allocator) {}

    void PushBack(const T& element)
    {
        elements_.push_back(element);
    }

    T& PushBackNew()
    {
        elements_.push_back(T());
        return elements_.back();
    }

    void Resize(int newSize)
    {
        elements_.resize(newSize);
    }

    int GetSize() const
    {
        return static_cast<int>(elements_.size());
    }

    T& operator[](int index)
    {
        return elements_[index];
    }

    const T& operator[](int index) const
    {
        return elements_[index];
    }

private:

    std::vector<T> elements_;

};

}

#endif
