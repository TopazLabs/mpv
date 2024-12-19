#pragma once
#include "aieGlobal.h"
#include <vector>
#include <string>
#include "DeviceInfo.h"

#define AIMESSAGE(msg) if(aiutils::SystemInfo::MessagesEnabled()) { std::cerr << "\n:AIINFO: " << msg << std::endl; }
#define AIERROR(msg) if(aiutils::SystemInfo::MessagesEnabled()) { std::cerr << "\n:AIERROR: " << msg << std::endl; }

namespace aiutils {
using namespace std;
class AIE_EXPORT SystemInfo {
    vector<DeviceInfo> _gpuList;
    pair<double,double> _cpuMemory;
    float _osVersion;
    bool _supportsAVX, _supportsAVX2, _isWindows, _isLinux, _supportsVNNI, _messagesEnabled;
    int _nativeThreadCount;
    string _cpuName;
    bool _hasGLModels;
    string _machineId;
    void loadCommonFeatures();
    void loadSystemFeatures();
    void loadCPUInfo();
    string fetchCPUName();
    bool isAppTranslated();
    vector<DeviceInfo> fetchGPUInfo();
    string fetchMachineId();
    void loadInstructionsForCPU();

    SystemInfo(): _isLinux(false) {
        _messagesEnabled = strutils::getEnv("TOPAZ_AIE_MESSAGES") == "1";
        loadSystemFeatures();
        logSystemInfo();
    }
public:
    static SystemInfo& GetInstance() {
        static SystemInfo* __pSystemInfo = new SystemInfo();
        return *__pSystemInfo;
    }

    static bool MessagesEnabled() { return GetInstance()._messagesEnabled; }

    pair<double,double> fetchMemory();

    string cpuName() { return _cpuName; }
    pair<double,double> cpuMemory(bool reload = false) {
        if(reload)
            _cpuMemory = fetchMemory();
        return _cpuMemory;
    }

    int threadCount() const { return _nativeThreadCount; }
    bool supportsAVX() const { return _supportsAVX;}
    bool supportsAVX2() const { return _supportsAVX2;}
    bool supportsVNNI() const { return _supportsVNNI;}
    bool isWindows() const { return _isWindows;}
    bool isMac() const { return !_isWindows && !_isLinux;}
    bool isApple() const {
        using namespace strutils;
        return contains(toLower(_cpuName), "apple");
    }
    float osVersion() const { return _osVersion; }

    const vector<DeviceInfo>& getGPUList(bool reload = false) {
        if(reload || _gpuList.size() == 0) {
            AIE_DEBUG() << " Fetching GPU list " << _gpuList.size() << reload;
            _gpuList = fetchGPUInfo();
            AIE_DEBUG() << " Just fetched GPU list " << _gpuList.size();
        }
        return _gpuList;
    }

    bool allIntel() {
        auto gpus = getGPUList();
        for(const auto& gpu:gpus) {
            if(!gpu.isIntel())
                return false;
        }
        return true;
    }

    Json toJson() {
        std::string os = isMac() ? "Mac" : (isWindows() ? "Windows" : "Linux");
        auto ram = cpuMemory();
        auto devices = getGPUList();

        Json systemInfo;
        systemInfo["OS"] = os;
        systemInfo["OS Version"] = osVersion();
        systemInfo["CPU"] = cpuName();
        systemInfo["Threads"] = threadCount();
        systemInfo["AVX"] = supportsAVX();
        systemInfo["AVX2"] = supportsAVX2();
        systemInfo["VNNI"] = supportsVNNI();
        systemInfo["Is Apple Processor"] = isApple();
        systemInfo["RAM"] = ram.first;
        systemInfo["RAM Free/Used"] = ram.second;
        systemInfo["Machine ID"] = getMachineId();
        systemInfo["Device count"] = devices.size();

        Json deviceList = Json::array();
        for (const auto& device : devices) {
            Json deviceJson;
            deviceJson["Index"] = device.index;
            deviceJson["Name"] = device.name;
            deviceJson["Cores"] = device.cores;
            deviceJson["VRAM Total"] = device.totalMemory;
            deviceJson["VRAM Used"] = device.usedMemory;
            deviceJson["DataType"] = device.supportsDataType;
            deviceJson["Serial"] = device.serial;
            deviceJson["ComputeLevel"] = device.computeLevel;
            deviceJson["CUDA ID"] = device.cudaId;
            deviceJson["Visible"] = device.visible;
            deviceJson["Legacy"] = device.legacy;

            deviceList.push_back(deviceJson);
        }
        systemInfo["Devices"] = deviceList;

        return systemInfo;
    }

    void logSystemInfo() {
        std::string os = isMac() ? "Mac" : (isWindows() ? "Windows" : "Linux");
        auto ram = cpuMemory();
        auto devices = getGPUList();

        AIE_INFO() << "=== System Information ===";
        AIE_INFO() << "OS " << os << " Version " << osVersion();
        AIE_INFO() << "CPU " << cpuName().c_str()
            << " Threads " << threadCount() << " AVX " << supportsAVX()
            << " AVX2 " << supportsAVX2() << " VNNI " << supportsVNNI();
        AIE_INFO() << "Is Apple Processor " << isApple();
        AIE_INFO() << "RAM " << ram.first << " GB Total / " << ram.second << " Free/Used";
        AIE_INFO() << "Machine ID: " << _machineId;
        AIE_INFO() << "Device count: " << devices.size();
        for (const auto &device: devices) {
            AIE_INFO() << "- Index " << device.index
                << " Name " << device.name.c_str() << " Cores " << device.cores;
            AIE_INFO() << " VRAM " << device.totalMemory
                << " GB Total / " << device.usedMemory
                << " GB Used DataType " << device.supportsDataType;
            AIE_INFO() << " Serial " << device.serial.c_str()
                << " ComputeLevel: " << device.computeLevel
                << " CUDA ID: " << device.cudaId
                << " Visible: " << device.visible
                << " Legacy: " << device.legacy;
        }
        AIE_INFO() << "==========================";
    }

    float getDeviceMemory(int deviceId) {
        if (deviceId == -1)
            return cpuMemory().first;
        for (const auto &device: _gpuList) {
            if (device.index == deviceId)
                return device.totalMemory;
        }
        AIE_CRITICAL() << " No device found for memory detection " << deviceId;
        return 2.0f;
    }

    string getMachineId() { return _machineId; }

    virtual ~SystemInfo() {}

};

}
