#ifndef ENGINE_ALLOCATOR_H
#define ENGINE_ALLOCATOR_H

namespace M4
{

class Allocator
{

public:

    Allocator()
    {
    }

    template<typename T>
    T* New()
    {
        return new T;
    }

    template<typename T>
    void Delete(T* ptr) {
        delete ptr;
    }

private:

};

}

#endif
