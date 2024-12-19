#pragma once
#include <filesystem>
#include <chrono>
#include <thread>
#include <random>
#include "ModelManager.h"

namespace aiutils {
    using namespace std;
    namespace fs = std::filesystem;

    class TemporaryFile {
        fs::path _tempPath;
        string randomFilename() {
            string filename = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
            random_device rd;
            mt19937 g(rd());
            shuffle(filename.begin(), filename.end(), g);
            return filename.substr(0, 15);
        }
    public:
        TemporaryFile(string extension = "") {
            _tempPath = fs::temp_directory_path().string() + "/" + randomFilename() + (extension.empty() ? string("") : (string(".") + extension));
            AIE_DEBUG() << "temp file path " << _tempPath << " absolute path " << getTempPath();
        }

        string getTempPath() { return fs::absolute(_tempPath).string(); }

        virtual ~TemporaryFile() {
            while(fs::exists(_tempPath) && !fs::remove(_tempPath)) {
                this_thread::sleep_for(chrono::milliseconds(200));
            }
        }
    };
}
