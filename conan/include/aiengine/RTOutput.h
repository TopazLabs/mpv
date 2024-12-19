#pragma once
#include "ModelOutput.h"
#include "RTBackend.h"
#include "RTBuffer.h"

namespace aiengine {
    using namespace std;
    using namespace cv;

    class RTOutput: public ModelOutput {
    protected:
        RTBackend* _pBackend;
        shared_ptr<RTBuffer> _pBuffer;
        vector<RTLayerInfo> _layerInfos;
//    cudaStream_t _pStream;

    public:
        RTOutput(RTBackend* pBackend, shared_ptr<RTBuffer> pBuffer, vector<RTLayerInfo> infos): _pBackend(pBackend), _pBuffer(pBuffer), _layerInfos(infos) {
//            RTCUDA_STATUS(cudaStreamCreate(&_pStream), "Can't create CUDA stream");
        }

        virtual bool prepareOutput() {
            _outputs.clear();
            for(auto& layerInfo: _layerInfos) {
                std::vector<int64_t> outDims(layerInfo.dims.d, layerInfo.dims.d + layerInfo.dims.nbDims);
                cv::Mat output = makeOutputMat(outDims, CV_32F);
                _outputs.push_back(output);
            }
            RTCUDA_STATUS(cudaStreamWaitEvent(_pBackend->getDownloadStream(), _pBackend->m_inference_complete_event), "inference complete check");
            _pBuffer->getOutput(_layerInfos, _outputs, _pBackend->getDownloadStream(), _pBackend->m_output_consumed);
            RTCUDA_STATUS(cudaEventRecord(_pBackend->m_download_complete_event, _pBackend->getDownloadStream()), "Cant download record event");
            _pBackend->releaseBuffer(_pBuffer);
            _pBuffer = shared_ptr<RTBuffer>();
            return true;
        }

        virtual ~RTOutput() {
//            cudaStreamDestroy(_pStream);
        }

    };
}
