#pragma once
#include "aieGlobal.h"
#include "ModelManager.h"
#include <filesystem>

#if !defined(AIE_STATIC)
#define CPPHTTPLIB_OPENSSL_SUPPORT
#endif
#include "httplib.h"

namespace aiutils {
    using namespace std;
    namespace fs = std::filesystem;
    class WebConnectionFactory {
        Json _proxySettings;
        bool _useProxy;

        WebConnectionFactory() {
            auto configDirPath = aiengine::ModelManager::GetInstance().getModelDirPath();
            auto filepath = configDirPath + "/proxy.json";
            if(fs::exists(fs::path(filepath))) {
                std::ifstream f(filepath);
                _proxySettings = !f.is_open() ? Json() : Json::parse(f);
                _useProxy = _proxySettings.value("enabled", false);
                AIE_INFO() << "Proxy Settings: " << _useProxy << " hostname: " << _proxySettings.value("hostname", "") << " port: " << _proxySettings.value("port", 0)
                    << " username: " << _proxySettings.value("username", "") << " JSON: " << _proxySettings;
            } else {
                AIE_INFO() << "No proxy detected";
                _useProxy = false;
            }
        }
    public:
        static WebConnectionFactory& GetInstance() {
            static unique_ptr<WebConnectionFactory> __pWebConnectionFactory(new WebConnectionFactory());
            return *__pWebConnectionFactory;
        }

        shared_ptr<httplib::Client> makeClient(string hostname) {
            shared_ptr<httplib::Client> pClient(new httplib::Client(hostname.c_str()));
            pClient->set_follow_location(true);
#if __APPLE__ && !defined(AIE_STATIC)
            pClient->enable_server_certificate_verification(false);
#endif
            if(_useProxy) {
                pClient->set_proxy(_proxySettings["hostname"].get<string>(), _proxySettings["port"].get<int>());
                if (!_proxySettings["username"].is_null() && !_proxySettings["username"].empty())
                    pClient->set_proxy_basic_auth(_proxySettings["username"].get<string>(), _proxySettings["password"].get<string>());
            }
            return pClient;
        }

    };
}
