#pragma once
#include <string>
#include <map>
#include "StringUtil.h"

namespace aiutils {
    using namespace std;
    using namespace strutils;

    typedef enum {
        DeviceDataTypeNone,
        DeviceDataTypeFP32,
        DeviceDataTypeFP16,
        DeviceDataTypeInt8
    } DeviceDataType;

    class DeviceInfo {
    public:
        string name;
        string serial;
        float totalMemory = 0;
        float optimalMemory = 0;
        float freeMemory = 0;
        float usedMemory = 0;
        float temperature = 0;
        unsigned int index = 0;
        unsigned int cores = 0;
        bool legacy = false;
        bool discrete = false;
        bool visible = false;
        int cudaId = 0;
        int computeLevel = 0;
        DeviceDataType supportsDataType = DeviceDataTypeNone;

        bool isIntel() const {
            return contains(toUpper(name), "INTEL");
        }

        bool isNvidia() const {
            return contains(toUpper(name), "NVIDIA") || contains(toUpper(name), "QUADRO");
        }

        bool isLLVMpipe() const {
            return contains(toUpper(name), "LLVMPIPE");
        }

        bool isLegacy() const { return legacy; }
    };
}
