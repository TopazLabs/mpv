#pragma once
#include "ModelOutput.h"
#include "aieGlobal.h"

AIE_FORWARD_DECLARE_OBJC_CLASS(MLDictionaryFeatureProvider);
AIE_FORWARD_DECLARE_OBJC_CLASS(NSMutableArray);
namespace aiengine {
    using namespace std;
    using namespace cv;

    class CMOutput: public ModelOutput {
    protected:
    MLDictionaryFeatureProvider *_pOutput;
    NSMutableArray *_pOutputNames;
    public:
        CMOutput(MLDictionaryFeatureProvider *pOutput, NSMutableArray* pOutputNames):_pOutput(pOutput), _pOutputNames(pOutputNames) {}

        virtual bool prepareOutput() override;
        virtual ~CMOutput() {}

    };
}
