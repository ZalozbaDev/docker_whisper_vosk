#!/bin/bash

# VOSK_SAMPLE_RATE=48000 /vosk_whisper_server 0.0.0.0 2700 1 /whisper/whisper-small_hsb_23_08_07/ggml-model.bin
# VOSK_SAMPLE_RATE=48000 /vosk_whisper_server 0.0.0.0 2700 1 /whisper/whisper-small_hsb_23_08_07/ggml-model-q5_0.bin

# VOSK_SAMPLE_RATE=48000 /vosk_whisper_server 0.0.0.0 2700 1 /whisper/whisper-base_hsb_2023_08_15/ggml-model.q5_0.bin

nvidia-smi

# VOSK_SAMPLE_RATE=48000 /vosk_whisper_server 0.0.0.0 2700 1 /whisper/Korla/hsb_stt_demo/hsb_whisper/ggml-model.bin

# VOSK_SAMPLE_RATE=48000 /vosk_whisper_server 0.0.0.0 2700 1 /whisper/Korla/hsb_stt_demo/hsb_whisper/ggml-model.q4_0.bin

VOSK_SAMPLE_RATE=48000 /vosk_whisper_server 0.0.0.0 2700 1 /whisper/Korla/hsb_stt_demo/hsb_whisper/ggml-model.q8_0.bin

while /bin/true; do sleep 1 ; done

# export VOSK_SAMPLE_RATE=48000 
# gdb /vosk_whisper_server
# set args 0.0.0.0 2700 1 /whisper/Korla/hsb_stt_demo/hsb_whisper/ggml-model.q8_0.bin
# set print thread-events off
# run
