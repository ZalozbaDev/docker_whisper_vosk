#!/bin/bash

VOSK_SAMPLE_RATE=48000 /vosk_whisper_server 0.0.0.0 2700 1 /uasr-data/ggml-model.bin
