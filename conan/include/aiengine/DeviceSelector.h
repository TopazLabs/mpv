#pragma once
#include <set>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "SystemInfo.h"

namespace aiengine {
    using namespace aiutils;
    using Json = nlohmann::json;

    class AIE_EXPORT DeviceSelector {

        SystemInfo& _systemInfo;
        float _totalRAM;
        vector<DeviceInfo> _devices;
        int _preferredDevice;
        int _allBackends;
        static unique_ptr<DeviceSelector> __pDeviceSelector;

        DeviceSelector(bool hasGLModels = true):_systemInfo(SystemInfo::GetInstance()) {
            _devices = _systemInfo.getGPUList();
            _totalRAM = _systemInfo.cpuMemory().first;
            _preferredDevice = findPreferredDevice();
            _allBackends = AllBackends();
        }

        int findPreferredDevice() {
            float maxMem = 0.0f;
            bool maxFP16 = false;
            int preferred = -1;
            if (_systemInfo.isApple()) return 0;
            bool hasOV = _systemInfo.isWindows() && _systemInfo.supportsAVX2();
            for (int i = 0; i < deviceCount(); ++i) {
                const auto &dev = _devices[i];
                if (dev.isIntel() && !hasOV) continue;
                if (dev.discrete && dev.totalMemory <= 1.5f) continue;

                if (dev.totalMemory < maxMem) continue;
                bool hasFP16 = dev.supportsDataType & aiutils::DeviceDataTypeFP16;
                if (dev.totalMemory == maxMem && hasFP16 <= maxFP16) continue;
                if (preferred >= 0 && _devices[preferred].computeLevel > dev.computeLevel) continue;
                maxMem = dev.totalMemory, maxFP16 = hasFP16, preferred = i;
            }
            return preferred;
        }

    public:
        static DeviceSelector& GetInstance(bool hasGLModels = true) {
            if(__pDeviceSelector.get() == nullptr) {
                __pDeviceSelector.reset(new DeviceSelector(hasGLModels));
            }
            return *__pDeviceSelector;
        }

        bool useLegacy(int deviceId) {
            if (isMeta(deviceId)) deviceId = _preferredDevice;
            if (deviceId == -1) return false;
            return _devices[deviceId].legacy;
        }

        unsigned threadsForDevice(int deviceId) {
            if (_systemInfo.threadCount() < 4) return 1;
            if (isMeta(deviceId)) deviceId = _preferredDevice;
            if (deviceId == -1) {
                if (!_systemInfo.supportsAVX()) return 1;
                return _systemInfo.threadCount() / 2;
            }

            auto device = _devices[deviceId];
            if (device.isIntel())
                return 1 + device.supportsDataType;
            if (device.supportsDataType & aiutils::DeviceDataTypeFP16)
                return (device.totalMemory > 4.0f) + (device.totalMemory > 6.0f) + 1;
            return (device.totalMemory > 4.0f) + 1;
        }

        float blocksForDevice(int deviceId) {
            if (isMeta(deviceId)) deviceId = _preferredDevice;
            if (deviceId == -1) {
                if (!_systemInfo.supportsAVX()) return 0.1f;
                return ((_totalRAM > 8.0f)*4 + (_totalRAM > 16.0f)*5 + 1)/10.0f;
            }

            auto device = _devices[deviceId];
            if (device.isIntel())
                return (1 + device.supportsDataType)/10.0f;
            if (device.supportsDataType & aiutils::DeviceDataTypeFP16)
                return ((device.totalMemory > 2.0f) + (device.totalMemory > 4.0f)*2 + (device.totalMemory > 4.0f)*3 + (device.totalMemory > 6.0f)*3 + 1)/10.0f;
            return ((device.totalMemory > 2.0f) + (device.totalMemory > 4.0f)*2 + (device.totalMemory > 6.0f)*3 + (device.totalMemory > 8.0f)*3 + 1)/10.0f;
        }

        // 0..gpus.size() deviceIds represent GPUs in DXGI/Metal order
        // -2 = Auto, -1 = CPU, gpus.size() = All GPUs
        bool isMeta(int deviceId) {
            return deviceId < -1 || deviceId >= deviceCount();
        }

        float minVRAM() {
            float minMem = 100.0f;
            for (auto device: _devices) {
                if (minMem > device.totalMemory)
                    minMem = device.totalMemory;
            }
            return minMem;
        }

        int deviceCount() { return (int)_devices.size(); }
        int preferredDevice() { return _preferredDevice; }

        double deviceMemory(int deviceId) {
            if (isMeta(deviceId)) deviceId = _preferredDevice;
            if (deviceId == -1) return _systemInfo.cpuMemory().first;
            return _devices[deviceId].totalMemory;
        }

        int deviceCapability(int deviceId) {
            if (isMeta(deviceId)) deviceId = _preferredDevice;
            if (deviceId == -1) return 0;

            int lvl = _devices[deviceId].computeLevel;
            const auto &name = _devices[deviceId].name;
            if (lvl == 601 && !strutils::contains(name, "GTX 10")) lvl = 0;
            if (lvl == 705 && !strutils::contains(name, "RTX 20")) lvl = 0;
            if (lvl == 806 && !strutils::contains(name, "RTX 30")) lvl = 0;
            if (lvl == 809 && !strutils::contains(name, "RTX 40")) lvl = 0;
            return lvl;
        }

        // backend selection methods
        int availableBackends(const map<string, Json> &backends, int deviceId, float minCMLMacVersion);
        MODEL_BACKEND_TYPE preferredBackend(int deviceId, int backends, float minCMLMacVersion = 10.15);
        std::set<std::string> preferredModelFiles(const map<string, Json> &backends, int deviceId);

        // for testing purposes only, as
        // model downloaders assume not set
        void forceBackends(int backends);

        vector<pair<int,int>> ComputeDeviceInstances(int deviceId, int extraThreads, MODEL_BACKEND_TYPE mbt);

        virtual ~DeviceSelector() {}
    };
}
