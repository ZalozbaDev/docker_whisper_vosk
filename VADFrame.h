
#include <cstddef>

enum VADState {OFF, ACTIVE};

template<std::size_t numberSamples>
class VADFrame
{
public:
	short    samples[numberSamples];
	float    fSamples[numberSamples];
	VADState state;
};
