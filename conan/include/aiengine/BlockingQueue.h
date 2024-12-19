#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include "aieGlobal.h"

namespace aiutils {
    template<class Q, class T>
    class BlockingQueueTemplate {
        std::mutex _mutex;
        std::condition_variable _condVar;
    protected:
        Q _queue;
        bool _terminate;
    public:
        BlockingQueueTemplate():_terminate(false) {}

        virtual ~BlockingQueueTemplate() {
            AIE_DEBUG() << " BQT removed";
        }

        void clear() {
            std::lock_guard<std::mutex> lock(_mutex);
            _queue = Q();
        }

        unsigned int size() {
            std::lock_guard<std::mutex> lock(_mutex);
            return _queue.size();
        }

        virtual bool front(T& element) {
            std::unique_lock<std::mutex> lock(_mutex);
            _condVar.wait(lock, [&] { return _terminate || !_queue.empty(); });
            if ( _terminate) return false;
            element = _queue.front();
            return true;
        }

        virtual bool pop(T& element) {
            std::unique_lock<std::mutex> lock(_mutex);
            _condVar.wait(lock, [&] { return _terminate || !_queue.empty(); });
            if (_terminate) return false;
            element = _queue.front();
            return _queue.pop(), true;
        }

        virtual void push(const T& element) {
            std::lock_guard<std::mutex> lock(_mutex);
            _queue.push(element);
            _condVar.notify_one();
        }

        bool empty() {
            std::lock_guard<std::mutex> lock(_mutex);
            return _terminate || _queue.empty();
        }

        virtual void destroy() {
            std::lock_guard<std::mutex> lock(_mutex);
            _terminate = true;
            _condVar.notify_all();
            AIE_DEBUG() << "BQT stop signalled";
        }

        virtual void initialize() {
            _terminate = false;
        }
    };

    template<class T>
    using BlockingQueue = BlockingQueueTemplate<std::queue<T>, T>;
    template<class T>
    using BlockingPriorityQueue = BlockingQueueTemplate<std::priority_queue<T>, T>;
}
