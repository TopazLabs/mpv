#pragma once
#include "BlockingQueue.h"

namespace aiutils {
    using namespace std;
    template<class Q, class T>
    class ClampedBlockingQueueTemplate: public BlockingQueueTemplate<Q,T> {

        mutex _clampMutex;
        condition_variable _clampCondVar;
        unsigned int _maxSize;
    public:
        ClampedBlockingQueueTemplate(unsigned int maxQueueLength = 1000):BlockingQueueTemplate<Q,T>(),_maxSize(maxQueueLength) {}
        virtual ~ClampedBlockingQueueTemplate() {
            destroy();
            AIE_DEBUG() << " CBQT removed";
        }

        virtual void push(const T& element) override {
            unique_lock<mutex> lock(_clampMutex);
            _clampCondVar.wait(lock, [&] { return BlockingQueue<T>::_queue.size() < _maxSize || BlockingQueue<T>::_terminate; });
            if(!BlockingQueue<T>::_terminate)
                BlockingQueue<T>::push(element);
        }

        virtual bool pop(T& element) override {
            auto retVal = BlockingQueue<T>::pop(element);
            if(retVal)
                _clampCondVar.notify_one();
            return retVal;
        }

        virtual void destroy() override {
            BlockingQueue<T>::destroy();
            _clampCondVar.notify_all();
        }
    };

    template<class T>
    using ClampedBlockingQueue = ClampedBlockingQueueTemplate<std::queue<T>,T>;

    template<class T>
    using PriorityBlockingQueue = ClampedBlockingQueueTemplate<priority_queue<T>,T>;

}
