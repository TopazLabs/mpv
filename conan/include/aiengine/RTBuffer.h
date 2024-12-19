#pragma once
#include "aieGlobal.h"
#include <vector>
#include "opencv2/opencv.hpp"
#include <cuda_runtime_api.h>
#include <NvInfer.h>

#define RTCUDA_STATUS(cmd, msg) RTBuffer::CudaStatus(cmd, msg)
#define RTSET_CUDA_DEVICE(ID) RTCUDA_STATUS(cudaSetDevice(ID), to_string(ID) + " Unable to set cuda device")

namespace aiengine {
    using namespace std;
    using namespace cv;
    using namespace nvinfer1;

    struct RTLayerInfo{
        int index;
        size_t size;
        Dims dims;
        bool isInput;
    };

    class RTBuffer {
    protected:
        vector<float*> _pBindings;
        vector<float*> _pBindingsPinned;
        int _cudaId;
        float *m_tmp_output = nullptr;

    public:

        static bool CudaStatus(cudaError_t code, string msg) {
            if (code == cudaSuccess) return true;
            AIE_CRITICAL() << msg << " : " << cudaGetErrorString(code);
            return false;
        }

        RTBuffer(int cudaId = 0):_cudaId(cudaId) {
            RTSET_CUDA_DEVICE(_cudaId);
        }

        bool createBindings(const map<string, RTLayerInfo>& infos, cudaStream_t stream) {
            RTSET_CUDA_DEVICE(_cudaId);
            _pBindings.resize(infos.size(), nullptr);
            _pBindingsPinned.resize(infos.size(), nullptr);
            for (auto& info: infos) {
                if (!RTCUDA_STATUS(cudaMallocAsync((void**)&_pBindings[info.second.index], info.second.size, stream)
                                           , "Can't allocate TensorRT binding"))
                    return false;
                if (!RTCUDA_STATUS(cudaMallocHost((void**)&_pBindingsPinned[info.second.index], info.second.size)
                                           , "Can't allocate TensorRT binding"))
                    return false;
            }
            return true;
        }

        bool setImage(RTLayerInfo& layer, const Mat& input, cudaStream_t uploadStream) {
            RTSET_CUDA_DEVICE(_cudaId);
            memcpy(_pBindingsPinned[layer.index], input.data, layer.size);
            return RTCUDA_STATUS(cudaMemcpyAsync(_pBindings[layer.index], _pBindingsPinned[layer.index], layer.size, cudaMemcpyHostToDevice, uploadStream)
                                      , "Can't copy TensorRT input image");
        }

        bool setValue(int index, float value, cudaStream_t uploadStream) {
            RTSET_CUDA_DEVICE(_cudaId);
            memcpy(_pBindingsPinned[index], &value, sizeof(float));
            return RTCUDA_STATUS(cudaMemcpyAsync(_pBindings[index], _pBindingsPinned[index], sizeof(float), cudaMemcpyHostToDevice, uploadStream)
                                  , "Can't copy TensorRT input param");
        }

        bool getOutput(vector<RTLayerInfo>& layers, vector<Mat>& outputs, cudaStream_t downloadStream, cudaEvent_t output_ready_event) {
            RTSET_CUDA_DEVICE(_cudaId);
            for(auto &layer: layers) {
                RTCUDA_STATUS(cudaMemcpyAsync(_pBindingsPinned[layer.index], _pBindings[layer.index], layer.size, cudaMemcpyDeviceToHost, downloadStream)
                                       , "Can't copy TensorRT output");
            }
            RTCUDA_STATUS(cudaEventRecord(output_ready_event, downloadStream), "checking output ready event");
            RTCUDA_STATUS(cudaStreamSynchronize(downloadStream), "Can't sync TensorRT output");
            int i =0;
            for(auto &layer: layers) {
                memcpy(outputs[i++].data, _pBindingsPinned[layer.index], layer.size); // TODO but how?
            }
            return true;
        }

        void** getBindings() { return (void**) _pBindings.data(); }
        virtual ~RTBuffer() {
            for (int i = 0; i < _pBindings.size(); ++i) {
                cudaFree(_pBindings[i]);
                cudaFreeHost(_pBindingsPinned[i]);
            }
            cudaFree(m_tmp_output);
        }
    };
}
