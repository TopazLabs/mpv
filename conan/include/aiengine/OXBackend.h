#pragma once
#include <string>
#include <map>
#include <vector>
#include <opencv2/opencv.hpp>
#include "ModelBackend.h"
#include "onnxruntime_cxx_api.h"
#include "onnxruntime_c_api.h"
//#include <tensorrt_provider_factory.h>
//#include <cuda_provider_factory.h>
#if defined(_WIN32)
#include <dml_provider_factory.h>
#endif
#ifdef __linux__
#include <tensorrt_provider_factory.h>
#include "onnxruntime_session_options_config_keys.h"
#endif
#include "SystemInfo.h"

namespace aiengine {
    using namespace std;
    using namespace cv;
    static const OrtApi* __pOrtApi = OrtGetApiBase()->GetApi(ORT_API_VERSION);

    class OXBackend : public ModelBackend {

        OrtSession* _pSession;
        OrtSessionOptions* _pSessionOptions;
        OrtRunOptions* _pRun;
        OrtCUDAProviderOptionsV2* _pCudaOptions;
        static OrtMemoryInfo* __pMemoryInfo;
        OrtAllocator* _pAllocator;
        vector<string> _outputNames;
        map<string, pair<ONNXTensorElementDataType, vector<int64_t>>> _inputInfo;
        bool _fixInputName, _fixOutputName, _fp16;

        bool setupSessionOptions() {
            AIE_INFO() << " OX with device: " << _targetDevice;
            auto &gpus = aiutils::SystemInfo::GetInstance().getGPUList();
#ifdef __linux__
            //Convert _targetDevice to cuda ID
            int cudaId = gpus[_targetDevice].cudaId;
            std::string optionCudaId = std::to_string(cudaId);

            bool ok = CheckStatus(__pOrtApi->CreateCUDAProviderOptions(&_pCudaOptions), "Create CUDA options");
            if (!ok) {
                return false;
            }
            std::vector<const char*> keys = {"arena_extend_strategy", "device_id", "cudnn_conv_use_max_workspace", "cudnn_conv_algo_search"};
            std::vector<const char*> values = {"kSameAsRequested", optionCudaId.c_str(), "1", "HEURISTIC"};

            ok = CheckStatus(__pOrtApi->UpdateCUDAProviderOptions(_pCudaOptions, keys.data(), values.data(), 4), "Set CUDA options");
            if (!ok) {
                return false;
            }
#endif
            int dxgiId = gpus[_targetDevice].index;
            if(_targetDevice >=0) {
            return CheckStatus(__pOrtApi->CreateSessionOptions(&_pSessionOptions), "Session options: ") &&
                CheckStatus(__pOrtApi->CreateRunOptions(&_pRun), "Run options") &&
                CheckStatus(__pOrtApi->DisableMemPattern(_pSessionOptions), "Mem Pattern Disable: ") &&
#ifdef __linux__
                //CheckStatus(OrtSessionOptionsAppendExecutionProvider_Tensorrt(_pSessionOptions, cudaId), "TensorRT Init") &&
                CheckStatus(__pOrtApi->SessionOptionsAppendExecutionProvider_CUDA_V2(_pSessionOptions, _pCudaOptions), "ONNX CUDA Init") &&
                CheckStatus(__pOrtApi->AddRunConfigEntry(_pRun, kOrtSessionOptionsUseDeviceAllocatorForInitializers, "1"), "ONNX CUDA Allocator Setting: ");
#else
                CheckStatus(OrtSessionOptionsAppendExecutionProvider_DML(_pSessionOptions, dxgiId), "DML Init");
#endif
            } else {
                int tc = aiutils::SystemInfo::GetInstance().threadCount();
                return CheckStatus(__pOrtApi->CreateSessionOptions(&_pSessionOptions), "Session options: ") &&
                        CheckStatus(__pOrtApi->SetInterOpNumThreads(_pSessionOptions, tc/2), "Inter OP CPU Threads: ") &&
                        CheckStatus(__pOrtApi->SetIntraOpNumThreads(_pSessionOptions, tc/4), "Intra OP CPU Threads: ");
            }
        }

        void initModelInfo() {
            size_t count = 0;
            _inputInfo.clear();
            if (CheckStatus(__pOrtApi->SessionGetInputCount(_pSession, &count), "Input Count: ")) {
                for (size_t i = 0; i < count; i++) {
                    char *name;
                    CheckStatus(__pOrtApi->SessionGetInputName(_pSession, i, _pAllocator, (char**)&name), "Input name: ");
                    OrtTypeInfo* pTypeInfo = nullptr;
                    CheckStatus(__pOrtApi->SessionGetInputTypeInfo(_pSession, i, &pTypeInfo), "Input type info: ");
                    const OrtTensorTypeAndShapeInfo* pTensorInfo;
                    CheckStatus(__pOrtApi->CastTypeInfoToTensorInfo(pTypeInfo, &pTensorInfo), "Tensor Info: ");
                    ONNXTensorElementDataType type;
                    CheckStatus(__pOrtApi->GetTensorElementType(pTensorInfo, &type), "Tensor Type: ");
                    size_t n;
                    CheckStatus(__pOrtApi->GetDimensionsCount(pTensorInfo, &n), "Tensor Dims: ");
                    vector<int64_t> dims;
                    dims.resize(n);
                    __pOrtApi->GetDimensions(pTensorInfo, (int64_t*)dims.data(), n);

                    __pOrtApi->ReleaseTypeInfo(pTypeInfo);
                    _inputInfo[string(name)] = pair<ONNXTensorElementDataType, vector<int64_t>>(type,dims);
                    _fixInputName = aiutils::strutils::endsWith(name, ":0");
                    _fp16 = type == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16;
                }
            }
            initOutputNames();
        }

        void initOutputNames() {
            size_t count = 0;
            _outputNames.clear();
            if (CheckStatus(__pOrtApi->SessionGetOutputCount(_pSession, &count), "Output Count: ")) {
                for (size_t i = 0; i < count; i++) {
                    char *name;
                    CheckStatus(__pOrtApi->SessionGetOutputName(_pSession, i, _pAllocator, (char**)&name), "Output name: ");
                    _outputNames.push_back(string(name));
                    _fixOutputName = aiutils::strutils::endsWith(name, ":0");
                }
            }
        }

        static OrtEnv* Env() {
            static OrtEnv* __pEnv = nullptr;
            if(__pEnv == nullptr) {
                AIE_INFO() << " ORT ENV SETUP " << OrtGetApiBase()->GetVersionString();
#ifdef AIE_DETAILED_LOGGING
                OrtGetApiBase()->GetApi(ORT_API_VERSION)->CreateEnv(ORT_LOGGING_LEVEL_VERBOSE, "OXBackend", &__pEnv);
#else
                OrtGetApiBase()->GetApi(ORT_API_VERSION)->CreateEnv(ORT_LOGGING_LEVEL_ERROR, "OXBackend", &__pEnv);
#endif
            }
            //AIE_INFO() << " FETCHING ORT ENV";
            return __pEnv;
        }

    public:

        OXBackend() :_pSession(nullptr), _pSessionOptions(nullptr),_fixInputName(false), _fixOutputName(false), _fp16(false) {}

        virtual bool load(map<string, vector<unsigned char>>& files) override {
            auto buffer = files.begin()->second;
            OrtEnv* env = Env();
            if (!setupSessionOptions()) {
                AIE_CRITICAL() << " ORT Session creation failed";
                return false;
            }
            AIE_DEBUG() << " BUFFER DATA: " << buffer.size();
            if (!CheckStatus(__pOrtApi->CreateSessionFromArray(env, buffer.data(), buffer.size(), _pSessionOptions, &_pSession), "Session creation failed: "))
                return false;
            if (!CheckStatus(__pOrtApi->GetAllocatorWithDefaultOptions(&_pAllocator), "Default Allocator: "))
                return false;
            initModelInfo();
            return true;
        }

        virtual void printDescription() override {
            AIE_INFO() << " Inputs";
            for(auto inp: _inputInfo) {
                AIE_INFO() << inp.first << " type: " << (inp.second.second.size() == 1 ? "param" : "mat");
            }
            AIE_INFO() << " Outputs";
            for(auto oname: _outputNames) {
                AIE_INFO() << oname.c_str();
            }
        }

        virtual bool isLoaded() override {
            return _pSession != nullptr;
        }

        virtual ModelBackend* clone(int) override {
            return nullptr;
        }

        virtual shared_ptr<ModelOutput> process(shared_ptr<ModelInput> pInput) override;

        virtual shared_ptr<ModelInput> createInput(unsigned int backendIndex) override;

        virtual bool dummyProc() override;

        virtual ~OXBackend() {
            if(_pSession)
                __pOrtApi->ReleaseSession(_pSession);
            if(_pSessionOptions)
                __pOrtApi->ReleaseSessionOptions(_pSessionOptions);
            if(_pCudaOptions)
                __pOrtApi->ReleaseCUDAProviderOptions(_pCudaOptions);
            if(_pRun)
                __pOrtApi->ReleaseRunOptions(_pRun);
        }

        static bool CheckStatus(OrtStatus* status, const char* message)
        {
            AIE_DEBUG() << " CHECKING STATUS: " << message;
            if (status != nullptr) {
                const char* msg = __pOrtApi->GetErrorMessage(status);
                AIE_CRITICAL() << " ONNX problem: " << message << msg;
                __pOrtApi->ReleaseStatus(status);
                return false;
            }
            AIE_DEBUG() << " ONNX SUCCESS: " << message;
            return true;
        }

        static bool IsTensor(OrtValue* pTensor) {
            int isTensor;
            CheckStatus(__pOrtApi->IsTensor(pTensor, &isTensor), "Is Tensor float:");
            return isTensor != 0;
        }

        static OrtMemoryInfo* GetMemoryInfo() {
            if(__pMemoryInfo == nullptr) {
                if (!CheckStatus(__pOrtApi->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &__pMemoryInfo), "Memory info creation failed:"))
                    __pMemoryInfo = nullptr;
            }
            return __pMemoryInfo;
        }
    };
}
