#pragma once
#include "aieGlobal.h"
#include <string>
#include <opencv2/opencv.hpp>
#include "BufferedModelInput.h"
#include "ModelOutput.h"
#include <string>
#include <map>
#include <vector>
#include "nlohmann/json.hpp"

namespace aiengine {
    using namespace std;
    using namespace cv;
    using Json = nlohmann::json;

    class AIE_EXPORT ModelBackend {
    protected:
        map<string,string> _inputNames;
        bool _valid;
        int _targetDevice;
        unsigned int _backendIndex = 0;

    public:
        ModelBackend():_valid(true), _targetDevice(-1) {}

        virtual bool loadFromFile(string modelPath, string key = "");

        virtual bool load(map<string, vector<unsigned char>> &buffers) = 0;

        virtual bool acquireBuffers() { return true; }

        virtual void releaseBuffers() {};

        virtual bool isLoaded() = 0;

        virtual void setTargetDevice(int targetDevice) {
            _targetDevice = targetDevice;
            AIE_INFO() << " PARAM DEVICE: " << _targetDevice;
        }

        virtual void setModelBackendInfo(const Json &info) {
            if(info.contains("inputs")) {
                auto inputNames = info["inputs"];
                for(auto kv:inputNames.items()) {
                    _inputNames[kv.key()] = kv.value().get<string>();
                }
            }
        }

        virtual void printDescription() = 0;

        virtual bool supportsTarget(int ) { return true; }

        virtual shared_ptr<ModelOutput> process(shared_ptr<ModelInput> pInput) = 0;


        virtual shared_ptr<ModelInput> createInput(unsigned int backendIndex) {
            return shared_ptr<ModelInput>(new BufferedModelInput(backendIndex));
        }

        virtual ModelBackend* clone(int targetDevice) = 0;

        virtual void destroy() {}
        virtual ~ModelBackend() {}
        virtual int getTargetDevice() { return _targetDevice; }

        virtual bool dummyProc() {
            return false;
        }

        static bool IsBackendAvailable(MODEL_BACKEND_TYPE type, int backends) {
            return (backends & (int)type);
        }
        unsigned int getBackendIndex() const { return _backendIndex; }
        void setBackendIndex(unsigned int newBackendIndex) { _backendIndex = newBackendIndex; }
    };
}
