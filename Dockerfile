FROM ubuntu:jammy
MAINTAINER Daniel Sobe <daniel.sobe@sorben.com>

# normal call
# docker build --progress=plain -t vosk_server_whisper .

# rebuild from scratch
# docker build --progress=plain -t vosk_server_whisper . --no-cache

# RUN sed -i 's/ main/ main contrib non-free/' /etc/apt/sources.list

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

RUN apt install -y nvidia-cuda-toolkit nvidia-utils-535-server

# get boost source code for some boost header files 
RUN wget -q https://boostorg.jfrog.io/artifactory/main/release/1.76.0/source/boost_1_76_0.tar.gz \
  && tar xzf boost_1_76_0.tar.gz

# get VOSK API (for header file)
RUN git clone https://github.com/ZalozbaDev/vosk-api.git vosk-api
RUN cd vosk-api && git checkout 1053cfa0f80039d2956de7e05a05c0b8db90c3c0

# get VOSK server implementation
RUN git clone https://github.com/ZalozbaDev/vosk-server.git vosk-server
RUN cd vosk-server && git checkout 21147c33e383f45b942846c9f713789d8bca41d1

RUN cp vosk-api/src/vosk_api.h /
RUN cp vosk-server/websocket-cpp/asr_server.cpp /

# dummy to trigger rebuild
RUN touch b

# get whisper.cpp files
RUN git clone https://github.com/ZalozbaDev/whisper.cpp.git whisper.cpp
RUN cd whisper.cpp && git checkout v1.4.3
# last known-good version is here:
# RUN cd whisper.cpp && git checkout ec7a6f0

# prepare whisper dependencies
RUN cd whisper.cpp/ && WHISPER_CUBLAS=1 make ggml.o 
RUN cd whisper.cpp/ && WHISPER_CUBLAS=1 make whisper.o 
RUN cd whisper.cpp/ && WHISPER_CUBLAS=1 make ggml-cuda.o
RUN cd whisper.cpp/ && WHISPER_CUBLAS=1 make ggml-alloc.o
RUN cd whisper.cpp/ && WHISPER_CUBLAS=1 make ggml-backend.o
RUN cd whisper.cpp/ && WHISPER_CUBLAS=1 make ggml-quants.o

COPY VoskRecognizer.cpp VoskRecognizer.h VADFrame.h VADWrapper.cpp VADWrapper.h RecognitionResult.h \
AudioLogger.h AudioLogger.cpp vosk_api_wrapper.cpp /

RUN g++ -Wall -Wno-write-strings -std=c++17 -O3 -fPIC -DGGML_USE_CUBLAS -o vosk_whisper_server \
-I/boost_1_76_0/ -I. -I/whisper.cpp/ -I/whisper.cpp/examples/ -I/webrtc-audio-processing/webrtc/ \
asr_server.cpp VoskRecognizer.cpp VADWrapper.cpp vosk_api_wrapper.cpp AudioLogger.cpp \
whisper.cpp/examples/common.cpp whisper.cpp/examples/common-ggml.cpp  whisper.cpp/ggml.o whisper.cpp/whisper.o \
whisper.cpp/ggml-cuda.o whisper.cpp/ggml-alloc.o whisper.cpp/ggml-backend.o whisper.cpp/ggml-quants.o \
webrtc-audio-processing/build/webrtc/common_audio/libcommon_audio.a \
-lpthread -lcublas -lculibos -lcudart -lcublasLt -L/usr/local/cuda/lib64 -L/opt/cuda/lib64

RUN mkdir -p /logs/
RUN mkdir -p /whisper/

RUN apt install -y gdb

############################################
# Vosk server startup script
############################################

COPY startme.sh /
RUN chmod 755 startme.sh

CMD ["/startme.sh"]

# CMD ["./asr_server", "0.0.0.0" ,"2700", "5" ,"/opt/vosk-model-en/model"]
