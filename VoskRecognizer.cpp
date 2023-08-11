
#include <VoskRecognizer.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>

#include <cassert>

#include "common.h"
#include "whisper.h"

int VoskRecognizer::voskRecognizerInstanceId = 1;

// command-line parameters from stream example
struct whisper_params {
    int32_t n_threads  = std::min(4, (int32_t) std::thread::hardware_concurrency());
    int32_t step_ms    = 3000;
    int32_t length_ms  = 10000;
    int32_t keep_ms    = 200;
    int32_t capture_id = -1;
    int32_t max_tokens = 32;
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

    std::string language  = "en";
    std::string model     = "models/ggml-base.en.bin";
    std::string fname_out;
};

//////////////////////////////////////////////
VoskRecognizer::VoskRecognizer(int modelId, float sample_rate, const char *configPath)
{
	char status;
	
	std::cout << "vosk_recognizer_new, instance=" << voskRecognizerInstanceId << " sample_rate=" << sample_rate << std::endl;

	m_modelInstanceId = modelId;
	m_instanceId      = voskRecognizerInstanceId++;
	m_inputSampleRate = sample_rate;
	
	m_libraryLoaded   = false;
	
	m_recoState = VoskRecognizerState::UNINIT;
	
	m_configPath = std::string(configPath);
}

//////////////////////////////////////////////
VoskRecognizer::~VoskRecognizer(void)
{
	char status;
	
	std::cout << "vosk_recognizer_free, instance=" << m_instanceId << std::endl;
	
	delete(audioLogger);
	
	whisper_free(ctx);
	
	m_recoState = VoskRecognizerState::UNINIT;
	
	delete(vad);
	
	partialResult.clear();
	finalResults.clear();
	
	// don't decrease, let every instance get a unique ID
	// voskRecognizerInstanceId--;
}

//////////////////////////////////////////////
int VoskRecognizer::acceptWaveform(const char *data, int length)
{
	int status;
	bool noMoreData;
	char* dsData;
	
	// if not yet initalized, do that here and discard this audio
	if (m_recoState == VoskRecognizerState::UNINIT)
	{
		char initStatus;

		// whisper init
		ctx = whisper_init_from_file(m_configPath.c_str());

		vad = new VADWrapper(3, m_processingSampleRate);
		
		audioLogger = new AudioLogger(std::string("/logs/"), m_instanceId);
		
		m_recoState = VoskRecognizerState::INIT;
		
		return 0;
	}
	
	if ((m_inputSampleRate == 48000) && (m_processingSampleRate == 16000))
	{
		// only 48kHz-->16kHz is supported (both VAD and recognizer)
		// e.g. Jitsi provides 48 kHz so we need to downsample 1:3
		int ctr = 0;
		dsData = new char[length / 3];
		while (ctr < length)
		{
			dsData[ctr / 3]       = data[ctr];
			dsData[(ctr / 3) + 1] = data[ctr + 1];
			ctr += 6;
		}
	}
	else
	{
		std::cout << "Unsupported sampling rates input " << m_inputSampleRate << " Hz and processing " << m_processingSampleRate << "Hz." << std::endl;
		assert(false);	
	}
	
	// push all data into VADWrapper
	status = vad->process(m_processingSampleRate, (const int16_t*) dsData, (length / 6));
	
	if (status == -1)
	{
		std::cout << "VAD processing error!" << std::endl;	
	}
	
	delete(dsData);
	
	noMoreData = vad->analyze();
	
	while (noMoreData == false)
	{
		unsigned int availableChunks = vad->getAvailableChunks();
		VADWrapperState uttStatus = vad->getUtteranceStatus();
		
		while (availableChunks > 0)
		{
			std::unique_ptr<VADFrame<VADWrapper::nrVADSamples>> chunk = vad->getNextChunk();
			
			// ... provide data here
			
			audioLogger->addChunk(std::move(chunk));
			
			availableChunks--;
			
			// std::cout << "Push chunks to recognizer, remaining = " << availableChunks << std::endl;
		}
		
		if (uttStatus == VADWrapperState::COMPLETE)
		{
			// .. flush data here
		}

		noMoreData = vad->analyze();
	}
	
	// if we are in idle state (again), a possible partial result is now final
	if (vad->getUtteranceStatus() == VADWrapperState::IDLE)
	{
		promoteToFinalResult();
	}
	
	if (finalResults.size() > 0)
	{
		// at least one final utterance can be read
		return 1;
	}
	
	// no final utterance available (maybe partial)
	return 0;
}

//////////////////////////////////////////////
const char* VoskRecognizer::getPartialResult(void)
{
	std::string res = "{ \"partial\" : \"";
	
	if (partialResult.size() > 0)
	{
		for (unsigned int i = 0; i < partialResult.size(); i++)
		{
			res += partialResult[i]->text;
			if (i < (partialResult.size() - 1))
			{
				res += " ";
			}
		}
	}
	
	res += "\" }";
	
	std::cout << "Partial result: " << res << std::endl;
	
	memset(partialResultBuffer, 0, sizeof(partialResultBuffer));
	strncpy(partialResultBuffer, res.c_str(), sizeof(partialResultBuffer) - 1);
	
	return partialResultBuffer;	
}

//////////////////////////////////////////////
const char* VoskRecognizer::getFinalResult(void)
{
	std::string res = "{ \"text\" : \"-- ";
	
	if (finalResults.size() > 0)
	{
		std::string currFinalResult = finalResults.front();
		audioLogger->flush(currFinalResult);
		res += currFinalResult;
		finalResults.erase(finalResults.begin());
	}
	
	res += " --\" }";
	
	std::cout << "Final result: " << res << std::endl;
	
	memset(finalResultBuffer, 0, sizeof(finalResultBuffer));
	strncpy(finalResultBuffer, res.c_str(), sizeof(finalResultBuffer) - 1);
	
	return finalResultBuffer;	
}

//////////////////////////////////////////////
void VoskRecognizer::promoteToFinalResult(void)
{
	std::string finalResult;
	
	if (partialResult.size() > 0)
	{
		for (unsigned int i = 0; i < partialResult.size(); i++)
		{
			finalResult += partialResult[i]->text;
			if (i < (partialResult.size() - 1))
			{
				finalResult += " ";
			}
		}
		
		std::cout << "Promoting partial result to final: " << finalResult << std::endl;
		
		finalResults.push_back(finalResult);
		
		partialResult.clear();
	}
}

//////////////////////////////////////////////
void VoskRecognizer::resultCallback(char* word, unsigned int startTimeMs, unsigned int endTimeMs, float negLogLikelihood)
{
	std::unique_ptr<RecognitionResult> newResult = std::make_unique<RecognitionResult>(word, startTimeMs, endTimeMs, negLogLikelihood);
	
	if (partialResult.size() == 0)
	{
		partialResult.push_back(std::move(newResult));	
	}
	else
	{
		// timestamps hopefully show if a new utterance starts (e.g. last one has been flushed)
		if (newResult->start < partialResult.back()->end)
		{
			promoteToFinalResult();
		}

		partialResult.push_back(std::move(newResult));		
	}
}
