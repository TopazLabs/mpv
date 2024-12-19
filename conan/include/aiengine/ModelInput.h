#pragma once
#include "StringUtil.h"
#include <string>
#include <opencv2/opencv.hpp>

namespace aiengine {
    using namespace std;
    using namespace cv;

    class ModelInput {
    protected:
        unsigned int _backendIndex;
        bool _fixName;
        vector<string> _outputLayerNames;

        virtual string fixLayerName(const string& name) {
            return _fixName && !aiutils::strutils::endsWith(name, ":0") ? (name + ":0" ) : name;
        }
        virtual void setImage(const string& name, const Mat &image) = 0;
        virtual void setValue(const string& name, float param) = 0;

    public:
        ModelInput(unsigned int backendIndex, bool fixName = false): _backendIndex(backendIndex), _fixName(fixName) {

        }
        virtual ~ModelInput() {}

        virtual void setMat(const string& name, const Mat &image) {
            setImage(fixLayerName(name), image);
        }

        virtual void setParameter(const string& name, float param) {
            setValue(fixLayerName(name), param);

        }
        virtual void log() = 0;
        virtual void clear() { _outputLayerNames.clear(); }

        vector<string> getOutputLayerNames() const { return _outputLayerNames; }
        virtual void setOutputLayerNames(const vector<string>& outputLayerNames) {
            _outputLayerNames.clear();
            for(auto& layerName: outputLayerNames) {
                _outputLayerNames.push_back(fixLayerName(layerName));
            }
        }
        void setFixName(bool fixName) { _fixName = fixName; }
        unsigned int getBackendIndex() const { return _backendIndex; }
    };

}
