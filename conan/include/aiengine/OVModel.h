#pragma once
#include <opencv2/opencv.hpp>
#include <openvino/openvino.hpp>
#include "StringUtil.h"
#include "SystemInfo.h"
#ifdef __linux__
#include <iterator>
#endif

namespace aiengine {
    using namespace std;
    using namespace cv;
    using namespace aiutils;

    class OVModel {
        int _targetDevice;
        std::map<std::string, ov::PartialShape> _shapes;
        std::string_view _weights;
        std::shared_ptr<ov::Model> _network;
        ov::CompiledModel _executableNetwork;
        ov::Core _core;
        atomic_bool _init;
        mutex _mutex;
        bool _shouldFixInputName, _shouldFixOutputName;
        bool _shouldTranspose;
        vector<string> _outputNames;

        void loadModelToPlugin() {
            try {
                auto deviceStr = deviceString();
                AIE_INFO() << " OpenVino device string is " << deviceStr.c_str()  << " index " << _targetDevice;
                _executableNetwork = _core.compile_model(_network, deviceStr);
                AIE_INFO() << " - Loaded network successfully for deviceid " << deviceStr.c_str() << " index " << _targetDevice;
            }
            catch (const std::exception &error)
            {
                AIE_CRITICAL() << " - Running CPU fallback because of this error [ " << error.what() << " ] ";
                _executableNetwork = _core.compile_model(_network, "CPU");
                AIE_INFO() << " - Loaded with CPU ";
            }
            uint32_t nireq = _executableNetwork.get_property(ov::optimal_number_of_infer_requests);
            AIE_INFO() << " ExeNetwork optimal requests: " << nireq;
        }

        string allDeviceString() {
            vector<string> gpuNames;
            vector<string> devices = _core.get_available_devices();
            for (const string &device : devices) {
                if (strutils::contains(device, "GPU")) {
                    gpuNames.push_back(device);
                }
            }
            if(gpuNames.size() > 1) {
                ostringstream gpuString;
                std::reverse(gpuNames.begin(),gpuNames.end());
                copy(gpuNames.begin(), gpuNames.end(), ostream_iterator<string>(gpuString, ","));
                auto gpuStr = gpuString.str();
                AIE_INFO() << " Using multiple OV GPUs: " << gpuStr.c_str();
                return string(strutils::getEnv("OV_USE_MLS_AUTO") == "1" ? "AUTO:" : "MULTI:") + gpuStr.substr(0, gpuStr.length()-1);
            } else if(gpuNames.size() == 1) {
                AIE_INFO() << " Using single OV GPU: " << gpuNames[0].c_str();
                return gpuNames[0];
            }
            return "CPU";
        }

        string deviceString() {
            auto& gpus = aiutils::SystemInfo::GetInstance().getGPUList();
            vector<string> gpuNames;
            AIE_DEBUG() << " Parameters: device = " << _targetDevice << gpus.size();
            if(_targetDevice == (int)gpus.size()) {
                return allDeviceString();
            } else if(_targetDevice >= 0 && gpus[_targetDevice].isIntel()) {
                if (strutils::getEnv("OV_USE_DEVICE_INDEX") == "1") {
                    AIE_INFO() << " OV device index selected: " << _targetDevice;
                    return "GPU." + to_string(_targetDevice);
                }

                std::vector<std::string> devices = _core.get_available_devices();
                for (const string &device : devices) {
                    auto gpuName = strutils::toLower(strutils::trim(_core.get_property(device, ov::device::full_name)));
                    auto targetName = strutils::toLower(strutils::trim(gpus[_targetDevice].name));
                    if(strutils::contains(gpuName, targetName) || strutils::contains(targetName, gpuName)) {
                        AIE_INFO() << " OV matched: " << gpuName << gpus[_targetDevice].name.c_str();
                        return device;
                    } else
                        AIE_INFO() << " OV checked: " << gpuName << gpus[_targetDevice].name.c_str();
                }
            }

            AIE_INFO() << " OV device selection: CPU " << _core.get_property("CPU", ov::device::full_name);
            return "CPU";
        }

    public:
        OVModel():_init(false), _shouldFixInputName(false), _shouldFixOutputName(false),_shouldTranspose(false) {}

        void init(std::map<std::string, ov::PartialShape> shapes) {
            lock_guard<mutex> lock(_mutex);
            if(!_init) {
                AIE_INFO() << " OVM INITING";
                _network->reshape(shapes);
                loadModelToPlugin();
                _init = true;
                AIE_INFO() << " OVM FINISHED INITING";
            }
        }

        bool shouldFixInputName() {
            return _shouldFixInputName;
        }

        bool shouldFixOutputName() {
            return _shouldFixOutputName;
        }

        bool shouldTranspose() {
            return _shouldTranspose;
        }

        virtual void printDescription() {
            AIE_INFO() << " Network Name: " << _network->get_name().c_str() << " Size: " << _network->get_graph_size();
            AIE_INFO() << " Transpose: " <<  _shouldTranspose;

            const auto &inputs = _network->inputs();
            AIE_INFO() << " Inputs: " << inputs.size();
            for(const auto &input : inputs)
                AIE_INFO() << " Input name: " << input.get_any_name();

            const auto &outputs = _network->outputs();
            AIE_INFO() << " Outputs: " << outputs.size();
            for(const auto &output : outputs)
                AIE_INFO() << " Output name: " << output.get_any_name();
        }

        bool load(map<string, vector<unsigned char>> &buffers) {
            try {
                if (buffers.size() == 1 && strutils::endsWith(buffers.begin()->first, ".onnx")) {
                    const auto [name, buf] = *buffers.begin();
		    auto onnx = string(buf.begin(), buf.end());
                    _network = _core.read_model(onnx, ov::Tensor());
                    _shouldTranspose = false;
                } else if (buffers.size() == 2) {
                    const auto [name0, buf0] = *buffers.begin();
                    const auto [name1, buf1] = *++buffers.begin();
                    if (strutils::endsWith(name0, ".xml")) {
                        auto xml = string(buf0.begin(), buf0.end());
                        _weights = string(buf1.begin(), buf1.end());
                        uint8_t *wPtr = (uint8_t*)&_weights[0];
                        auto binBlob = ov::Tensor(ov::element::u8, { _weights.size() }, wPtr);
                        _network = _core.read_model(xml, binBlob);
                        _shouldTranspose = true;
                    } else {
                        auto xml = string(buf0.begin(), buf0.end());
                        _weights = string(buf0.begin(), buf0.end());
                        uint8_t *wPtr = (uint8_t*)&_weights[0];
                        auto binBlob = ov::Tensor(ov::element::u8, { _weights.size() }, wPtr);
                        _network = _core.read_model(xml, binBlob);
                        _shouldTranspose = true;
                    }
                } else {
                    AIE_CRITICAL() << " Unable to find OV model in buffers";
                    return false;
                }
                if(!isLoaded()) {
                    AIE_CRITICAL() << " Error loading network/weights from buffers for openvino";
                    return false;
                }

                ov::preprocess::PrePostProcessor ppp(_network);
                for(const auto &input : _network->inputs()) {
                    auto name = input.get_any_name();
                    if (strutils::endsWith(name, ":0")) _shouldFixInputName = true;
                    auto &in = ppp.input(input.get_any_name());
                    in.tensor().set_element_type(ov::element::f32);
                }
                for(const auto &output : _network->outputs()) {
                    auto name = output.get_any_name();
                    if (strutils::endsWith(name, ":0")) _shouldFixOutputName = true;
                    auto &out = ppp.output();
                    out.tensor().set_element_type(ov::element::f32);
                }
                _network = ppp.build();
                printDescription();
            } catch(exception ex) {
                AIE_CRITICAL() << " Parsing open vino model failed with error: " << ex.what();
                return false;
            }
            return true;
        }

        void setTargetDevice(int device) {
            _targetDevice = device;
        }

        bool isLoaded() {
            return _network->get_graph_size() > 0;
        }

        ov::InferRequest makeInferRequest() {
            return _executableNetwork.create_infer_request();
        }

        std::map<std::string, ov::Shape> getMaxShapes() {
            std::map<std::string, ov::Shape> shapes;
            for (const auto &input : _network->inputs()) {
                auto s = input.get_partial_shape().get_max_shape();
                while (s.size() < 4) s.push_back(1);
                if (_shouldTranspose) s = {s[0],s[2],s[3],s[1]};
                shapes[input.get_any_name()] = s;
            }
            return shapes;
        }

        bool isInit() const { return _init; }

        vector<string> getOutputNames() {
            if(_outputNames.empty()) {
                for(const auto& output : _network->outputs()) {
                    _outputNames.push_back(output.get_any_name());
                }
            }
            return _outputNames;
        }

        virtual ~OVModel() {
            AIE_DEBUG() << " OVM removed";
        }
    };

}
