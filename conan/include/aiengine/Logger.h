#pragma once
#include <iostream>
#include <streambuf>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <thread>

namespace aiutils {
    class NullBuffer : public std::streambuf {
    public:
        int overflow(int c) { return c; }
    };

    class AIE_EXPORT Logger {
        bool _enabled = true, _isOutStreamSet = false;
        std::ostream _nullStream = std::ostream(new NullBuffer());
        std::ostream _outStream;
        std::mutex _mutex;

        inline static Logger* __pLogger = nullptr;

        Logger():_isOutStreamSet(false) , _outStream(new NullBuffer()) {}
        Logger(std::streambuf* pStreamBuffer):_isOutStreamSet(true), _outStream(pStreamBuffer) {}

        std::ostream& getOutStream() {
            return _isOutStreamSet ? _outStream : std::cerr;
        }

        std::ostream& stream() {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_enabled) {
                auto curTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                getOutStream() << std::endl << std::put_time(std::localtime(&curTime), "%Y-%m-%d %X ") << std::this_thread::get_id() << " ";
                return getOutStream();
            }
            return _nullStream;
        }

        static Logger& GetInstance() {
            if(!__pLogger)
                __pLogger = new Logger();
            return *__pLogger;
        }

    public:
        static Logger& SetInstance(std::streambuf* pStreamBuffer) {
            if (__pLogger)
                delete __pLogger;
            __pLogger = new Logger(pStreamBuffer);
            return *__pLogger;
        }

        static std::ostream& Log() {
            return GetInstance().stream();
        }

        static void Enable() {
            GetInstance()._enabled = true;
        }

        static void Disable() {
            GetInstance()._enabled = false;
        }
    };
}

