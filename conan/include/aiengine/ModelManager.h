#pragma once
#include <vector>
#include <string>
#include "aieGlobal.h"
#include "FileFetcher.h"

#define MODEL_ENCRYPTION "a0331add513e4e6fae035cb12e4b11e5"

namespace aiengine {
    using namespace std;
    using namespace aiutils;

    typedef enum {
        LICENSE_TYPE_NONE,
        LICENSE_TYPE_STANDARD,
        LICENSE_TYPE_PROFESSIONAL,
        LICENSE_TYPE_ENTERPRISE,
        LICENSE_TYPE_CLOUD,
        LICENSE_TYPE_AMI,
        LICENSE_TYPE_CUSTOM
    } LICENSE_TYPE;

    class AIE_EXPORT ModelManager {
        static unique_ptr<ModelManager> __pModelManager;
        string _modelDirPath;
        map<string, Json> _modelsInfo;
        string _uniqueField;
        bool _appendVersion;
        bool _allowDownloads;
        FileFetcher _fileFetcher;
        int _waitAndDownload;

    protected:
        ModelManager(string uniqueField, bool appendVersion, bool allowDownloads, string remoteHostname, string remoteDirPath);
        bool loadModelInfoMap(string modelDirPath);
        bool isModelValid(Json& model);

    public:

        static ModelManager& MakeInstance(string uniqueField, bool appendVersion, bool allowDownloads, string remoteHostname, string remoteDirPath) {
            __pModelManager.reset(new ModelManager(uniqueField, appendVersion, allowDownloads, remoteHostname, remoteDirPath));
            return *__pModelManager;
        }

        static ModelManager& GetInstance() {
            return *__pModelManager;
        }

        static bool IsInitialized() {
            return __pModelManager != nullptr;
        }

        map<string, Json>& getModelsInfo() { return _modelsInfo; }
        Json& getModelInfo(const string& name) { return _modelsInfo[name]; }
        bool modelInfoExists(const string& name) { return _modelsInfo.find(name) != _modelsInfo.end(); }

        static bool isBackendAvailable(MODEL_BACKEND_TYPE type, int backends);

        virtual string modelPath(string modelName) {
            return _fileFetcher.filePath(modelName);
        }

        virtual bool modelFileExists(const Json& modelInfo, string filename) {
            return _fileFetcher.verifyLocalFile(filename, modelInfo["password"].get<string>());
        }

        virtual bool downloadModelAndVerify(const Json& modelInfo, string filename);

        virtual void setAllowDownloads(bool status) {
            AIE_INFO() << " CAN DOWNLOAD MODELS: " << status;
            _allowDownloads = status;
        }

        bool allowDownloads() { return _allowDownloads; }

        virtual bool setLocalDirPath(const string& path) {
            return _fileFetcher.setLocalDirPath(path);
        }

        virtual void setConfigDirPath(const string& path) {
            _fileFetcher.setConfigDirPath(path);
        }

        virtual string getLocalDirPath() {
            return _fileFetcher.getLocalDirPath();
        }

        virtual bool downloadModel(const Json& modelInfo, string filename) {
            return downloadModelAndVerify(modelInfo, filename);
        }

        string computeFilename(string format, string shortName, string version,
              int capability, int width, int height, unsigned int scale, bool forceNaming = true);

        static std::string printKeys(const Json &modelsInfo) {
            std::string namesStr = "";
            for (const auto &item : modelsInfo.items())
                namesStr += item.key() + " ";
            return namesStr;
        }

        bool setModelDirPath(string configPath, string dataPath = "") {
            if(dataPath == "")
                dataPath = configPath;
            if(_modelDirPath != configPath) {
                _modelDirPath = configPath;
                AIE_INFO() << " ModelManager: setting modeldirPath: " << _modelDirPath;
                setConfigDirPath(configPath);
                if(setLocalDirPath(dataPath)) {
                    if(!loadModelInfoMap(_modelDirPath)) {
                        AIE_CRITICAL() << " No models found in directory: " << configPath;
                        return false;
                    }
                } else {
                    AIE_CRITICAL() << " Model directory not writable: " << configPath;
                    return false;
                }
            }
            return true;
        }

        string cacheDir(string name);

        string getModelDirPath() const { return _modelDirPath; }

        virtual ~ModelManager() {}
    };
}
