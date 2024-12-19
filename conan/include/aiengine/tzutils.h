#pragma once
#include "aieGlobal.h"
#include <vector>
#include <map>
#include <fstream>
#include <iostream>

namespace aiutils {
    namespace tzutils {
        AIE_EXPORT bool readCompressedModel(std::string modelPath, std::vector<unsigned char> &buffer, std::string key = "");
        AIE_EXPORT bool readUncompressedModel(std::string modelPath, std::vector<unsigned char> &buffer);
        AIE_EXPORT int readCompressedFiles(std::string modelPath, std::map<std::string, std::vector<unsigned char>> &files, std::string key = "", bool validate = true);
        AIE_EXPORT bool saveEncryptedFile(std::map<std::string, std::vector<unsigned char>> &files, std::string path, std::string key="");
        AIE_EXPORT std::shared_ptr<std::map<std::string, std::vector<unsigned char>>> readCachedCompressedFiles(std::string modelPath, std::string key = "");
        AIE_EXPORT std::string bufferToFile(std::vector<unsigned char> &buffer, std::string path);
        AIE_EXPORT bool verifyCompressedBuffer(std::string filename, const std::vector<unsigned char> &buffer, std::string key = "", bool checkDetails = true);

        AIE_EXPORT std::vector<std::string> uncompressZip(std::string zipPath, std::string destPath, std::string key = "");
        AIE_EXPORT std::string readZipEntry(std::string zipPath, std::string ext, std::string key = "");
    };
}
