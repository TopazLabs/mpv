#pragma once
#include <string>
#include <opencv2/opencv.hpp>
#include "OXBackend.h"

namespace aiengine {
    using namespace std;
    using namespace cv;

    class OXOutput: public ModelOutput {
    protected:
        vector<OrtValue*> _pTensors;
        virtual bool convertToMats(vector<OrtValue*> outputTensors, vector<Mat>& outputs) {
            for(auto& outputTensor : outputTensors) {
                if (OXBackend::IsTensor(outputTensor)) {
                    OrtTensorTypeAndShapeInfo* outShape = nullptr;
                    size_t size = 0;
                    vector<int64_t> dims;
                    if (OXBackend::CheckStatus(__pOrtApi->GetTensorTypeAndShape(outputTensor, &outShape), "Get Shape: ") && OXBackend::CheckStatus(__pOrtApi->GetDimensionsCount(outShape, &size), "Get Size: ")) {
                        dims.resize(size);
                        if (OXBackend::CheckStatus(__pOrtApi->GetDimensions(outShape, dims.data(), size), "Get dims: ")) {
                            ONNXTensorElementDataType type;
                            __pOrtApi->GetTensorElementType(outShape, &type);
                            __pOrtApi->ReleaseTensorTypeAndShapeInfo(outShape);
                            bool isFP16 = ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16 == type;
                            float* floatarr;
                            if (OXBackend::CheckStatus(__pOrtApi->GetTensorMutableData(outputTensor, (void**)&floatarr), "Convert to Mat: ")) {
                                Mat output = makeOutputMat(dims, isFP16 ? CV_16F : CV_32F);
    #ifdef __linux__
                                memcpy(output.data, floatarr, output.elemSize() * output.total());

    #else
                                memcpy_s(output.data, output.elemSize() * output.total(), floatarr, output.elemSize() * output.total());
    #endif
                                if(isFP16)
                                    output.convertTo(output, CV_MAKETYPE(CV_32F, output.channels()));
                                outputs.push_back(output);
                                AIE_DEBUG() << " Output is processed successfully";
                                return true;
                            }
                        }
                    }
                }
            }
            AIE_CRITICAL() << " Output is not a tensor";
            return false;
        }
    public:
        OXOutput(vector<OrtValue*> pTensors = vector<OrtValue*>()): _pTensors(pTensors) {}

        virtual bool prepareOutput() {
            auto ret = convertToMats(_pTensors, _outputs);
            return ret;
        }

        virtual ~OXOutput() {
            for(auto& pTensor : _pTensors)
                __pOrtApi->ReleaseValue(pTensor);
        }

    };
}
