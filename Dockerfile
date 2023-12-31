FROM debian:bullseye-slim
MAINTAINER Daniel Sobe <daniel.sobe@sorben.com>

# normal call
# docker build -t vosk_server_whisper .

# rebuild from scratch
# docker build -t vosk_server_whisper . --no-cache

RUN apt update

########################################################
# Setup locale to properly support sorbian diacritics
########################################################

RUN apt-get install -y locales

RUN sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen && \
    locale-gen

ENV LC_ALL en_US.UTF-8 
ENV LANG en_US.UTF-8  
ENV LANGUAGE en_US:en     

########################################################
# Install all dependencies for compilation
########################################################

RUN apt install -y g++ wget git make nano 

###################################
# Build WebRTC dependency
###################################

RUN git clone https://github.com/ZalozbaDev/webrtc-audio-processing.git webrtc-audio-processing
RUN cd webrtc-audio-processing && git checkout 6e37f37c4ea8790760b4c55d9ce9024a7e7bf260

RUN apt install -y meson libabsl-dev

RUN cd webrtc-audio-processing && meson . build -Dprefix=$PWD/install && ninja -C build

############################################
# Build vosk server wrapper and whisper glue code
############################################

# get boost source code for some boost header files 
RUN wget -q https://boostorg.jfrog.io/artifactory/main/release/1.76.0/source/boost_1_76_0.tar.gz \
  && tar xzf boost_1_76_0.tar.gz

# get VOSK API (for header file)
RUN git clone https://github.com/ZalozbaDev/vosk-api.git vosk-api
RUN cd vosk-api && git checkout 1053cfa0f80039d2956de7e05a05c0b8db90c3c0

# get VOSK server implementation
RUN git clone https://github.com/ZalozbaDev/vosk-server.git vosk-server
RUN cd vosk-server && git checkout 1faf58f4fbe1609bb2fce319dd7f64a20dd993b5

RUN cp vosk-api/src/vosk_api.h /
RUN cp vosk-server/websocket-cpp/asr_server.cpp /

# get whisper.cpp files
RUN git clone https://github.com/ZalozbaDev/whisper.cpp.git whisper.cpp
RUN cd whisper.cpp && git checkout a4bb2df36aeb4e6cfb0c1ca9fbcf749ef39cc852

# prepare whisper dependencies
RUN cd whisper.cpp/ && make ggml.o && make whisper.o

COPY VoskRecognizer.cpp VoskRecognizer.h VADFrame.h VADWrapper.cpp VADWrapper.h RecognitionResult.h \
AudioLogger.h AudioLogger.cpp vosk_api_wrapper.cpp /

RUN g++ -Wall -Wno-write-strings -std=c++17 -O3 -fPIC -o vosk_whisper_server -I/boost_1_76_0/ -I. -I/whisper.cpp/ -I/whisper.cpp/examples/ \
asr_server.cpp VoskRecognizer.cpp VADWrapper.cpp vosk_api_wrapper.cpp AudioLogger.cpp \
whisper.cpp/examples/common.cpp whisper.cpp/examples/common-ggml.cpp  whisper.cpp/ggml.o whisper.cpp/whisper.o  \
webrtc-audio-processing/build/webrtc/common_audio/libcommon_audio.a \
-lpthread

RUN mkdir -p /logs/
RUN mkdir -p /uasr-data/

############################################
# Vosk server startup script
############################################
    
COPY startme.sh /
RUN chmod 755 startme.sh

CMD ["/startme.sh"]

# CMD ["./asr_server", "0.0.0.0" ,"2700", "5" ,"/opt/vosk-model-en/model"]
