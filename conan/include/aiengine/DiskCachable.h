#pragma once
#include "aieGlobal.h"
#include "TemporaryFile.h"
#include <fstream>
#include <vector>
#include <filesystem>

namespace aiutils {
    using namespace std;
    namespace fs = std::filesystem;

    class DiskCachable {
    protected:
        TemporaryFile _file;
    public:


        virtual bool cache() {
            if(isCached())
                return true;
            vector<unsigned char> buffer;
            ofstream file(_file.getTempPath(), ios_base::binary | ios_base::trunc | ios_base::out);
            if(file.is_open() && serialize(buffer)) {
                file.write((const char*)buffer.data(), buffer.size());
                AIE_DEBUG() << " Caching to file: " << _file.getTempPath() << buffer.size();
                return true;
            }
            AIE_CRITICAL() << " Unable to write to cache file: " << _file.getTempPath() << buffer.size();
            return false;
        }

        virtual bool load() {
            if(isCached()) {
                ifstream file(_file.getTempPath(), ios_base::binary | ios_base::ate);
                if(file.is_open()) {
                    std::streamsize size = file.tellg();
                    file.seekg(0, std::ios::beg);
                    std::vector<unsigned char> buffer(size);
                    if (file.read((char*)buffer.data(), size)) {
                        AIE_DEBUG() << " Reading cached file: " << _file.getTempPath() << buffer.size();
                        return deserialize(buffer);
                    }
                }
            }
            return false;
        }

        virtual bool isCached() {
            return fs::exists(fs::path(_file.getTempPath()));
        }

        virtual bool serialize(vector<unsigned char>&) = 0;
        virtual bool deserialize(const vector<unsigned char>& ) = 0;
        virtual ~DiskCachable() {
        }
    };
}
