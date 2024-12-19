#pragma once
#include <string>
#include <opencv2/opencv.hpp>

namespace aiengine {
    using namespace std;
    using namespace cv;

    class ModelOutput {
    protected:
        string _error;
        vector<Mat> _outputs;

        virtual Mat makeOutputMat(vector<int64_t>& dims, int cvType = CV_32F) {
            Mat image;
            switch(dims.size()) {
            case 1:
                image = Mat(1, dims[0], CV_MAKE_TYPE(cvType, 1));
                break;
            case 2:
                image = Mat(dims[0], dims[1], CV_MAKE_TYPE(cvType, 1));
                break;
            case 3:
                image = Mat(dims[0], dims[1], CV_MAKE_TYPE(cvType, dims[2]));
                break;
            default:
                image = Mat(dims[1], dims[2], CV_MAKE_TYPE(cvType, dims[3]));
            }
            return image;
        }

    public:
        ModelOutput() {}
        virtual bool prepareOutput() { return true; }
        virtual vector<Mat>& getResults() { return _outputs; }
        virtual Mat getResult(unsigned int index = 0) { return (index < _outputs.size()) ? _outputs[index] : Mat(); }
        virtual bool hasResults() { return !_outputs.empty(); }
        void setError(const string& errorMsg) { _error = errorMsg; }
        string getError() {return _error;}
        bool hasError() { return !_error.empty(); }

        virtual ~ModelOutput() {}

    };
}
