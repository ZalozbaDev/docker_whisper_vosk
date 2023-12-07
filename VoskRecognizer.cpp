
#include <VoskRecognizer.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>

#include <cassert>

#include "common.h"

int VoskRecognizer::voskRecognizerInstanceId = 1;

//////////////////////////////////////////////
VoskRecognizer::VoskRecognizer(int modelId, float sample_rate, const char *configPath)
{
	std::cout << "vosk_recognizer_new, instance=" << voskRecognizerInstanceId << " sample_rate=" << sample_rate << std::endl;

	m_modelInstanceId = modelId;
	m_instanceId      = voskRecognizerInstanceId++;
	m_inputSampleRate = sample_rate;
	
	m_recoState = VoskRecognizerState::UNINIT;
	
	m_configPath = std::string(configPath);
	
	for (int i = 0; i < 10; i++)
	{
		std::string helloworld = m_configPath;
		for (int k = i; k < 10; k++)
		{
			helloworld = "*" + helloworld; 	
		}
		finalResults.push_back(helloworld);
	}

	// init static parts already here
	
	vad = new VADWrapper(3, m_processingSampleRate);
	
	audioLogger = new AudioLogger(std::string("/logs/"), m_instanceId);
}

//////////////////////////////////////////////
VoskRecognizer::~VoskRecognizer(void)
{
	std::cout << "vosk_recognizer_free, instance=" << m_instanceId << std::endl;
	
	delete(audioLogger);
	
	// if not yet initalized, do not try to free either
	if (m_recoState != VoskRecognizerState::UNINIT)
	{
		whisper_free(ctx);
	}
	
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
	
	// if not yet initalized, do that here and discard this audio
	if (m_recoState == VoskRecognizerState::UNINIT)
	{
		// whisper init
		cparams.use_gpu = true;
		
		ctx = whisper_init_from_file_with_params(m_configPath.c_str(), cparams);

		pcmf32.clear();
		
		WebRtcSpl_ResetResample48khzTo16khz(&m_resamplestate_48_to_16);

		m_recoState = VoskRecognizerState::INIT;
		
		return 0;
	}
	
	if ((m_inputSampleRate != 48000) || (m_processingSampleRate != 16000))
	{
		// only 48kHz-->16kHz is supported (both VAD and recognizer)
		// e.g. Jitsi provides 48 kHz so we need to downsample 1:3
		std::cout << "Unsupported sampling rates input " << m_inputSampleRate << " Hz and processing " << m_processingSampleRate << "Hz." << std::endl;
		assert(false);	
	}
	
	// splitting audio into chunks & resampling to 16kHz
	const int framelen48=480;
	const int framelen16=160;
	int32_t tmp[framelen48 + 256] = { 0 };
	int16_t buf[framelen16];
	
	while(leftOverDataLen + length >= framelen48 * 2){

		int useLen = framelen48 * 2 - leftOverDataLen;
		memcpy(leftOverData + leftOverDataLen, data, useLen);
		data += useLen;
		length -= useLen;
		leftOverDataLen = 0;

		WebRtcSpl_Resample48khzTo16khz((const int16_t*)leftOverData,buf,&m_resamplestate_48_to_16,tmp);
  
		// TODO we could remove all leftover handling from VAD
		status = vad->process(m_processingSampleRate, buf, framelen16);
	
		if (status == -1)
		{
			std::cout << "VAD processing error!" << std::endl;	
		}
	}

	if (length > 0)
	{
		leftOverDataLen=length;
		memcpy(leftOverData,data,length);
	}
	
	noMoreData = vad->analyze();
	
	while (noMoreData == false)
	{
		unsigned int availableChunks = vad->getAvailableChunks();
		
		while (availableChunks > 0)
		{
			std::unique_ptr<VADFrame<VADWrapper::nrVADSamples>> chunk = vad->getNextChunk();
			
			pcmf32.insert(pcmf32.cend(), std::begin(chunk->fSamples), std::end(chunk->fSamples));
			
			audioLogger->addChunk(std::move(chunk));
			
			availableChunks--;
		}
		
		noMoreData = vad->analyze();
	}
	
	// if we are in idle state (again), a possible partial result is now final
	if (pcmf32.size() > 0)
	{
		if (vad->getUtteranceStatus() == VADWrapperState::IDLE)
		{
			runWhisper();
			promoteToFinalResult();
			pcmf32.clear();
		}
		else
		{
			std::unique_ptr<RecognitionResult> newResult = std::make_unique<RecognitionResult>(const_cast<char*>("."), (unsigned int) 0, (unsigned int) 1, 1.0f);
			// partialResult.push_back(std::move(newResult));
		}
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
void VoskRecognizer::runWhisper(void)
{
	// run whisper on the current state of audio buffer
	whisper_params params;
	whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

	wparams.print_progress   = false;
	wparams.print_special    = params.print_special;
	wparams.print_realtime   = false;
	wparams.print_timestamps = !params.no_timestamps;
	wparams.translate        = params.translate;
	wparams.single_segment   = false; // !use_vad;
	wparams.max_tokens       = params.max_tokens;
	wparams.language         = params.language.c_str();
	wparams.n_threads        = params.n_threads;

	wparams.audio_ctx        = params.audio_ctx;
	wparams.speed_up         = params.speed_up;

	wparams.tdrz_enable      = params.tinydiarize; // [TDRZ]

	// disable temperature fallback
	//wparams.temperature_inc  = -1.0f;
	wparams.temperature_inc  = params.no_fallback ? 0.0f : wparams.temperature_inc;

	wparams.prompt_tokens    = nullptr; // params.no_context ? nullptr : prompt_tokens.data();
	wparams.prompt_n_tokens  = 0;       // params.no_context ? 0       : prompt_tokens.size();

	std::cout << "Push audio to whisper, size=" << pcmf32.size() << std::endl;
	if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
		fprintf(stderr, "whisper_full(): failed to process audio\n");
		assert(false);
	}

	partialResult.clear();
	
	const int n_segments = whisper_full_n_segments(ctx);
	for (int i = 0; i < n_segments; ++i) {
		const char * text = whisper_full_get_segment_text(ctx, i);

		const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
		const int64_t t1 = whisper_full_get_segment_t1(ctx, i);

		std::unique_ptr<RecognitionResult> newResult = std::make_unique<RecognitionResult>(const_cast<char*>(text), (unsigned int) t0, (unsigned int) t1, 1.0f);
		partialResult.push_back(std::move(newResult));
	}
}
