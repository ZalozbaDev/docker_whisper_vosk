#ifndef VOSK_RECOGNIZER_H
#define VOSK_RECOGNIZER_H

#include <iostream>

extern "C" {
#include "vosk_api.h"
}

#include <VADWrapper.h>
#include <RecognitionResult.h>
#include <AudioLogger.h>

enum VoskRecognizerState {UNINIT, INIT};

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

	struct whisper_context* ctx;
	
	VADWrapper *vad;
	
	std::vector<std::unique_ptr<RecognitionResult>> partialResult;
	
	std::vector<std::string>                        finalResults;
	
	// to avoid early deletion of string objects, use preallocated memory for the most recent string
	char partialResultBuffer[1000];
	char finalResultBuffer[1000];
	
	void promoteToFinalResult(void);
	
	AudioLogger *audioLogger;
};

#endif // VOSK_RECOGNIZER_H

