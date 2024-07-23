# ESP32-AudioInI2S
A simple MEMS I2S microphone and audio processing library for ESP32.

## Features
* Simple I2S sample reading and setup. Just choose the pins, sample size and sample rate.
* Robust audio processing classes for analysis.
  * Simple FFT compute on your I2S samples.
  * Frequency bands in 2, 4, 8, 16, 32 or 64 buckets.
  * Volume Unit Meter.
  * Set a noise floor to ignore values below it.
  * Normalize values into desired min/max ranges.
  * Auto level values for noisy/quiet enviroments where you want to keep values around the normalize max.
  * Ability to set the peak falloff rates and types. NO_FALLOFF, LINEAR_FALLOFF, ACCELERATE_FALLOFF, EXPONENTIAL_FALLOFF.
  * Equalizer to adjust the frequency levels for each bucket. Good for lowering the bass or treble response depending on the enviroment.

#### [AudioInI2S.md](AudioInI2S Class)
  * [examples/Basic/Basic.ino](Basic) - Reads I2S microphone data to be viewed in the Serial Plotter.

#### [AudioAnalysis.md](AudioAnalysis Class)
  * [examples/Frequencies/Frequencies.ino](Frequencies) - Reads I2S microphone data, processes them into frequency buckets to be viewed in the Serial Plotter.
  * [examples/FastLED/FastLED.ino](FastLED) - Reads I2S microphone data, processes them into frequency buckets and displays them on a WS2812B led strip.
  * [examples/OLED_128x64/OLED_128x64.ino](OLED) - Reads I2S microphone data, processes them into frequency buckets and displays them on a 128x64 OLED display.
  * [examples/TTGO-T-Display/Basic-Visuals/Basic-Visuals.ino](TFT) - Reads I2S microphone data, processes them into frequency buckets and displays them on a 240x135 16bit TFT display.

#### [AudioFrequencyAnalysis.md](AudioFrequencyAnalysis Class)
  * [examples/FrequencyRangeFrequencyRange.ino](FrequencyRange) - Reads I2S microphone data, processes them into frequency buckets to be viewed in the Serial Plotter.
  * [examples/TTGO-T-Display/FrequencyRange-Visuals/FrequencyRange-Visuals.ino](FrequencyRange-Visuals) - Reads I2S microphone data, processes them into frequency buckets and displays them on a 240x135 16bit TFT display.

## Hardware 
* ESP32, ESP32 S2, ESP32 C2, ESP32 C3
* INMP441 - MEMS Microphone

## Known Issues
The `AudioAnalysis.h` and `AudioFrequencyAnalysis.h` classes are built on top of ArduinoFF2 V2 develop branch. You can find out more about it here: https://github.com/kosme/arduinoFFT/tree/develop

`AudioAnalysis.h` is not optimized and uses a lot of helper variables and floats. That said it is still very responsive at 1024 sample size and 44100 sample rate.

## Recognition
This library's audio processing would not be possible without ArduinoFFT https://github.com/kosme/arduinoFFT

## Licensing 
MIT Open Source - Free to use anywhere. 
