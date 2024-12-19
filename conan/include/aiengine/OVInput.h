#pragma once
#include <openvino/openvino.hpp>
#include "BufferedModelInput.h"
#include "ImageUtil.h"

namespace aiengine {
    class OVInput: public BufferedModelInput {
    protected:
        ov::InferRequest _inferRequest;
        bool _transpose;
        bool _init, _fixOutputName;

        void setImage(const string& name, const Mat& image) override {
            if (_init) {
                float *pData = _inferRequest.get_tensor(name).data<float>();
                if (_transpose) aiutils::imgutils::NHWC2NCHW(image, pData);
                else image.copyTo(cv::Mat(image.size(), image.type(), pData));
            } else {
                BufferedModelInput::setImage(name, image);
            }
        }

        void setValue(const string& name, float param) override {
            if(_init) {
                ov::Tensor input = _inferRequest.get_tensor(name);
                memcpy(input.data<float>(), &param, sizeof(float));
            } else {
                BufferedModelInput::setValue(name, param);
            }
        }

    public:
        OVInput(unsigned int backendIndex, bool fixInputName, bool fixOutputName, bool transpose) :
            BufferedModelInput(backendIndex, fixInputName), _transpose(transpose), _init(false), _fixOutputName(fixOutputName) {}

        virtual ~OVInput() {}

        void log() override {}
        void setInferRequest(ov::InferRequest inferRequest) { _inferRequest = inferRequest; }
        ov::InferRequest getInferRequest() { return _inferRequest; }
        bool isInit() const { return _init; }

        std::map<std::string, ov::PartialShape> getShapes() {
            std::map<std::string, ov::PartialShape> shapes;
            for (const auto& [name, image] : _images) {
                if (_transpose) shapes[name] = { 1, image.channels(), image.rows, image.cols };
                else shapes[name] = { 1, image.rows, image.cols, image.channels() };
            }
            return shapes;
        }

        virtual void setOutputLayerNames(const vector<string>& outputLayerNames) override {
            _outputLayerNames.clear();
            for(const auto& outputLayerName : outputLayerNames) {
                string name = _fixOutputName && !aiutils::strutils::endsWith(outputLayerName, ":0") ? (outputLayerName + ":0" ) : outputLayerName;
                _outputLayerNames.push_back(name);
            }
        }

        void moveValues() {
            _init = true;
            for(auto& kv: _images)
                setImage(kv.first, kv.second);
            for(auto& kv: _params)
                setValue(kv.first, kv.second);
            _images.clear();
            _params.clear();
        }
    };
}
