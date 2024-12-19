#pragma once

#if defined(_MSC_VER) && !defined(AIE_STATIC)
#  if defined(AIE_LIB)
#    define AIE_EXPORT __declspec(dllexport)
#  else
#    define AIE_EXPORT __declspec(dllimport)
#  endif
#elif defined(__linux__) && !defined(AIE_STATIC)
#  if defined(AIE_LIB)
#    define AIE_EXPORT __attribute__((visibility("default")))
#  else
#    define AIE_EXPORT
#  endif
#else
#    define AIE_EXPORT
#endif

#ifndef AIE_FORWARD_DECLARE_OBJC_CLASS
#  ifdef __OBJC__
#    define AIE_FORWARD_DECLARE_OBJC_CLASS(classname) @class classname
#  else
#    define AIE_FORWARD_DECLARE_OBJC_CLASS(classname) class classname
#  endif
#endif

#include "Logger.h"
#include <nlohmann/json.hpp>

using Json = nlohmann::json;

#define AIE_LOG() aiutils::Logger::Log()

#ifdef AIE_DETAILED_LOGGING
    #define AIE_DEBUG() AIE_LOG() << " DEBUG: "
    #define AAIPRINT_DURATION(msg, var, code) auto var = std::chrono::high_resolution_clock::now(); code; AIE_DEBUG() << msg << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - var).count()/1000 << " ms";
#else
    #define AIE_DEBUG() if(false) std::cerr
    #define AAIPRINT_DURATION(msg, var, code) code;
#endif

#define AIE_LOG_IMAGE(img) img.rows << "x" << img.cols << ":" << img.channels()
#define AIE_INFO() AIE_LOG() << " INFO: "
#define AIE_WARNING() AIE_LOG() << " WARNING: "
#define AIE_CRITICAL() AIE_LOG() << " CRITICAL: "
#define SS << " " <<

#define AIPRINT_DURATION(msg, var, code) auto var = std::chrono::high_resolution_clock::now(); code; AIE_INFO() << msg << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - var).count()/1000 << " ms";

#define AIE_TRACE_TIME(name, code) \
    uint64_t name = aieGetUs(); code; \
    AIE_DEBUG() << #name": " << (aieGetUs() - name) / 1000.0 << " ms";

#define AIE_INFO_TIME(name, code) \
    uint64_t name = aieGetUs(); code; \
    AIE_INFO() << #name": " << (aieGetUs() - name) / 1000.0 << " ms";

AIE_EXPORT uint64_t aieGetUs();

namespace aiengine {

// moved out of ModelBackend to seperate
// noinfer from opencv/inference code
enum MODEL_BACKEND_TYPE {
    MB_NONE       = 0x0,
    MB_DNN        = 0x1,
    MB_TENSORFLOW = 0x2,
    MB_OPENVINO   = 0x4,
    MB_COREML     = 0x8,
    MB_ONNX16     = 0x10,
    MB_ONNX       = 0x20,
    MB_OPENVINO8  = 0x40,
    MB_OPENVINO16 = 0x80,
    MB_TENSORRT   = 0x100
};

AIE_EXPORT MODEL_BACKEND_TYPE TypeFor(const std::string &name);
AIE_EXPORT std::string MBNameFor(MODEL_BACKEND_TYPE type);
AIE_EXPORT int AllBackends();

}
