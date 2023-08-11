#ifndef RECOGNITION_RESULT_H
#define RECOGNITION_RESULT_H

#include <vector>
#include <chrono>

class RecognitionResult
{
public:
	std::string               text;
	std::chrono::milliseconds start;
	std::chrono::milliseconds end;
	float                     m_confidence;
	
	RecognitionResult(char* word, unsigned int startTimeMs, unsigned int endTimeMs, float confidence) 
	{
		text               = word;
		start              = std::chrono::milliseconds(startTimeMs);
		end                = std::chrono::milliseconds(endTimeMs);
		m_confidence       = confidence;
	}
};

#endif // RECOGNITION_RESULT_H
