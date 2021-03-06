#ifndef DEEPSPEECH_H
#define DEEPSPEECH_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SWIG
    #if defined _MSC_VER
        #define DEEPSPEECH_EXPORT __declspec(dllexport)
    #else
        #define DEEPSPEECH_EXPORT __attribute__ ((visibility("default")))
    #endif /*End of _MSC_VER*/
#else
    #define DEEPSPEECH_EXPORT
#endif

typedef struct ModelState ModelState;

typedef struct StreamingState StreamingState;

/**
 * @brief Stores each individual character, along with its timing information
 */
typedef struct MetadataItem {
  /** The character generated for transcription */
  char* character;

  /** Position of the character in units of 20ms */
  int timestep;

  /** Position of the character in seconds */
  float start_time;
} MetadataItem;

/**
 * @brief Stores the entire CTC output as an array of character metadata objects
 */
typedef struct Metadata {
  /** List of items */
  MetadataItem* items;
  /** Size of the list of items */
  int num_items;
  /** Approximated confidence value for this transcription. This is roughly the
   * sum of the acoustic model logit values for each timestep/character that
   * contributed to the creation of this transcription.
   */
  double confidence;
} Metadata;

enum DeepSpeech_Error_Codes
{
    // OK
    DS_ERR_OK                 = 0x0000,

    // Missing invormations
    DS_ERR_NO_MODEL           = 0x1000,

    // Invalid parameters
    DS_ERR_INVALID_ALPHABET   = 0x2000,
    DS_ERR_INVALID_SHAPE      = 0x2001,
    DS_ERR_INVALID_SCORER     = 0x2002,
    DS_ERR_MODEL_INCOMPATIBLE = 0x2003,
    DS_ERR_SCORER_NOT_ENABLED = 0x2004,

    // Runtime failures
    DS_ERR_FAIL_INIT_MMAP     = 0x3000,
    DS_ERR_FAIL_INIT_SESS     = 0x3001,
    DS_ERR_FAIL_INTERPRETER   = 0x3002,
    DS_ERR_FAIL_RUN_SESS      = 0x3003,
    DS_ERR_FAIL_CREATE_STREAM = 0x3004,
    DS_ERR_FAIL_READ_PROTOBUF = 0x3005,
    DS_ERR_FAIL_CREATE_SESS   = 0x3006,
    DS_ERR_FAIL_CREATE_MODEL  = 0x3007,
};

/**
 * @brief An object providing an interface to a trained DeepSpeech model.
 *
 * @param aModelPath The path to the frozen model graph.
 * @param[out] retval a ModelState pointer
 *
 * @return Zero on success, non-zero on failure.
 */
DEEPSPEECH_EXPORT
int DS_CreateModel(const char* aModelPath,
                   ModelState** retval);

/**
 * @brief Get beam width value used by the model. If {@link DS_SetModelBeamWidth}
 *        was not called before, will return the default value loaded from the
 *        model file.
 *
 * @param aCtx A ModelState pointer created with {@link DS_CreateModel}.
 *
 * @return Beam width value used by the model.
 */
DEEPSPEECH_EXPORT
unsigned int DS_GetModelBeamWidth(ModelState* aCtx);

/**
 * @brief Set beam width value used by the model.
 *
 * @param aCtx A ModelState pointer created with {@link DS_CreateModel}.
 * @param aBeamWidth The beam width used by the model. A larger beam width value
 *                   generates better results at the cost of decoding time.
 *
 * @return Zero on success, non-zero on failure.
 */
DEEPSPEECH_EXPORT
int DS_SetModelBeamWidth(ModelState* aCtx,
                         unsigned int aBeamWidth);

/**
 * @brief Return the sample rate expected by a model.
 *
 * @param aCtx A ModelState pointer created with {@link DS_CreateModel}.
 *
 * @return Sample rate expected by the model for its input.
 */
DEEPSPEECH_EXPORT
int DS_GetModelSampleRate(ModelState* aCtx);

/**
 * @brief Frees associated resources and destroys model object.
 */
DEEPSPEECH_EXPORT
void DS_FreeModel(ModelState* ctx);

/**
 * @brief Enable decoding using an external scorer.
 *
 * @param aCtx The ModelState pointer for the model being changed.
 * @param aScorerPath The path to the external scorer file.
 *
 * @return Zero on success, non-zero on failure (invalid arguments).
 */
DEEPSPEECH_EXPORT
int DS_EnableExternalScorer(ModelState* aCtx,
                            const char* aScorerPath);

/**
 * @brief Disable decoding using an external scorer.
 *
 * @param aCtx The ModelState pointer for the model being changed.
 *
 * @return Zero on success, non-zero on failure.
 */
DEEPSPEECH_EXPORT
int DS_DisableExternalScorer(ModelState* aCtx);

/**
 * @brief Set hyperparameters alpha and beta of the external scorer.
 *
 * @param aCtx The ModelState pointer for the model being changed.
 * @param aAlpha The alpha hyperparameter of the decoder. Language model weight.
 * @param aLMBeta The beta hyperparameter of the decoder. Word insertion weight.
 *
 * @return Zero on success, non-zero on failure.
 */
DEEPSPEECH_EXPORT
int DS_SetScorerAlphaBeta(ModelState* aCtx,
                          float aAlpha,
                          float aBeta);

/**
 * @brief Use the DeepSpeech model to perform Speech-To-Text.
 *
 * @param aCtx The ModelState pointer for the model to use.
 * @param aBuffer A 16-bit, mono raw audio signal at the appropriate
 *                sample rate (matching what the model was trained on).
 * @param aBufferSize The number of samples in the audio signal.
 *
 * @return The STT result. The user is responsible for freeing the string using
 *         {@link DS_FreeString()}. Returns NULL on error.
 */
DEEPSPEECH_EXPORT
char* DS_SpeechToText(ModelState* aCtx,
                      const short* aBuffer,
                      unsigned int aBufferSize);

/**
 * @brief Use the DeepSpeech model to perform Speech-To-Text and output metadata 
 * about the results.
 *
 * @param aCtx The ModelState pointer for the model to use.
 * @param aBuffer A 16-bit, mono raw audio signal at the appropriate
 *                sample rate (matching what the model was trained on).
 * @param aBufferSize The number of samples in the audio signal.
 *
 * @return Outputs a struct of individual letters along with their timing information. 
 *         The user is responsible for freeing Metadata by calling {@link DS_FreeMetadata()}. Returns NULL on error.
 */
DEEPSPEECH_EXPORT
Metadata* DS_SpeechToTextWithMetadata(ModelState* aCtx,
                                      const short* aBuffer,
                                      unsigned int aBufferSize);

/**
 * @brief Create a new streaming inference state. The streaming state returned
 *        by this function can then be passed to {@link DS_FeedAudioContent()}
 *        and {@link DS_FinishStream()}.
 *
 * @param aCtx The ModelState pointer for the model to use.
 * @param[out] retval an opaque pointer that represents the streaming state. Can
 *                    be NULL if an error occurs.
 *
 * @return Zero for success, non-zero on failure.
 */
DEEPSPEECH_EXPORT
int DS_CreateStream(ModelState* aCtx,
                    StreamingState** retval);

/**
 * @brief Feed audio samples to an ongoing streaming inference.
 *
 * @param aSctx A streaming state pointer returned by {@link DS_CreateStream()}.
 * @param aBuffer An array of 16-bit, mono raw audio samples at the
 *                appropriate sample rate (matching what the model was trained on).
 * @param aBufferSize The number of samples in @p aBuffer.
 */
DEEPSPEECH_EXPORT
void DS_FeedAudioContent(StreamingState* aSctx,
                         const short* aBuffer,
                         unsigned int aBufferSize);

/**
 * @brief Compute the intermediate decoding of an ongoing streaming inference.
 *
 * @param aSctx A streaming state pointer returned by {@link DS_CreateStream()}.
 *
 * @return The STT intermediate result. The user is responsible for freeing the
 *         string using {@link DS_FreeString()}.
 */
DEEPSPEECH_EXPORT
char* DS_IntermediateDecode(StreamingState* aSctx);

/**
 * @brief Signal the end of an audio signal to an ongoing streaming
 *        inference, returns the STT result over the whole audio signal.
 *
 * @param aSctx A streaming state pointer returned by {@link DS_CreateStream()}.
 *
 * @return The STT result. The user is responsible for freeing the string using
 *         {@link DS_FreeString()}.
 *
 * @note This method will free the state pointer (@p aSctx).
 */
DEEPSPEECH_EXPORT
char* DS_FinishStream(StreamingState* aSctx);

/**
 * @brief Signal the end of an audio signal to an ongoing streaming
 *        inference, returns per-letter metadata.
 *
 * @param aSctx A streaming state pointer returned by {@link DS_CreateStream()}.
 *
 * @return Outputs a struct of individual letters along with their timing information. 
 *         The user is responsible for freeing Metadata by calling {@link DS_FreeMetadata()}. Returns NULL on error.
 *
 * @note This method will free the state pointer (@p aSctx).
 */
DEEPSPEECH_EXPORT
Metadata* DS_FinishStreamWithMetadata(StreamingState* aSctx);

/**
 * @brief Destroy a streaming state without decoding the computed logits. This
 *        can be used if you no longer need the result of an ongoing streaming
 *        inference and don't want to perform a costly decode operation.
 *
 * @param aSctx A streaming state pointer returned by {@link DS_CreateStream()}.
 *
 * @note This method will free the state pointer (@p aSctx).
 */
DEEPSPEECH_EXPORT
void DS_FreeStream(StreamingState* aSctx);

/**
 * @brief Free memory allocated for metadata information.
 */
DEEPSPEECH_EXPORT
void DS_FreeMetadata(Metadata* m);

/**
 * @brief Free a char* string returned by the DeepSpeech API.
 */
DEEPSPEECH_EXPORT
void DS_FreeString(char* str);

/**
 * @brief Return version of this library. The returned version is a semantic version
 *        (SemVer 2.0.0). The string returned must be freed with {@link DS_FreeString()}.
 *
 * @return The version string.
 */
DEEPSPEECH_EXPORT
char* DS_Version();

#undef DEEPSPEECH_EXPORT

#ifdef __cplusplus
}
#endif

#endif /* DEEPSPEECH_H */
