#ifndef VOSK_RECOGNIZER_H
#define VOSK_RECOGNIZER_H

#include <iostream>
#include <vector>

extern "C" {
#include "vosk_api.h"
}

#include <VADWrapper.h>
#include <RecognitionResult.h>
#include <AudioLogger.h>
extern "C" {
#include "common_audio/signal_processing/include/signal_processing_library.h"
}

#include "whisper.h"

enum VoskRecognizerState {UNINIT, INIT};

// command-line parameters from stream example
struct whisper_params {
    int32_t n_threads  = 8; // TODO hard-coded
    int32_t step_ms    = 3000;
    int32_t length_ms  = 10000;
    int32_t keep_ms    = 200;
    int32_t capture_id = -1;
    int32_t max_tokens = 64;
    int32_t audio_ctx  = 0;

    float vad_thold    = 0.6f;
    float freq_thold   = 100.0f;

    bool speed_up      = false;
    bool translate     = false;
    bool no_fallback   = false;
    bool print_special = false;
    bool no_context    = true;
    bool no_timestamps = false;
    bool tinydiarize   = false;
    bool save_audio    = false; // save audio to wav file
    bool use_gpu       = true;

    std::string language  = "en";
    std::string model     = "models/ggml-base.en.bin";
    std::string fname_out;
};

//////////////////////////////////////////////
class VoskRecognizer
{
public:
	VoskRecognizer(int modelId, float sample_rate, const char *configPath);
	~VoskRecognizer(void);
	int getInstanceId(void) { return m_instanceId; }
	int getModelInstanceId(void) { return m_modelInstanceId; }
	float getSampleRate(void) { return m_inputSampleRate; }
	int acceptWaveform(const char *data, int length);
	void resultCallback(char* word, unsigned int startTimeMs, unsigned int endTimeMs, float negLogLikelihood);
	const char* getPartialResult(void);
	const char* getFinalResult(void);
	
private:
	static const ssize_t m_processingSampleRate = 16000;
	
	static int voskRecognizerInstanceId;

	int m_instanceId;
	int m_modelInstanceId;
	float m_inputSampleRate;
	bool m_libraryLoaded;
	VoskRecognizerState m_recoState;
	
	std::string m_configPath;

	struct whisper_context_params cparams;
	struct whisper_context* ctx;
	const int n_samples_30s  = (1e-3 * 30000.0) * WHISPER_SAMPLE_RATE;
    std::vector<float> pcmf32;
	
	VADWrapper *vad;
	
	WebRtcSpl_State48khzTo16khz m_resamplestate_48_to_16;
	char leftOverData[480*2] = {0};
	int leftOverDataLen = 0;
	
	std::vector<std::unique_ptr<RecognitionResult>> partialResult;
	
	std::vector<std::string>                        finalResults;
	
	// to avoid early deletion of string objects, use preallocated memory for the most recent string
	char partialResultBuffer[1000];
	char finalResultBuffer[1000];
	
	void promoteToFinalResult(void);
	void runWhisper(void);
	
	AudioLogger *audioLogger;
};

#endif // VOSK_RECOGNIZER_H

