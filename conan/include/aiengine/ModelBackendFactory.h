#pragma once
#include "aieGlobal.h"
#include "ModelBackend.h"

namespace aiengine {

    class AIE_EXPORT ModelBackendFactory
    {
    protected:
        static int __allBackends;
        static map<string, MODEL_BACKEND_TYPE> __backendNameMapping;
        static map<MODEL_BACKEND_TYPE, string> __backendNames;
        static vector<MODEL_BACKEND_TYPE> __allBackendTypes;
        static vector<MODEL_BACKEND_TYPE> __availableBackendTypes;
        ModelBackendFactory() {}
    public:
        static ModelBackend* MakeBackend(string type);
        static ModelBackend* MakeBackend(MODEL_BACKEND_TYPE type);
        static MODEL_BACKEND_TYPE TypeFor(string type);
        static string NameFor(MODEL_BACKEND_TYPE type);
        static string MBNameFor(MODEL_BACKEND_TYPE type);
        static bool IsBackendAvailable(MODEL_BACKEND_TYPE type);
        static vector<MODEL_BACKEND_TYPE> AllModelBackendTypes() { return __allBackendTypes; }
        static vector<MODEL_BACKEND_TYPE> AvailableBackendTypes();
        static vector<string> AvailableBackendTypeStrings();
        static int AllBackends() { return __allBackends; }
        static int MBCount() { return __backendNames.size() - 1;}
    };
}
