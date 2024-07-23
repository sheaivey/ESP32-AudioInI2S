# AudioAnalysis Class
This class provides simple functions to access the human audible spectrum via bucketing frequencies into bands
that can be easily visualized.

## Features
* Simple I2S sample reading and setup. Just choose the pins, sample size and sample rate.
* Robust audio processing library for analysis.
  * Simple FFT compute on your I2S samples.
  * Frequency bands in 2, 4, 8, 16, 32 or 64 buckets.
  * Volume Unit Meter.
  * Set a noise floor to ignore values below it.
  * Normalize values into desired min/max ranges.
  * Auto level values for noisy/quiet environments where you want to keep values around the normalize max.
  * Ability to set the peak falloff rates and types. NO_FALLOFF, LINEAR_FALLOFF, ACCELERATE_FALLOFF, EXPONENTIAL_FALLOFF.
  * Equalizer to adjust the frequency levels for each bucket. Good for lowering the bass or treble response depending on the environment.
* Easy to follow examples
  * `Frequencies` - Reads I2S microphone data, processes them into frequency buckets to be viewed in the Serial Plotter.
  * `FastLED` - Reads I2S microphone data, processes them into frequency buckets and displays them on a WS2812B led strip.
  * `OLED` - Reads I2S microphone data, processes them into frequency buckets and displays them on a 128x64 OLED display.
  * `TFT` - Reads I2S microphone data, processes them into frequency buckets and displays them on a 240x135 16bit TFT display.


## Hardware 
* ESP32, ESP32 S2, ESP32 C2, ESP32 C3
* INMP441 - MEMS Microphone

## AudioAnalysis - Class Functions
* `#include <AudioAnalysis.h>`
* **AudioAnalysis()**

**FFT Functions**
* **void computeFFT(int32_t samples[], int sample_size, int sample_rate)** - calculates FFT on sample data
* **float \*getReal()** - gets the Real values after FFT calculation
* **float \*getImaginary()** - gets the imaginary values after FFT calculation

**Band Frequency Functions**
* **void setNoiseFloor(float noiseFloor)** - threshold before sounds are registered
* **void computeFrequencies(uint8_t band_size = BAND_SIZE)** - converts FFT data into frequency bands
* **void normalize(bool normalize = true, float min = 0, float max = 1)** - normalize all values and constrain to min/max.
* **void autoLevel(falloff_type falloffType = ACCELERATE_FALLOFF, float falloffRate = 0.01, float min = 255, float max = -1)** - auto ballance normalized values to ambient noise levels. min and max are based on pre-normalized values.
* **void setEqualizerLevels(float low = 1, float mid = 1, float high = 1 )** adjust the frequency levels for a given range - low, medium and high. 0.5 = 50%, 1.0 = 100%, 1.5 = 150%  the raw value etc.
* **void setEqualizerLevels(float *bandEq)** - full control over each bands eq value. Array of float percentage values 1.0 = 100% [BAND_SIZE ...]
* **float *getEqualizerLevels(); // gets the last bandEq levels
* **void bandPeakFalloff(falloff_type falloffType = ACCELERATE_FALLOFF, float falloffRate = 0.05)** - set the falloff type and rate for band peaks.
* **void vuPeakFalloff(falloff_type falloffType = ACCELERATE_FALLOFF, float falloffRate = 0.05)** - set the falloff type and rate for volume unit peak.
* **isNormalize()** - is normalize enabled
* **isAutoLevel()** - is auto level enabled
* **isClipping()** - is values exceeding max
* **float \*getBands()** - gets the last bands calculated from processFrequencies()
* **uint16_t \*getBandNames()** - gets the band names in Hz calculated from calculateFrequencyOffsets()
* **uint16_t getBandName(uint8_t index)** - gets the band name in Hz at index calculated from calculateFrequencyOffsets()
* **float \*getPeaks()** - gets the last peaks calculated from processFrequencies()
* **float getBand(uint8_t index)** - gets the value at bands index
* **float getBandAvg()** - average value across all bands
* **float getBandMax()** - max value across all bands
* **int getBandMaxIndex()** - index of the highest value band
* **int getBandMinIndex()** - index of the lowest value band
* **float getPeak(uint8_t index)** - gets the value at peaks index
* **float getPeakAvg()** - average value across all peaks
* **float getPeakMax()** - max value across all peaks
* **int getPeakMaxIndex()** - index of the highest value peak
* **int getPeakMinIndex()** - index of the lowest value peak

**Volume Unit Functions**
* **float getVolumeUnit()** - gets the last volume unit calculated from processFrequencies()
* **float getVolumeUnitPeak()** - gets the last volume unit peak calculated from processFrequencies()
* **float getVolumeUnitMax()** - value of the highest value volume unit
* **float getVolumeUnitPeakMax()** - value of the highest value volume unit

## Known Issues
The `AudioAnalysis.h` library is build on top of ArduinoFF2 V2 develop branch. You can find out more about it here: https://github.com/kosme/arduinoFFT/tree/develop

`AudioAnalysis.h` is not optimized and uses a lot of helper variables and floats. That said it is still very responsive at 1024 sample size and 44100 sample rate.

## Recognition
This library's audio processing would not be possible without ArduinoFFT https://github.com/kosme/arduinoFFT

## Licensing 
MIT Open Source - Free to use anywhere. 
