#pragma once
#include <string>
#include <vector>
#include <fstream>
#include "StringUtil.h"

namespace aiutils {
    using namespace std;
    class AIE_EXPORT FileFetcher {
        string _localDirPath;
        string _configDirPath;
        string _remoteDirPath;
        string _remoteHostname;
        bool _useProxy;
        Json _proxy;

        void loadProxySettings() {


        }
    public:
        FileFetcher(string remoteHostname, string remoteDirPath): _remoteDirPath(remoteDirPath), _remoteHostname(remoteHostname), _useProxy(false) {
            AIE_INFO() << " Creating file fetcher for: " << remoteHostname << " REMOTE DIR: " << remoteDirPath;
        }

        virtual ~FileFetcher() {}

        virtual bool verifyLocalFile(string filename, string key);

        string filePath(string filename) {
            return _localDirPath + "/" + filename;
        }

        bool setLocalDirPath(string path);

        void setConfigDirPath(string path) {
            _configDirPath = path;
            auto filepath = _configDirPath + "/proxy.json";
            std::ifstream f(filepath);
            try {
                _proxy = !f.is_open() ? Json() : Json::parse(f);
                _useProxy = !_proxy.is_null() && !_proxy["enable"].empty() && _proxy["enable"].get<bool>();
            } catch (exception& ex) {
                _proxy = Json();
                _useProxy = false;
            }
        }

        string getLocalDirPath() {
            return _localDirPath;
        }

        virtual bool downloadRemoteFileAndVerify(string filename, string key, unsigned int maxAttempts = 3);
    };
}
