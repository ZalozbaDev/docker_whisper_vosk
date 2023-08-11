#ifndef AUDIO_LOGGER_H
#define AUDIO_LOGGER_H

#include <deque>
#include <memory>
#include <string>

#include <VADWrapper.h>


//////////////////////////////////////////////
class AudioLogger
{
public:
	AudioLogger(std::string logPath, int instanceId);
	~AudioLogger(void);
	void addChunk(std::unique_ptr<VADFrame<VADWrapper::nrVADSamples>> chunk);
	void flush(std::string resultText);
private:
	int m_instanceId;
	std::string m_logPath;
	std::deque<std::unique_ptr<VADFrame<VADWrapper::nrVADSamples>>> chunks;
	std::string filename;
	unsigned long long idx;
};

#endif // AUDIO_LOGGER_H
