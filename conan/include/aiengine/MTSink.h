#pragma once
#include <thread>
#include <atomic>
#include <chrono>
#include "aieGlobal.h"
#include "PriorityBlockingQueue.h"

namespace aiutils {
    using namespace std;
    template<class T>
    class MTSink {
    protected:
        unsigned int _threadCount;
        atomic_bool _keepProcessing;
        unique_ptr<BlockingQueue<T>> _pInputQueue;
        vector<thread> _threads;
        atomic_uint _activeProcesses;

        bool callProcess(const T& data) {
            _activeProcesses++;
            auto retVal = process(data);
            _activeProcesses--;
            return retVal;
        }
    public:
        MTSink(unsigned int maxQueueLength = 0, unsigned int threadCount = 1):_threadCount(threadCount), _keepProcessing(false) {
            _pInputQueue.reset(maxQueueLength ? new ClampedBlockingQueue<T>(maxQueueLength) : new BlockingQueue<T>());
        }

        virtual void initialize() {
            _keepProcessing = true;
            _pInputQueue->initialize();
            for(unsigned int i=0;i<_threadCount;i++) {
                _threads.push_back(thread(&MTSink::run, this));
            }
        }

        virtual void waitAndDestroy() {
            AIE_DEBUG() << " Write Queue Size Before: " << _pInputQueue->size();
            while(_keepProcessing && (!_pInputQueue->empty() || _activeProcesses > 0)) {
                this_thread::sleep_for(chrono::milliseconds(100));
            }
            AIE_DEBUG() << " Write Queue Size After: " << _pInputQueue->size();
            destroy();
        }

        virtual void destroy() {
            _keepProcessing = false;
            _pInputQueue->destroy();
            _pInputQueue->clear();
            for(auto& thread: _threads)
                if(thread.joinable())
                    thread.join();
        }

        virtual void run() {
            while(_keepProcessing) {
                T data;
                if(!_pInputQueue->pop(data) || !_keepProcessing || !callProcess(data))
                    break;
            }
            _keepProcessing = false;
        }

        virtual void processAsync(const T& data) {
            _pInputQueue->push(data);
        }

        virtual bool process(const T& data) = 0;
        bool keepProcessing() { return _keepProcessing; }
    };
}
