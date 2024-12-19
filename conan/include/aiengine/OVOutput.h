#pragma once
#include <openvino/openvino.hpp>
#include "ModelOutput.h"
#include "ImageUtil.h"

namespace aiengine {
    class OVOutput : public ModelOutput {
    protected:
        ov::InferRequest _inferRequest;
        vector<string> _outputNames;
        bool _transposed;

    public:
        OVOutput(ov::InferRequest inferRequest, vector<string> outputNames, bool transposed)
          : _inferRequest(inferRequest), _outputNames(outputNames), _transposed(transposed) {}

        bool prepareOutput() {
            using aiutils::imgutils::NCHW2NHWC;
            _inferRequest.wait(), _outputs.clear();
            for (const auto& name: _outputNames) {
                ov::Tensor output = _inferRequest.get_tensor(name);
                float *pData = output.data<float>();
                auto size = output.get_shape();

                int n = size.size(), h = n > 2 ? size[n-3] : 1, w = size[n-2], c = size[n-1];
                if (_transposed) _outputs.push_back(NCHW2NHWC(pData, size.data(), n));
                else _outputs.push_back(cv::Mat(h, w, CV_32FC(c), pData).clone());
            }
            return true;
        }
    };
}
