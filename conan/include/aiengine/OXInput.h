#pragma once
#include "ModelInput.h"
#include <opencv2/opencv.hpp>
#include "OXBackend.h"
#include "StringUtil.h"

namespace aiengine {
    using namespace std;
    using namespace cv;

    class OXInput: public ModelInput {
    protected:
        bool _isFP16, _fixOutputName;
        vector<const OrtValue*> _values;
        vector<const char*> _names;
        vector<const char*> _outputNamesPtrs;
        vector<vector<char>> _outputNamesBuffer;
        vector<vector<char>> _namesBuffer;
        map<string, int> _nameMapping;
        vector<Mat> _valuesBuffer;

        bool isTensor(OrtValue* pTensor) {
            int isTensor;
            OXBackend::OXBackend::CheckStatus(__pOrtApi->IsTensor(pTensor, &isTensor), "Is Tensor float:");
            return isTensor != 0;
        }

        void setInputTensor(string name, const Mat& data, vector<int64_t>& dims, bool isFP16) {
            if(_nameMapping.find(name) == _nameMapping.end()) {
                _nameMapping[name] = _namesBuffer.size();
                _namesBuffer.push_back(vector<char>(name.length()+1));
                std::copy(name.begin(), name.end()+1, _namesBuffer.back().begin());
                _names.push_back(_namesBuffer.back().data());
                _values.push_back(nullptr);
                _valuesBuffer.push_back(Mat());
            }
            int index = _nameMapping[name];
            OrtValue* pTensor = nullptr;
            _valuesBuffer[index] = data;
            AIE_DEBUG() << " input tensor info " << AIE_LOG_IMAGE(data) << " total: " << data.total() << " elemsize: " << data.elemSize() << " type " << data.type() << CV_16F << CV_32F << isFP16;
            if(!OXBackend::CheckStatus(__pOrtApi->CreateTensorWithDataAsOrtValue(OXBackend::GetMemoryInfo(), (void*)_valuesBuffer[index].data, data.total()*data.elemSize(), dims.data(), dims.size(),
                                                                  isFP16 ? ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16 : ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
                                                                  &pTensor), "Tensor value float:")) {
                AIE_CRITICAL() << " Unable to set input tensor for " << name.c_str();
                return;
            }
            if (_values[index] != nullptr)
                __pOrtApi->ReleaseValue((OrtValue*)_values[index]);
            _values[index] = pTensor;
            AIE_DEBUG() << " Done setting input tensor " << name.c_str() << data.size() << dims.size() << index << isFP16;
        }

        virtual void setValue(const string& name, float param) override {
            Mat input(1,1,CV_32FC1, Scalar(param)), image;
            input.convertTo(image, CV_MAKETYPE(_isFP16 ? CV_16F : CV_32F, image.channels()));
            vector<int64_t> dims = {1};
            setInputTensor(name, image, dims, _isFP16);
        }

        virtual void setImage(const string& name, const Mat& input) override {
            vector<int64_t> dims = { 1, input.rows, input.cols, input.channels() };
            if(_isFP16) {
                Mat image;
                AAIPRINT_DURATION("AAAAA conversion time", tt, input.convertTo(image, CV_MAKETYPE(CV_16F, input.channels()));)
                setInputTensor(name, image, dims, _isFP16);
            } else
                setInputTensor(name, input, dims, _isFP16);
        }

    public:
        OXInput(unsigned int backendIndex, bool fixInputNames = false, bool fixOutputName = false, bool isFP16 = false): ModelInput(backendIndex, fixInputNames), _fixOutputName(fixOutputName), _isFP16(isFP16) {
            AIE_DEBUG() << " OXInput " << backendIndex << fixInputNames << fixOutputName << _isFP16;
        }

        virtual ~OXInput() {
            OXInput::clear();
        }

        virtual void log() override {
            for(int i=0;i<_values.size();i++) {
                AIE_DEBUG() << " Input " << i << " : " << _names[i] << " = " << _values[i];
            }
        }


        vector<const OrtValue*> getValues() { return _values; }
        vector<const char*> getNames() { return _names; }
        vector<const char*>& getOutputLayerNamesPtr() { return _outputNamesPtrs; }
        virtual void setOutputLayerNames(const vector<string>& outputLayerNames) override {
            _outputLayerNames.clear();
            _outputNamesBuffer.clear();
            _outputNamesPtrs.clear();
            for(const auto& outputLayerName : outputLayerNames) {
                string name = _fixOutputName && !aiutils::strutils::endsWith(outputLayerName, ":0") ? (outputLayerName + ":0" ) : outputLayerName;
                _outputLayerNames.push_back(name);
                _outputNamesBuffer.push_back(vector<char>(name.length()+1));
                std::copy(name.begin(), name.end()+1, _outputNamesBuffer.back().begin());
                _outputNamesPtrs.push_back(_outputNamesBuffer.back().data());
            }
        }

        virtual void clear() override {
            for(auto pInput: _values) {
                if(pInput != nullptr)
                    __pOrtApi->ReleaseValue((OrtValue*)pInput);
            }
        }

    };
}
