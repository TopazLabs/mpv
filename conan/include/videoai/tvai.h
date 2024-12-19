#ifndef TVAI_H
#define TVAI_H
#include "tvai_data.h"

#ifdef _WIN32
#define DECL_EXPORT __declspec(dllexport)
#else
#define DECL_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif
    /**
     * Sets the logging status for the TVAI library.
     *
     * @param status The logging status to be set. Use 1 to enable logging and 0 to disable logging.
     */
    DECL_EXPORT void tvai_set_logging(int status);
    
    /**
     * @brief Creates a video processor.
     *
     * This function creates a video processor based on the provided `VideoProcessorInfo`.
     *
     * @param pVideoProcessorInfo A pointer to the `VideoProcessorInfo` structure containing the information needed to create the video processor.
     * @return A void pointer to the newly created video processor.
     */
    DECL_EXPORT void* tvai_create(VideoProcessorInfo* pVideoProcessorInfo);
    
    /**
     * @brief Processes the given frame using the specified processor.
     *
     * This function processes the provided frame using the given processor.
     *
     * @param pProcessor A pointer to the processor object.
     * @param pBuffer A pointer to the buffer to be processed.
     *
     * @return An integer value indicating the status of the processing operation.
     *         A non-zero value indicates failure, while a zero value indicates sucess.
     */
    DECL_EXPORT int tvai_process(void* pProcessor, TVAIBuffer* pBuffer);

    /**
     * Retrieves the output size of the stabilization processor.
     *
     * @param pProcessor A pointer to the stabilization processor.
     * @param pWidth     A pointer to an integer to store the output width.
     * @param pHeight    A pointer to an integer to store the output height.
     */
    DECL_EXPORT void tvai_stabilize_get_output_size(void* pProcessor, int* pWidth, int* pHeight);

    /**
     * @brief Sets the device settings for TVAI.
     *
     * This function sets the device settings for TVAI using the provided device string and device settings structure.
     *
     * @param deviceString The device string with Auto: -2, CPU: -1, GPU0: 0, ... or a . separated list of GPU indices e.g. 0.1.3
     * @param pDevice The pointer to the device settings structure.
     * @return An integer value indicating the success or failure of the operation.
     */
    DECL_EXPORT int tvai_set_device_settings(char* deviceString, DeviceSetting* pDevice);

    /**
     * Retrieves a list of visible GPU devices and populates the provided deviceListString with the device information.
     * 
     * @param deviceListString A character array to store the device information.
     * @param n The maximum number of characters to copy into deviceListString.
     * @return The number of visible GPU devices found.
     */
    DECL_EXPORT int tvai_device_list(char *deviceListString, int n);

    /**
     * Checks if model exists.
     * Prints existing models of the given type if the model is not found.
     *
     * @param model A pointer to a character array representing the model name.
     * @param modelType The type of the model.
     * @param modelListString A pointer to a character array to store the list of models.
     * @param n The maximum size of the model list string.
     * @return 0 if model is found, -1 if the user is not logged in, or the number of existing models of the given type if the model is not found.
     */
    DECL_EXPORT int tvai_model_list(char* model, int type, char *modelListString, int n);

    /**
     * Retrieves the list of scales for a given model.
     *
     * @param model The model name.
     * @param scale The scale value to check.
     * @param scaleListString The output string to store the list of scales.
     * @param n The size of the output string.
     * @return The number of scales in the list.
     *         Returns -1 if the model is not found.
     *         Returns 0 if the scale is found in the list.
     *         Returns the number of scales the model supports if the given scale is not found.
     */
    DECL_EXPORT int tvai_scale_list(char* model, int scale, char *scaleListString, int n);

    /**
     * @brief Retrieves the next output frame from the video processor and stores it in the provided buffer.
     * 
     * @param pProcessor A pointer to the video processor object.
     * @param pBuffer A pointer to the buffer where the output frame will be stored.
     * @return 0 if successful, -1 otherwise.
     */
    DECL_EXPORT int tvai_output_frame(void* pProcessor, TVAIBuffer* pBuffer);

    /**
     * @brief Retrieves the parameters values from the video processor and stores it in the provided buffer.
     * 
     * @param pProcessor A pointer to the video processor object.
     * @param pBuffer A pointer to the buffer where the parameters values will be stored.
     * @return 0 if successful, -1 otherwise.
     */
    DECL_EXPORT int tvai_output_params(void* pProcessor, TVAIBuffer* pBuffer);

    /**
     * Returns the number of frames in the output queue.
     *
     * @return The number of frames in the output queue.
     */
    DECL_EXPORT int tvai_output_count(void* pProcessor);

    /**
     * Returns the number of frames expected to be returned
     *
     * @return The number of frames still waiting for processing.
     */
    DECL_EXPORT int tvai_frames_pending(void* pProcessor);

    /**
     * \brief Called to notify the processor that the stream has ended.
     * In most cases, this fun
     *
     * This function is called when the stream has reached its end. It sets the processor `_eof` flag to true,
     */
    DECL_EXPORT void tvai_end_stream(void* pProcessor);

    /**
     * Returns the number of that are yet to be processed.
     *
     * @return The number of frames that are yet to be processed.
     */
    DECL_EXPORT unsigned int tvai_remaining_frames(void* pProcessor);

    /**
     * Updates the parameters of the TVAI processor.
     *
     * This function updates the parameters of the given processor.
     * The updated parameters are provided as an array of floats, with the number of parameters specified by the 'count' parameter.
     *
     * @param pProcessor A pointer to the TVAI processor.
     * @param parameters An array of floats representing the updated parameters.
     * @param count The number of parameters in the 'parameters' array.
     */
    DECL_EXPORT void tvai_update_parameters(void* pProcessor, const DictionaryItem* pParameters, int count);

    /**
     * @brief Destroys the processor.
     *
     * This function is used to destroy the processor and free up allocated resources.
     *
     * @param pProcessor A pointer to the TVAI processor object to be destroyed.
     */
    DECL_EXPORT void tvai_destroy(void* pProcessor);

    /**
     * Waits for the specified number of milliseconds.
     *
     * @param milliseconds The number of milliseconds to wait.
     */
    DECL_EXPORT void tvai_wait(long milliseconds);

    /**
     * Sets the name of a video processor based on the given model.
     * Must be called after tvai_model_list and before tvai_create
     * @param model The model name.
     * @param index Magic number you do not need know abut. Should always be zero for most use cases.
     * @param vpName The output parameter to store the retrieved processor name.
     * @return Returns 1 on sucess, otherwise returns 0.
     */
    DECL_EXPORT int tvai_vp_name(char *model, unsigned int index, char* vpName);

    /**
     * Sets the logging path for the TVAI library.
     *
     * This function allows you to specify the file where you want your TVAI library logs to be written to.
     *
     * @param path The file where logs should be written to.
     */
    DECL_EXPORT void tvai_set_logging_path(const char *path);

    /**
     * Sets folder where model definition files(.jons) and models(.tz) are fetched from.
     * Should be called before anyhing else is done with the TVAI Library. 
     * By default configPath, and dataPath are the same.
     * 
     * @param configPath The path to the folder containing model definitions files, and the authentification file(auth.tpz).
     * @param dataPath The path to the folder where models should be fetched from.
     * @param enableDownload Flag indicating whether model downloads are enabled.
     * @return 1 if the initialization is successful, 0 otherwise.
     */
    DECL_EXPORT int tvai_setup_model_manager(char* configPath, char* dataPath, int enableDownload);

    /**
     * Checks if the user is logged in and is on the latest version of the App.
     *
     * @return 1 if the user is logged in and is on the latest version of the App, 0 otherwise
     */
    DECL_EXPORT int tvai_owns_app();


    /**
     * Disables TensorRT and OpenVINO for OFX plugins.
     */
    DECL_EXPORT void tvai_disable_trt_ov();

    /**
     * Logs usage metrics.
     *
     * @param eventName The name of the event.
     * @param version App version.
     */
    DECL_EXPORT void tvai_report_event(const char* eventName, const char* version);

#ifdef __cplusplus
}
#endif

#endif // TVAI_H
