#pragma once
#include "BlockingQueue.h"

namespace aiutils {
    template<class T>
    class ResourceManager {
    protected:
        BlockingQueue<T> _resources;
    public:
        ResourceManager() {}
        void add(T& resource) { _resources.push(resource); }
        bool canAcquire() { return _resources.size() > 0; }
        T acquire() {
            T t;
            _resources.pop(t);
            return t;
        }
        void release(T& resource) { _resources.push(resource); }
        virtual ~ResourceManager() {}
    };
}
