#ifndef TVAI_DATA_H
#define TVAI_DATA_H
#define TVAI_PROCESSOR_NAME_SIZE 32

#ifdef __cplusplus
extern "C" {
#endif
    /**
     * @brief Enumeration of different model types.
     */
    typedef enum {
        ModelTypeNone,                   /**< No model type specified. */
        ModelTypeUpscaling = 1,          /**< Model type for upscaling. */
        ModelTypeFrameInterpolation = 2, /**< Model type for frame interpolation. */
        ModelTypeParameterEstimation = 3,/**< Model type for parameter estimation. */
        ModelTypeCamPoseEstimation = 4,  /**< Model type for camera pose estimation. */
        ModelTypeStabilization = 5,      /**< Model type for stabilization. */
        ModelTypeShotDetection = 6,
        ModelTypeCount                   /**< Count of model types. */
    } ModelType;

    /**
     * @brief Enumeration of different pixel formats.
     */
    typedef enum {
        TVAIPixelFormatNone,        /**< No pixel format specified. */
        TVAIPixelFormatRGB8,        /**< 8-bit RGB pixel format. */
        TVAIPixelFormatRGB16,       /**< 16-bit RGB pixel format. */
        TVAIPixelFormatARGB8,       /**< 8-bit ARGB pixel format. */
        TVAIPixelFormatARGB16,      /**< 16-bit ARGB pixel format. */
        TVAIPixelFormatRGBA8,       /**< 8-bit RGBA pixel format. */
        TVAIPixelFormatRGBA16,      /**< 16-bit RGBA pixel format. */
        TVAIPixelFormatRGBA32F,     /**< 32-bit floating point RGBA pixel format. */
        TVAIPixelFormatVFRGBA32F    /**< 32-bit floating point RGBA pixel format with variable frame rate. */
    } TVAIPixelFormat;

    /**
     * @brief Structure to hold device settings.
     */
    typedef struct {
        int index;               /**< Bitwise flag for index of the GPU device (e.g. 3 would mean use GPU 1 and 2), CPU is -1, ALL GPU is GPU count and Auto is -2 */
        unsigned int extraThreadCount; /**< Number of extra threads to use. */
        double maxMemory;           /**< Maximum memory to use. */
        int useMultipleDevices;     /**< Flag to indicate if multiple devices are to be used. */
    } DeviceSetting;

    /**
     * @brief Structure to hold a dictionary item.
     */
    typedef struct {
        char *pKey;                 /**< Pointer to the key string. */
        char *pValue;               /**< Pointer to the value string. */
    } DictionaryItem;

    /**
     * @brief Structure to hold basic processor information.
     */
    typedef struct {
        char *modelName;            /**< Name of the model. */
        char processorName[TVAI_PROCESSOR_NAME_SIZE]; /**< Name of the processor. */
        int inputWidth;             /**< Input width of the video. */
        int inputHeight;            /**< Input height of the video. */
        int scale;                  /**< Scale factor. */
        TVAIPixelFormat pixelFormat;/**< Pixel format of the video. */
        double timebase;            /**< Timebase of the video. */
        double framerate;           /**< Framerate of the video. */
        DeviceSetting device;    /**< Information about of device to use for processing. */
        int preflight;              /**< Preflight check flag. */
        int canDownloadModel;       /**< Flag indicating if the model can be downloaded. */
        DictionaryItem *pParameters;/**< Pointer to an array of parameters. */
        int parameterCount;         /**< Number of parameters in the array. */
    } BasicProcessorInfo;

    /**
     * @brief Structure to hold video processor information.
     */
    typedef struct {
        BasicProcessorInfo basic;   /**< Basic processor information. */
        int outputWidth;            /**< Output width of the video. */
        int outputHeight;           /**< Output height of the video. */
        int frameCount;             /**< Number of frames processed. */
    } VideoProcessorInfo;

    /**
     * @brief Structure to hold buffer information for TVA processing.
     */
    typedef struct {
        unsigned char *pBuffer;     /**< Pointer to the buffer containing video data. */
        long lineSize;               /**< The size of a line in the buffer. */
        long long pts;              /**< Presentation timestamp of the buffer. */
        long frameNo;               /**< Frame number of the buffer. */
        long long duration;         /**< Duration of the buffer in time units. */
    } TVAIBuffer;

#ifdef __cplusplus
}
#endif

#endif // TVAI_DATA_H
