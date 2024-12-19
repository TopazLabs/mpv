#pragma once
#include "BufferedModelInput.h"
#include "aieGlobal.h"

AIE_FORWARD_DECLARE_OBJC_CLASS(NSMutableDictionary);
AIE_FORWARD_DECLARE_OBJC_CLASS(NSMutableArray);
namespace aiengine {
    using namespace std;
    using namespace cv;

    class CMInput: public BufferedModelInput {
    protected:
        NSMutableDictionary *_inputs;
        NSMutableArray *_pOutputNames;
        virtual void setImage(const string& name, const Mat &image) override;
        virtual void setValue(const string& name, float param) override;
    public:
        CMInput(unsigned int backendIndex, bool fixName = false);
        virtual ~CMInput() {}
        virtual void log() override;
        virtual void clear() override;
        NSMutableArray* getOutputNames() { return _pOutputNames; }
        NSMutableDictionary* getInputDict() { return _inputs; }
        virtual void setOutputLayerNames(const vector<string>& outputLayerNames) override;

    };
}
