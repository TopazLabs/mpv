#pragma once
#include "aieGlobal.h"
#include <memory>
#include <queue>
#include <mutex>
#include <thread>
#include <chrono>
#include "ModelBackend.h"
#include "BlockingQueue.h"
#include "ModelManager.h"
#include "ModelBackendFactory.h"
#include <filesystem>
#include "nlohmann/json.hpp"

namespace aiengine {
    using namespace std;
    using Json = nlohmann::json;

    namespace fs = std::filesystem;

    class BackendRunner {
    protected:
        bool _initialized;
        vector<unique_ptr<ModelBackend>> _pAllBackends;
        aiutils::BlockingQueue<ModelBackend*> _availableBackends;
        string _modelFileName, _modelPath, _modelKey;
        MODEL_BACKEND_TYPE _modelType;
        vector<pair<int,int>> _deviceIds;
        Json _modelBackendInfo, _modelInfo;
        bool _switchInputBackends;
        unsigned int _currentInputBackendIndex = 0;
        mutex _inputIndexMutex;

        ModelBackend* makeModelBackend(int deviceId) {
            auto pBackend = ModelBackendFactory::MakeBackend(_modelType);
            if(pBackend == nullptr) {
                AIE_CRITICAL() << " Unable to create or load model of type " << ModelBackendFactory::NameFor(_modelType).c_str();
                return nullptr;
            }
            pBackend->setModelBackendInfo(_modelBackendInfo);
            pBackend->setTargetDevice(deviceId);
            if(!pBackend->loadFromFile(_modelPath, _modelKey)) {
                delete(pBackend);
                fs::remove(fs::path(_modelPath));
                pBackend = ModelBackendFactory::MakeBackend(_modelType);
                pBackend->setModelBackendInfo(_modelBackendInfo);
                pBackend->setTargetDevice(deviceId);
                if(!ModelManager::GetInstance().downloadModel(_modelInfo, _modelFileName.c_str()) || !pBackend->loadFromFile(_modelPath, _modelKey)) {
                    delete(pBackend);
                    AIE_CRITICAL() << " Unable to create or load model of type " << ModelBackendFactory::NameFor(_modelType).c_str() << " from path: " << _modelFileName;
                    return nullptr;
                }
            }
            return pBackend;
        }

    public:
        BackendRunner(string modelFileName, MODEL_BACKEND_TYPE modelType, const vector<pair<int, int>>& deviceIds, const Json& modelInfo, bool switchInputs = true): _modelFileName(modelFileName),
            _modelType(modelType), _deviceIds(deviceIds), _switchInputBackends(switchInputs) {
            auto backends = modelInfo["backends"];
            auto backendTypeName = ModelBackendFactory::MBNameFor(modelType);
            _modelKey = modelInfo.value("password", MODEL_ENCRYPTION);
            _modelBackendInfo = backends[backendTypeName];
            _modelInfo = modelInfo;
            _modelPath = ModelManager::GetInstance().modelPath(_modelFileName);
        }

        bool load() {
            destroy();
            _pAllBackends.clear();
            AIE_INFO() << " Total Devices: " << _deviceIds.size();
            for(const auto& deviceId: _deviceIds) {
                auto device = deviceId.first;
                auto deviceCount = deviceId.second;
                AIE_INFO() << " DEVICE DETAILS: " << device << " INSTANCES " << deviceCount;
                if (deviceCount == 0)
                    continue;
                int j = 0;
                ModelBackend* pBackend = nullptr;
                while(j<deviceCount) {
                    if (pBackend != nullptr) {
                        _pAllBackends.push_back(unique_ptr<ModelBackend>(pBackend));
                        j++;
                        if(j==deviceCount) {
                            break;
                        }
                    }
                    if(pBackend == nullptr) {
                        pBackend = makeModelBackend(device);
                        if(pBackend == nullptr) {
                            AIE_CRITICAL() << " BackendRunner cannot be initialized for model: " << _modelFileName;
                            return false;
                        }
                    } else {
                        pBackend = pBackend->clone(device);
                    }
                }
            }
            AIE_INFO() << " TOTAL INSTANCES: " << _pAllBackends.size();
            return true;
        }


        bool acquireBuffers() {
            bool status = true;
            for (auto &pBackend : _pAllBackends)
                status &= pBackend->acquireBuffers();
            return status;
        }

        void releaseBuffers() {
            for (auto &pBackend : _pAllBackends)
                pBackend->releaseBuffers();
        }

        void initialize() {
            if (!_initialized) {
                AIE_INFO() << " Init backend runner " << _pAllBackends.size();
                int index = 0;
                for (const auto& pBackend: _pAllBackends) {
                   pBackend->setBackendIndex(_switchInputBackends ? index++ : 0);
                   _availableBackends.push(pBackend.get());
                }
                _initialized = true;
            }
        }

        void destroy() {
            _initialized = false;
            _availableBackends.clear();
        }

        unsigned int getBackendCount() const {
            return _pAllBackends.size();
        }

        shared_ptr<ModelOutput> process(shared_ptr<ModelInput>& pInput) {
            ModelBackend* pBackend = nullptr;
            while (_availableBackends.pop(pBackend)) {
                if(!pBackend)
                    return nullptr;
                if(!_switchInputBackends || pBackend->getBackendIndex() == pInput->getBackendIndex()) {
                    if (_initialized) {
                         auto pOutput = pBackend->process(pInput);
                         _availableBackends.push(pBackend);
                         return pOutput;
                    }
                } else {
                    _availableBackends.push(pBackend);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
            return nullptr;
        }

        virtual shared_ptr<ModelInput> createInput() {
            lock_guard<mutex> lock(_inputIndexMutex);
            if(_switchInputBackends)
                _currentInputBackendIndex = (_currentInputBackendIndex + 1) % _pAllBackends.size();
            return _pAllBackends.size() > _currentInputBackendIndex ? _pAllBackends[_currentInputBackendIndex]->createInput(_currentInputBackendIndex) : nullptr;
        }

        MODEL_BACKEND_TYPE getModelType() { return _modelType; }

        virtual ~BackendRunner() {
            destroy();
            AIE_DEBUG() << " BR removed";
        }
    };

}
