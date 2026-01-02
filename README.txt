PROJECT F.R.I.D.A.Y
------------
ESP32 AI Voice Assistant

DESCRIPTION
-----------
A portable AI-powered voice assistant built using ESP32.
The system records voice input, converts speech to text using Deepgram,
processes the query using Google Gemini, and responds with spoken audio output.

FEATURES
--------
- Offline audio recording (WAV on SD card)
- Speech-to-Text using Deepgram API
- AI response using Google Gemini
- Text-to-Speech audio output
- Repeat last response
- Battery voltage monitoring
- Modular and scalable architecture

TECH STACK
----------
Microcontroller : ESP32
Language        : C++ (Arduino Framework)
Audio Input     : INMP441 I2S Microphone
Audio Output    : MAX98357 I2S Amplifier
STT             : Deepgram API
LLM             : Google Gemini API
Storage         : MicroSD Card

AUTHOR
------
Developed and customized by TechiesMS
Based on original work by KALO Projects

