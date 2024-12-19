#pragma once
#include <string>
#include <opencv2/opencv.hpp>
#include "ModelInput.h"
#include "aieGlobal.h"

namespace aiengine {
    using namespace std;
    using namespace cv;

    class BufferedModelInput: public ModelInput {
    protected:
        map<string, Mat> _images;
        map<string, float> _params;
    public:
        BufferedModelInput(unsigned int backendIndex, bool fixName = false):ModelInput(backendIndex, fixName) {}
        virtual ~BufferedModelInput() {}
        virtual void setImage(const string& name, const Mat &image) override {
            _images[name] = image;
        }

        virtual void setValue(const string& name, float param) override {
            _params[name] = param;
        }

        virtual void clear() override {
            ModelInput::clear();
            _images.clear();
            _params.clear();
        }


        virtual void log() override {
            for(auto kv: getImages()) {
                auto image = kv.second;
                AIE_DEBUG() << " Image: " << kv.first.c_str() << " Size: " << image.rows << " " << image.cols << " " << image.channels();
            }
            for(auto kv: getParams()) {
                AIE_DEBUG() << " Param: " << kv.first.c_str() << " == " << kv.second;
            }
        }

        map<string, Mat>& getImages() {return _images;}
        map<string, float>& getParams() {return _params;}
    };
}
