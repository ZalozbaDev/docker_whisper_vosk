
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <VoskRecognizer.h>

extern "C" {
#include "vosk_api.h"
}

//////////////////////////////////////////////
class VoskModel
{
public:
	int         instanceId;
	std::string modelPath;
};

static int voskModelInstanceId = 1;

///////////////////////////////////////////////
//
// the vosk server spawns one model only
//
// there is nothing to do here yet, just keep the configred model path
//
//////////////////////////////////////////////
VoskModel *vosk_model_new(const char *model_path)
{
	VoskModel* instance;
	printf("vosk_model_new, path=%s, instance=%d.\n", model_path, voskModelInstanceId);
	
	instance = new VoskModel();
	instance->instanceId = voskModelInstanceId;
	instance->modelPath  = std::string(model_path);
	
	voskModelInstanceId++;
	return instance;
}

///////////////////////////////////////////////
//
// likely to be never called by the server
// 
//////////////////////////////////////////////
void vosk_model_free(VoskModel *model)
{
	printf("vosk_model_free, instance=%d\n", model->instanceId);
	
	delete(model);
	
	voskModelInstanceId--;
}

///////////////////////////////////////////////
//
// every server session creates one recognizer instance
//
// e.g. one conference member == one session == one instance
//
// sample rate is set by the server (and defined as environment on the command line)
//
// TBD: might want to get rid of the fixed 1:1 assignment of recognizers to sessions
// 
//////////////////////////////////////////////
VoskRecognizer *vosk_recognizer_new(VoskModel *model, float sample_rate)
{
	VoskRecognizer* instance;
	
	instance = new VoskRecognizer(model->instanceId, sample_rate, model->modelPath.c_str());
	
	return instance;
}

///////////////////////////////////////////////
//
// this is actually being called if a session ends (e.g. a conference member quits)
//
///////////////////////////////////////////////
void vosk_recognizer_free(VoskRecognizer *recognizer)
{
	delete(recognizer);
}

///////////////////////////////////////////////
void vosk_recognizer_set_max_alternatives(VoskRecognizer *recognizer, int max_alternatives)
{
	// stub
	printf("vosk_recognizer_set_max_alternatives, instance=%d, max_alternatives=%d.\n", recognizer->getInstanceId(), max_alternatives);
}

///////////////////////////////////////////////
void vosk_recognizer_set_words(VoskRecognizer *recognizer, int words)
{
	// stub
	printf("vosk_recognizer_set_words, instance=%d, words=%d.\n", recognizer->getInstanceId(), words);
}

///////////////////////////////////////////////
//
// "main" function that handles almost everything 
// 
//////////////////////////////////////////////
int vosk_recognizer_accept_waveform(VoskRecognizer *recognizer, const char *data, int length)
{
	printf("vosk_recognizer_accept_waveform, instance=%d, modelInstaceId=%d, length=%d, sampleRate=%.2f.\n", recognizer->getInstanceId(), recognizer->getModelInstanceId(), length, recognizer->getSampleRate());
	
	return recognizer->acceptWaveform(data, length);
}

////////////////////////////////////////////////
const char *vosk_recognizer_partial_result(VoskRecognizer *recognizer)
{
	printf("vosk_recognizer_partial_result, instance=%d, modelInstaceId=%d\n", recognizer->getInstanceId(), recognizer->getModelInstanceId());
	
	return recognizer->getPartialResult();
}

////////////////////////////////////////////////
const char *vosk_recognizer_result(VoskRecognizer *recognizer)
{
	printf("vosk_recognizer_result, instance=%d, modelInstaceId=%d\n", recognizer->getInstanceId(), recognizer->getModelInstanceId());

	return recognizer->getFinalResult();
}

////////////////////////////////////////////////
const char *vosk_recognizer_final_result(VoskRecognizer *recognizer)
{
	// no special handling required, just forward
	printf("vosk_recognizer_final_result, instance=%d, modelInstaceId=%d\n", recognizer->getInstanceId(), recognizer->getModelInstanceId());
	return vosk_recognizer_result(recognizer);
}

