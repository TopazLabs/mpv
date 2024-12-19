#pragma once
#include <string>
#include <opencv2/opencv.hpp>
#include "ModelManager.h"
#include "ModelBackend.h"
#include "tzutils.h"
#include <chrono>
#include "CMInput.h"
#include <filesystem>

AIE_FORWARD_DECLARE_OBJC_CLASS(CMModel);
namespace aiengine {
    using namespace std;
    using namespace aiutils;
    namespace fs = std::filesystem;
    class CMBackend : public ModelBackend {
    protected:
        CMModel *_pModel;
        string _compiledPath;

        virtual bool loadFromCompiledFile(string filepath);

        void cacheCompiledFile(string filepath);
        void createModel();
        string getCompiledPath();
    public:
        CMBackend(): _pModel(nullptr) {}

        virtual ModelBackend* clone(int targetDevice) override;

        virtual bool load(map<string, vector<unsigned char> > &buffers) override;

        virtual bool loadFromFile(string modelPath, string key = "") override;

        bool isLoaded() override;

        virtual shared_ptr<ModelOutput> process(shared_ptr<ModelInput> pInput) override;
        virtual shared_ptr<ModelInput> createInput(unsigned int backendIndex) override;
        virtual bool dummyProc() override;

        virtual void printDescription() override;
        virtual ~CMBackend();
    };
}
