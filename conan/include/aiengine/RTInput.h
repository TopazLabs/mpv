#pragma once
#include "ModelInput.h"
#include "RTBackend.h"
#include <cuda_runtime_api.h>
#include <NvInfer.h>

namespace aiengine {
    using namespace std;
    using namespace cv;

    class RTInput: public ModelInput {
    protected:
        RTBackend *_pBackend;
        shared_ptr<RTBuffer> _pBuffer;
        bool _fixOutputName;

        virtual void setImage(const string& name, const Mat &image) override {
            acquire();
            auto info = _pBackend->getLayerInfo(name);
            if(info.index < 0) {
                AIE_CRITICAL() << " Unable to find image input with name: " << name.c_str();
                return;
            }
            RTCUDA_STATUS(cudaStreamWaitEvent(_pBackend->getUploadStream(), _pBackend->m_input_consumed), "checking input consumed");
            _pBuffer->setImage(info, image, _pBackend->getUploadStream());
            RTCUDA_STATUS(cudaEventRecord(_pBackend->m_upload_complete_event, _pBackend->getUploadStream()), "Cant record Upload event");
        }

        virtual void setValue(const string& name, float param) override {
            acquire();
            auto info = _pBackend->getLayerInfo(name);
            if(info.index < 0) {
                AIE_CRITICAL() << " Unable to find image input with name: " << name.c_str();
                return;
            }
            RTCUDA_STATUS(cudaStreamWaitEvent(_pBackend->getUploadStream(), _pBackend->m_input_consumed), "input consumed");
            _pBuffer->setValue(info.index, param, _pBackend->getUploadStream());
            RTCUDA_STATUS(cudaEventRecord(_pBackend->m_upload_complete_event, _pBackend->getUploadStream()), "Cant record Upload event");
        }

    public:
        RTInput(unsigned int backendIndex, RTBackend* pBackend, bool fixInputName = false, bool fixOutputName = false): ModelInput(backendIndex, fixInputName), _pBackend(pBackend), _fixOutputName(fixOutputName) {

        }

        virtual ~RTInput() {}

        virtual void log() override {}
        virtual void clear() override { _outputLayerNames.clear(); }
        void acquire() {
            if(!_pBuffer)
                _pBuffer = _pBackend->acquireBuffer();
        }
        shared_ptr<RTBuffer> getBuffer() { return _pBuffer; }
        void releaseBuffer() { _pBuffer = shared_ptr<RTBuffer>(); }
        virtual void setOutputLayerNames(const vector<string>& outputLayerNames) override {
            _outputLayerNames.clear();
            for(const auto& outputLayerName : outputLayerNames) {
                _outputLayerNames.push_back(_fixOutputName && !aiutils::strutils::endsWith(outputLayerName, ":0") ? (outputLayerName + ":0" ): outputLayerName);
            }
        }
        
    };
}
