#!/bin/bash

# VOSK_SAMPLE_RATE=48000 /vosk_whisper_server 0.0.0.0 2700 1 /uasr-data/whisper-small_hsb_23_08_07/ggml-model.bin
# VOSK_SAMPLE_RATE=48000 /vosk_whisper_server 0.0.0.0 2700 1 /uasr-data/whisper-small_hsb_23_08_07/ggml-model-q5_0.bin

# VOSK_SAMPLE_RATE=48000 /vosk_whisper_server 0.0.0.0 2700 1 /uasr-data/whisper-base_hsb_2023_08_15/ggml-model.q5_0.bin

VOSK_SAMPLE_RATE=48000 /vosk_whisper_server 0.0.0.0 2700 1 /uasr-data/Korla/hsb_stt_demo/tree/main/hsb_whisper/ggml-model.bin
