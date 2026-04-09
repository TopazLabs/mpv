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
        string name;
        size_t size;
        Dims dims;
        bool isInput;
    };

    class RTBuffer {
    protected:
        map<string, float*> _pBindings;
        map<string, float*> _pBindingsPinned;
        int _cudaId = 0;
        float *m_tmp_output = nullptr;

    public:

        static bool CudaStatus(cudaError_t code, string msg) {
            if (code == cudaSuccess) return true;
            AIE_CRITICAL() << msg << " : " << cudaGetErrorString(code);
            return false;
        }

        explicit RTBuffer(int cudaId = 0):_cudaId(cudaId) {
            RTSET_CUDA_DEVICE(_cudaId);
        }

        bool createBindings(const map<string, RTLayerInfo>& infos, cudaStream_t stream) {
            RTSET_CUDA_DEVICE(_cudaId);
            for (auto& info: infos) {
                float *addr, *pinned;
                if (!RTCUDA_STATUS(cudaMallocAsync((void**)&addr, info.second.size, stream),
                        "Can't allocate TensorRT binding"))
                    return false;
                if (!RTCUDA_STATUS(cudaMallocHost((void**)&pinned, info.second.size),
                        "Can't allocate TensorRT binding"))
                    return false;
                _pBindings[info.second.name] = addr;
                _pBindingsPinned[info.second.name] = pinned;
            }
            return true;
        }

        bool setImage(const RTLayerInfo& layer, const Mat& input, cudaStream_t uploadStream) {
            RTSET_CUDA_DEVICE(_cudaId);
            memcpy(_pBindingsPinned[layer.name], input.data, layer.size);
            return RTCUDA_STATUS(cudaMemcpyAsync(
                _pBindings[layer.name], _pBindingsPinned[layer.name], layer.size,
                cudaMemcpyHostToDevice, uploadStream), "Can't copy TensorRT input image");
        }

        bool setValue(const RTLayerInfo& layer, float value, cudaStream_t uploadStream) {
            RTSET_CUDA_DEVICE(_cudaId);
            memcpy(_pBindingsPinned[layer.name], &value, sizeof(float));
            return RTCUDA_STATUS(cudaMemcpyAsync(
                _pBindings[layer.name], _pBindingsPinned[layer.name], sizeof(float),
                cudaMemcpyHostToDevice, uploadStream), "Can't copy TensorRT input param");
        }

        bool getOutput(vector<RTLayerInfo>& layers, vector<Mat>& outputs, cudaStream_t downloadStream, cudaEvent_t output_ready_event) {
            RTSET_CUDA_DEVICE(_cudaId);
            for (auto &layer : layers) {
                RTCUDA_STATUS(cudaMemcpyAsync(
                    _pBindingsPinned[layer.name], _pBindings[layer.name], layer.size,
                    cudaMemcpyDeviceToHost, downloadStream), "Can't copy TensorRT output");
            }
            RTCUDA_STATUS(cudaEventRecord(output_ready_event, downloadStream), "checking output ready event");
            RTCUDA_STATUS(cudaStreamSynchronize(downloadStream), "Can't sync TensorRT output");
            int i = 0;
            for (auto &layer : layers)
                memcpy(outputs[i++].data, _pBindingsPinned[layer.name], layer.size); // TODO but how?
            return true;
        }

        const map<string, float*> &getBindings() {
            return _pBindings;
        }

        virtual ~RTBuffer() {
            for (auto &binding : _pBindings)
                cudaFree(binding.second);
            for (auto &binding : _pBindingsPinned)
                cudaFreeHost(binding.second);
            cudaFree(m_tmp_output);
        }
    };
}
