#pragma once

#include <string>
#include <chrono>

#include "zip.h"
#include <fstream>
#include <filesystem>

#include <opencv2/opencv.hpp>
#include "ModelBackend.h"
#include "tzutils.h"
#include "ImageUtil.h"
#include "StringUtil.h"
#include "SystemInfo.h"
#include "OVModel.h"
#include "OVInput.h"
#include "OVOutput.h"

namespace aiengine {
    using namespace std;
    using namespace cv;
    using namespace aiutils;

    class OVBackend : public aiengine::ModelBackend {
        int _index;
        aiengine::BufferedModelInput _input;
        ov::InferRequest _inferRequest;
        shared_ptr<OVModel> _pOVModel;

    public:
        OVBackend():_index(0), _pOVModel(new OVModel()), _input(0) {
        }

        OVBackend(shared_ptr<OVModel>& pOVModel):_pOVModel(pOVModel), _input(0) {
            AIE_INFO() << " Shared PTR: " << pOVModel.use_count();
        }
        virtual ~OVBackend() {
            AIE_DEBUG() << " OVB removed";
        }

        bool loadFromFile(string modelPath, string key) override {
            if (strutils::endsWith(modelPath, ".xml")) {
                AIE_INFO() << " Loading raw openvino files from " << modelPath;
                map<string, vector<unsigned char>> buffers;
                vector<unsigned char> buffer;
                if (!tzutils::readUncompressedModel(modelPath, buffer)) {
                    AIE_CRITICAL() << " File not found or is unreadable " << modelPath;
                    return false;
                }
                buffers[modelPath] = buffer;
                auto mPath = strutils::replaceAll(modelPath, ".xml", ".bin");
                if (!tzutils::readUncompressedModel(mPath, buffer)) {
                    AIE_CRITICAL() << " File not found or is unreadable " << mPath;
                    return false;
                }
                buffers[mPath] = buffer;
                return load(buffers);
            }
            return ModelBackend::loadFromFile(modelPath, key);
        }

        bool load(map<string, vector<unsigned char>> &buffers) override {
            return _pOVModel->load(buffers);
        }

        virtual ModelBackend* clone(int) override {
            AIE_DEBUG() << " CLONING OV BACKEND " << _index;
            auto pBackend = new OVBackend(this->_pOVModel);
            pBackend->_inputNames = _inputNames;
            pBackend->_index = _index + 1;
            return pBackend;
        }

        virtual void setTargetDevice(int device) override {
            ModelBackend::setTargetDevice(device);
            _pOVModel->setTargetDevice(device);
        }

        virtual bool supportsTarget(int target) override {
            auto& gpus = aiutils::SystemInfo::GetInstance().getGPUList();
            return target < 0 || (target < (int)gpus.size() && gpus[target].isIntel());
        }

        bool isLoaded() override {
            return _pOVModel->isLoaded();
        }

        virtual void printDescription() override {
            _pOVModel->printDescription();
        }

        shared_ptr<ModelOutput> process(shared_ptr<ModelInput> pInput) override {
            try {
                auto pOVInput = dynamic_pointer_cast<OVInput>(pInput);
                if (!pOVInput->isInit()) {
                    _pOVModel->init(pOVInput->getShapes());
                    pOVInput->setInferRequest(_pOVModel->makeInferRequest());
                    pOVInput->moveValues();
                }
                auto pOVOutput = new OVOutput(pOVInput->getInferRequest(),
                    pInput->getOutputLayerNames(), _pOVModel->shouldTranspose());
                auto pOutput = shared_ptr<ModelOutput>(pOVOutput);
                pOVInput->getInferRequest().start_async();
                pOVInput->setInferRequest(_pOVModel->makeInferRequest());
                return pOutput;
            } catch (const std::exception &error) {
                AIE_CRITICAL() << " OpenVino inference failed: " << error.what();
                return shared_ptr<ModelOutput>();
            }
        }

        shared_ptr<ModelInput> createInput(unsigned int backendIndex) override {
            return shared_ptr<ModelInput>(new OVInput(backendIndex, _pOVModel->shouldFixInputName(), _pOVModel->shouldFixOutputName(), _pOVModel->shouldTranspose()));
        }

        bool dummyProc() override {
            auto pInput = createInput(0);
            for (const auto& [name, shape] : _pOVModel->getMaxShapes())
                pInput->setMat(name, cv::Mat(shape[1], shape[2], CV_32FC(shape[3])));
            pInput->setOutputLayerNames(_pOVModel->getOutputNames());
            auto pOutput = process(pInput);
            return pOutput->prepareOutput();
        }
    };
}
