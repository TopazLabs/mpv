#pragma once
#include "aieGlobal.h"
#include <opencv2/opencv.hpp>
#include <vector>
#include <fstream>
#include "StringUtil.h"

namespace aiutils {
    namespace imgutils {

        using namespace std;
        using namespace cv;

        inline double clampValue(double value, double min, double max) {
            if (value < min) value = min;
            if (value > max) value = max;
            return value;
        }

        inline void NHWC2NCHW(const cv::Mat &img, float *pData) {
            int depth = img.channels(), height = img.rows, width = img.cols, nPixels = width * height;
            vector<cv::Mat> channels;
            split(img, channels);
            for(auto i=0;i<depth;++i) {
                memcpy(pData + nPixels*i, channels[i].data, nPixels*sizeof(float));
            }
        }

        template<class T>
        inline Mat NCHW2NHWC(float *pData, T* pDims, int n) {
            unsigned long width = pDims[n-1], height = pDims[n-2], nPixels = width * height, depth = n > 2 ? pDims[n-3] : 1;
            vector<Mat> channels(depth);
            for(unsigned long i=0;i<depth;++i) {
                channels[i] = Mat(height, width, CV_32FC1, pData + i * nPixels, width * sizeof(float));
            }
            Mat outMat;
            merge(channels, outMat);
            return outMat;
        }

        inline vector<Mat> splitChannels(const vector<Mat> &images) {
            vector<Mat> channels;
            for(auto image : images) {
                vector<Mat> imgs;
                split(image, imgs);
                channels.insert(channels.end(), imgs.begin(), imgs.end());
            }
            return channels;
        }

        inline Mat mergeChannels(const vector<Mat> &images) {
            auto channels = splitChannels(images);
            Mat output;
            merge(channels, output);
            return output;
        }

        inline vector<Mat> mergeChannelsToImages(const vector<Mat> &images, int channelsPerImage = 3) {
            vector<Mat> outputImages;
            auto channels = splitChannels(images);
            if(channels.size() % channelsPerImage != 0)
                return outputImages;
            for(auto i=0;i<(int)channels.size();i+=channelsPerImage) {
                vector<Mat> imageChannels(channels.begin()+i, channels.begin()+i+channelsPerImage);
                Mat output;
                merge(imageChannels, output);
                outputImages.push_back(output);
            }
            return outputImages;
        }

        inline cv::Mat convertDepth(cv::Mat input, int depth) {
            // based on depth values defined in openCV
            const double params[8] = {
                0xFF, 0x7F, 0xFFFF, 0x7FFF, // CV_8U, CV_8S, CV_16U, CV_16S
                0x7FFFFFFF, 1.0, 1.0, 1.0   // CV_32S, CV_32F, CV_64F, CV_16F
            };

            cv::Mat output;
            if (input.empty()) return output;
            double scale = params[depth] / params[input.depth()];
            input.convertTo(output, depth, scale, 0);
            return output;
        }

        inline cv::Mat safeCrop(cv::Mat input, cv::Rect roi) {
            // pad crop if partially out of bounds
            cv::Rect bounds({}, input.size());
            cv::Rect intersection = roi & bounds;
            cv::Rect interROI = intersection - roi.tl();
            if (intersection == roi) return input(intersection);
            if (intersection.empty()) return cv::Mat::zeros(roi.size(), input.type());

            cv::Mat crop(roi.size(), input.type());
            int min_x = interROI.x, max_x = roi.width  - interROI.br().x;
            int min_y = interROI.y, max_y = roi.height - interROI.br().y;
            copyMakeBorder(input(intersection), crop, min_y, max_y, min_x, max_x, cv::BORDER_REFLECT_101);
            return crop;
        }

        inline cv::Mat remosaic(const cv::Mat &image) {
            // convert 8U RGB image to raw model-compatible form
            cv::Mat bayer = cv::Mat(image.size() / 2, CV_MAKETYPE(CV_8U, 4));
            for (int i = 0; i < bayer.rows; ++i) {
                uchar *out = bayer.ptr<uchar>(i);
                const uchar *in0 = image.ptr<uchar>(i * 2 + 0);
                const uchar *in1 = image.ptr<uchar>(i * 2 + 1);
                for (int j = 0; j < bayer.cols; ++j) {
                    out[0] = in0[0], out[1] = in0[4]; // RG
                    out[2] = in1[1], out[3] = in1[5]; // GB
                    out += 4, in0 += 6, in1 += 6;
                }
            }
            return bayer;
        }

        inline bool exportMatToFile(string filepath, const Mat& matrix, string matName = "matrix") {
            AIE_INFO() << " Exporting " << matName.c_str() << " to file: " << filepath;
#ifdef __linux__
            ofstream file(filepath, ios::trunc | ios::out);
#else
            ofstream file(strutils::toWString(filepath), ios::trunc | ios::out);
#endif
            if(file.is_open()) {
                cv::FileStorage cvFile(filepath, cv::FileStorage::WRITE | cv::FileStorage::FORMAT_JSON | cv::FileStorage::MEMORY);
                AIE_INFO() << " Exporting " << matName.c_str() << AIE_LOG_IMAGE(matrix);
                cvFile << matName << matrix;
                string model = cvFile.releaseAndGetString();
                file << model;
                AIE_INFO() << model;
                file.close();
                return true;
            }
            return false;
        }

        inline Mat importMatFromFile(string filepath, string matName = "matrix") {
            AIE_INFO() << " Importing " << matName.c_str() << " to file: " << filepath;
            Mat matrix;
#ifdef __linux__
            ifstream file(filepath, ios::in);
#else
            ifstream file(strutils::toWString(filepath), ios::in);
#endif
            if(file.is_open()) {
                stringstream buffer;
                buffer << file.rdbuf();
                FileStorage file(buffer.str(), FileStorage::READ | FileStorage::FORMAT_JSON | FileStorage::MEMORY);
                file[matName] >> matrix;
                AIE_INFO() << " Imported " << matName.c_str() << AIE_LOG_IMAGE(matrix);
            }
            return matrix;
        }

        inline bool writeImage(string filepath, const Mat& image) {
#ifdef __linux__
            ofstream file(filepath, ios::trunc | ios::binary | ios::out);
#else
            ofstream file(strutils::toWString(filepath), ios::trunc | ios::binary | ios::out);
#endif
            vector<unsigned char> buffer;
            if(cv::imencode(filepath, image, buffer) && file.is_open()) {
                file.write((char*)buffer.data(), buffer.size());
                file.close();
                return true;
            }
            return false;
        }

        inline Mat readImage(string filepath) {
            Mat image;
#ifdef __linux__
            ifstream file(filepath, ios::binary | ios::in);
#else
            ifstream file(strutils::toWString(filepath), ios::binary | ios::in);
#endif
            if(file.is_open()) {
                std::vector<char> buffer(std::istreambuf_iterator<char>(file), {});
                image = cv::imdecode(buffer, cv::IMREAD_UNCHANGED);
                file.close();
            }
            return image;
        }

        inline Mat convertToRGB32F(Mat input) {
             /*
              * CONVERSION_PARAMS is based on depth values defined in openCV
                #define CV_8U   0
                #define CV_8S   1
                #define CV_16U  2
                #define CV_16S  3
                #define CV_32S  4
                #define CV_32F  5
                #define CV_64F  6
                #define CV_16F  7
            */
             const vector<pair<float, float>> CONVERSION_PARAMS = {pair<float, float>(1.0f/255, 0.0f), pair<float, float>(1.0f/255, 0.5f),
                                                                 pair<float, float>(1.0f/65535, 0.0f), pair<float, float>(1.0f/32768, 0.5f), pair<float, float>(1.0f/4294967295.0f, 0.5f),
                                                                 pair<float, float>(1.0f, 0.0f), pair<float, float>(1.0f, 0.0f), pair<float, float>(1.0f, 0.0f)};
             Mat output = input;
             if(input.empty()) {
                 return output;
             }
             auto params = CONVERSION_PARAMS[input.depth()];
             input.convertTo(output, CV_32FC3, params.first, params.second);
             return output;
         }

         inline Mat convertTo8U(const Mat in) {
             double scale = 1.0;
             int type = in.type();
             int outType = type;
             switch (in.type()) {
                 case CV_32FC1:
                     scale = 255.0;
                     outType = CV_8UC1;
                     break;
                 case CV_32FC3:
                     scale = 255.0;
                     outType = CV_8UC3;
                     break;
                 case CV_32FC4:
                     scale = 255.0;
                     outType = CV_8UC4;
                     break;
                 case CV_16UC1:
                     scale = 255.0 / 65535.0;
                     outType = CV_8UC1;
                     break;
                 case CV_16UC3:
                     scale = 255.0 / 65535.0;
                     outType = CV_8UC3;
                     break;
                 case CV_16UC4:
                     scale = 255.0 / 65535.0;
                     outType = CV_8UC4;
                     break;
                 default:
                     AIE_CRITICAL() << "Invalid type";
             }
             Mat out;
             in.convertTo(out, outType, scale, 0.0);
             return out;
         }


        inline void display(const Mat displayFrame, string windowName="frame", int waitDuration=10, string savePath="") {
            auto frame = convertDepth(displayFrame, CV_8U);
            if (savePath != "")
                imwrite(savePath, frame);
            namedWindow(windowName);
            imshow(windowName, frame);
            if (waitKey(waitDuration) == 27) {
                destroyWindow(windowName);
                exit(0);
            }
        }
    }
}
