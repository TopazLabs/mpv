#pragma once
#include "DiskCachable.h"
#include <queue>
#include <mutex>

namespace aiutils {
    template<class T>
    class DiskCachableQueue {
        int _cacheAfter;
        queue<shared_ptr<T>> _queue;
        mutex _mutex;
        shared_ptr<T> _front() {
            shared_ptr<T> pCachable = nullptr;
            if(_queue.size() > 0) {
                pCachable = _queue.front();
                if(pCachable->isCached())
                    pCachable->load();
            }
            return pCachable;
        }

    public:
        DiskCachableQueue(int cacheAfter = 5):_cacheAfter(cacheAfter) {}

        virtual void setCacheAfter(int cacheAfter) {
            lock_guard<mutex> lock(_mutex);
            _cacheAfter = cacheAfter;
        }

        virtual void push(shared_ptr<T> pCachable) {
            lock_guard<mutex> lock(_mutex);
            if(_queue.size() >= _cacheAfter) {
                _queue.back()->cache();
            }
            _queue.push(pCachable);
        }

        virtual shared_ptr<T> front() {
            lock_guard<mutex> lock(_mutex);
            return _front();
        }

        virtual shared_ptr<T> back() {
            lock_guard<mutex> lock(_mutex);
            shared_ptr<T> pCachable = nullptr;
            if(_queue.size() > 0) {
                pCachable = _queue.back();
                if(pCachable->isCached())
                    pCachable->load();
            }
            return pCachable;
        }

        virtual shared_ptr<T> pop() {
            lock_guard<mutex> lock(_mutex);
            shared_ptr<T> pCachable = _front();
            if(pCachable != nullptr) {
                _queue.pop();
            }
            return pCachable;
        }

        virtual void clear() {
            lock_guard<mutex> lock(_mutex);
            while (_queue.size() > 0) {
               _queue.pop();
            }
        }

        virtual int size() { lock_guard<mutex> lock(_mutex); return _queue.size(); }
        virtual bool isEmpty() { lock_guard<mutex> lock(_mutex); return _queue.size() == 0; }
        virtual ~DiskCachableQueue() {
            clear();
        }
    };
}
