#pragma once
#include <string>
#include <sstream>
#include <openssl/ssl.h>
#include <openssl/sha.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include "nlohmann/json.hpp"

namespace aiengine {
    using namespace std;
    using Json = nlohmann::json;

    class Crypto {
        string _key, _iv;
        EVP_CIPHER_CTX *_pEncryptContext, *_pDecryptContext;

        void handleErrors(void) {
            ERR_print_errors_fp(stderr);
            abort();
        }

        inline static const vector<char> BASE64_URL_CHARS = {
            'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
            'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
            'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
            'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_'
        };

    public:
        Crypto(string key = "#$%TGBN&*UHBKN*&TFV%~@#WSDCVG(*!", string iv="AUI!_0^&NBGYS12P"):_key(key), _iv(iv), _pEncryptContext(EVP_CIPHER_CTX_new()), _pDecryptContext(EVP_CIPHER_CTX_new()) {
            if(1 != EVP_EncryptInit_ex(_pEncryptContext, EVP_aes_256_cbc(), NULL, (unsigned char*)_key.c_str(), (unsigned char*)_iv.c_str()))
                handleErrors();
            if(1 != EVP_DecryptInit_ex(_pDecryptContext, EVP_aes_256_cbc(), NULL, (unsigned char*)_key.c_str(), (unsigned char*)_iv.c_str()))
                handleErrors();
        }

        vector<unsigned char> encrypt(vector<unsigned char> text) {
            vector<unsigned char> cipherText(text.size()*10);
            int len, cipherTextLen;
            if(1 != EVP_EncryptUpdate(_pEncryptContext, cipherText.data(), &len, text.data(), text.size()))
                handleErrors();
            cipherTextLen = len;
            if(1 != EVP_EncryptFinal_ex(_pEncryptContext, cipherText.data() + len, &len))
                handleErrors();
            cipherTextLen += len;
            cipherText.resize(cipherTextLen);
            return cipherText;
        }

        string decrypt(const vector<unsigned char>& cipherText) {
            vector<unsigned char> plainText(cipherText.size());
            int len, plainTextLen;
            if(1 != EVP_DecryptUpdate(_pDecryptContext, plainText.data(), &len, cipherText.data(), cipherText.size()))
                handleErrors();
            plainTextLen = len;
            if(1 != EVP_DecryptFinal_ex(_pDecryptContext, plainText.data() + len, &len))
                handleErrors();
            plainTextLen += len;
            plainText.resize(plainTextLen);
            plainText.push_back('\0');
            return string((char*)plainText.data());
        }

        vector<unsigned char> encryptObject(const Json& json) {
            auto jsonString = json.dump();
            vector<unsigned char> text(jsonString.begin(), jsonString.end());
            return encrypt(text);
        }

        Json decryptObject(const vector<unsigned char>& cipherText) {
            auto obj = Json::parse(decrypt(cipherText));
            return obj;
        }

        string encodeObject(const Json& object) {
            return encode(encryptObject(object));
        }

        Json decodeObject(const string& text) {
            return decryptObject(decode(text));
        }

        string encode(const vector<unsigned char>& input) {
            vector<char> output;
            for(const auto& in:input) {
                output.push_back('A' + (in/16));
                output.push_back('A' + (in%16));
            }
            output.push_back('\0');
            return string(output.data());
        }

        vector<unsigned char> decode(const string& input) {
            vector<unsigned char> output;
            for(auto i=0;i<input.size();i+=2) {
                output.push_back((input[i] - 'A')*16 + (input[i+1] - 'A'));
            }
            return output;
        }

        static std::string Base64URLEncode(const std::string & in) {
            std::string out;
            int val =0, valb=-6;
            size_t len = in.length();
            unsigned int i = 0;
            for (i = 0; i < len; i++) {
                unsigned char c = in[i];
                val = (val<<8) + c;
                valb += 8;
                while (valb >= 0) {
                    out.push_back(BASE64_URL_CHARS[(val>>valb)&0x3F]);
                    valb -= 6;
                }
            }
            if (valb > -6)
                out.push_back(BASE64_URL_CHARS[((val<<8)>>(valb+8))&0x3F]);
            return out;
        }

        static std::string Base64URLDecode(const std::string & in) {
            std::string out;
            std::vector<int> T(256, -1);
            unsigned int i;
            for (i =0; i < 64; i++) T[BASE64_URL_CHARS[i]] = i;
            int val = 0, valb = -8;
            for (i = 0; i < in.length(); i++) {
                unsigned char c = in[i];
                if (T[c] == -1) break;
                val = (val<<6) + T[c];
                valb += 6;
                if (valb >= 0) {
                    out.push_back(char((val>>valb)&0xFF));
                    valb -= 8;
                }
            }
            return out;
        }

        static vector<unsigned char> SHA256(const string& str) {
            vector<unsigned char> hash(SHA256_DIGEST_LENGTH);
            ::SHA256(reinterpret_cast<const unsigned char*>(str.c_str()), str.size(), hash.data());
            return hash;
        }

        virtual ~Crypto() {
            EVP_CIPHER_CTX_free(_pEncryptContext);
            EVP_CIPHER_CTX_free(_pDecryptContext);
        }
    };
}
