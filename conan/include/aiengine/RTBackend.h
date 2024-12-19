#pragma once
#include <opencv2/opencv.hpp>
#include <cuda_runtime_api.h>
#include <NvInfer.h>
#include "RTBuffer.h"
#include "ResourceManager.h"
#include "ModelBackend.h"
#include "SystemInfo.h"
#include <map>
#include "RTBuffer.h"
#include <cuda_runtime.h>

namespace aiengine {
    class RTBuffer;
    using namespace std;
    using namespace nvinfer1;

    class RTBackend : public ModelBackend {
        shared_ptr<nvinfer1::IRuntime> _pRuntime;
        shared_ptr<nvinfer1::ICudaEngine> _pEngine;
        unique_ptr<nvinfer1::IExecutionContext> _pContext;
        bool _fixInputNames, _fixOutputNames;
        aiutils::ResourceManager<shared_ptr<RTBuffer>> _resources;
        int _cudaId;
        map<string, RTLayerInfo> _tensorInfos;
        vector<RTLayerInfo> _outputTensorInfos;
        vector<string> _outputNames;

        int getBindingSize(nvinfer1::Dims dims) {
            // get size in bytes (assumes kFloat, no dynamic dims)
            int totalSize = sizeof(float), j = 0;
            while (j < dims.nbDims) totalSize *= dims.d[j++];
            return totalSize;
        }

        bool loadLayerInfo();

        bool createResources(int n = 5);

    public:
        cudaEvent_t m_inference_complete_event, m_upload_complete_event, m_download_complete_event, m_input_consumed, m_output_consumed;
        
        RTBackend() :_pRuntime(nullptr), _pEngine(nullptr), _pContext(nullptr), _pComputeStream(nullptr), _pDownloadStream(nullptr), _pUploadStream(nullptr), _fixInputNames(false), _fixOutputNames(false) {
        }

        virtual bool load(map<string, vector<unsigned char>>& files) override;

        cudaStream_t getComputeStream() const {
            return _pComputeStream;
        }

        cudaStream_t getUploadStream() const {
            return _pUploadStream;
        }

        cudaStream_t getDownloadStream() const {
            return _pDownloadStream;
        }

        shared_ptr<RTBuffer> acquireBuffer() {
            return _resources.acquire();
        }

        void releaseBuffer(shared_ptr<RTBuffer> pBuffer) {
            return _resources.release(pBuffer);
        }

        RTLayerInfo getLayerInfo(const string& name);

        virtual ModelBackend* clone(int) override {
            return nullptr;
        }

        virtual void printDescription() override {
            AIE_INFO() << " Model Info for " << _pEngine->getName();
            for(auto& kv: _tensorInfos)
                AIE_INFO() << kv.second.index << (kv.second.isInput ? " Input: " : " Output: ")
                        << kv.first.c_str()  << " " << kv.second.size  << " " << kv.second.dims.nbDims;
        }

        virtual bool isLoaded() override {
            return _pContext != nullptr;
        }

        std::shared_ptr<ModelOutput> process(std::shared_ptr<ModelInput> pInput) override;
        std::shared_ptr<ModelInput> createInput(unsigned int backendIndex) override;

        void setTargetDevice(int device) override {
            ModelBackend::setTargetDevice(device);
            auto& gpus = aiutils::SystemInfo::GetInstance().getGPUList();
            _cudaId = gpus[_targetDevice].cudaId;
        }

        bool dummyProc() override;

        virtual ~RTBackend() {
            if(isLoaded()) {
                cudaStreamDestroy(_pComputeStream);
                cudaStreamDestroy(_pUploadStream);
                cudaStreamDestroy(_pDownloadStream);
                cudaEventDestroy(m_inference_complete_event);
                cudaEventDestroy(m_upload_complete_event);
                cudaEventDestroy(m_download_complete_event);
                cudaEventDestroy(m_input_consumed);
                cudaEventDestroy(m_output_consumed);
            }
        }

        // ModelBackend interface
    protected:
        cudaStream_t _pComputeStream, _pDownloadStream, _pUploadStream;
    };
}
