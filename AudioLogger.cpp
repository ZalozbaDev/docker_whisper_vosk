
#include <iostream>

#include <AudioLogger.h>

#include <filesystem>
#include <ios>
#include <fstream>
#include <string>

//////////////////////////////////////////////
AudioLogger::AudioLogger(std::string logPath, int instanceId)
{
	m_logPath = logPath;
	m_instanceId = instanceId;
	
	// although directory should already exist
	std::filesystem::create_directories(logPath);
	
	chunks.clear();
	filename.clear();
	idx = 0;
}

//////////////////////////////////////////////
AudioLogger::~AudioLogger(void)
{
	chunks.clear();
}

//////////////////////////////////////////////
void AudioLogger::addChunk(std::unique_ptr<VADFrame<VADWrapper::nrVADSamples>> chunk)
{
	if (filename.size() == 0)
	{
		std::ostringstream os;
		
		auto t = std::time(nullptr);
		auto tm = *std::localtime(&t);
		
		os << std::put_time(&tm, "%d%m%y_%H%M%S") << "_" << m_instanceId << "_" << (idx++);
		
		filename = os.str();
	}
	
	chunks.push_back(std::move(chunk));
}

//////////////////////////////////////////////
void AudioLogger::flush(std::string resultText)
{
	std::cout << "Logging " << chunks.size() << " chunks to file " << filename << " utterance " << resultText << std::endl;
	
	if (filename.size() > 0)
	{
		if (chunks.size() > 0)
		{
			std::string audioFilename = m_logPath + filename + ".raw";
			std::ofstream audioStream(audioFilename.c_str(), std::ofstream::out | std::ofstream::binary);
			
			if ((audioStream.rdstate() & (std::ofstream::failbit | std::ofstream::badbit)) != 0)
			{
				std::cout << "Error opening " << audioFilename << " for writing!" << std::endl;
			}
			else
			{
				bool isGood = true;
				
				while ((chunks.size() > 0) && (isGood == true))
				{
					std::unique_ptr<VADFrame<VADWrapper::nrVADSamples>> chunk = std::move(chunks.front());
					chunks.pop_front();
					audioStream.write((const char*) chunk->samples, sizeof(chunk->samples));
					
					isGood = audioStream.good();
				}
			
				if (isGood == false)
				{
					std::cout << "Error writing audio file " << audioFilename << std::endl;	
				}
				
				audioStream.close();
			}
			
			std::string textFilename = m_logPath + filename + ".txt";
			std::ofstream textStream(textFilename.c_str(), std::ofstream::out);
			
			if ((textStream.rdstate() & (std::ofstream::failbit | std::ofstream::badbit)) != 0)
			{
				std::cout << "Error opening " << textFilename << " for writing!" << std::endl;
			}
			else
			{
				textStream << resultText << std::endl;
			
				if (textStream.good() == false)
				{
					std::cout << "Error writing text file " << textFilename << std::endl;	
				}
				
				textStream.close();
			}
		}
	}
	
	chunks.clear();
	filename.clear();
}
