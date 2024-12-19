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
        float totalMemory;
        float optimalMemory;
        float freeMemory;
        float usedMemory;
        float temperature;
        unsigned int index;
        unsigned int cores;
        bool legacy;
        bool discrete;
        bool visible;
        int cudaId;
        int computeLevel;
        DeviceDataType supportsDataType;
        DeviceInfo():cores(0),computeLevel(0) {}
        DeviceInfo(const DeviceInfo& deviceInfo) {
            *this = deviceInfo;
        }

        DeviceInfo& operator=(const DeviceInfo& deviceInfo) {
            name = deviceInfo.name;
            serial = deviceInfo.serial;
            totalMemory = deviceInfo.totalMemory;
            optimalMemory = deviceInfo.optimalMemory;
            freeMemory = deviceInfo.freeMemory;
            usedMemory = deviceInfo.usedMemory;
            temperature = deviceInfo.temperature;
            cores = deviceInfo.cores;
            index = deviceInfo.index;
            legacy = deviceInfo.legacy;
            discrete = deviceInfo.discrete;
            visible = deviceInfo.visible;
            supportsDataType = deviceInfo.supportsDataType;
            cudaId = deviceInfo.cudaId;
            computeLevel = deviceInfo.computeLevel;
            return *this;
        }

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
