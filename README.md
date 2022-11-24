# ESP32-AudioInI2S
A simple MEMS I2S microphone and audio processing library for ESP32.

## Features
* Simple I2S sample reading and setup. Just choose the pins, sample size and sample rate.
* Robust audio processing library for analysis.
  * Simple FFT compute on your I2S samples.
  * Frequency bands in 2, 4, 8, 16, 32 or 64 buckets.
  * Volume Unit Meter.
  * Set a noise floor to ignore values below it.
  * Normalize values into desired min/max ranges.
  * Auto level values for noisy/quiet enviroments where you want to keep values around the normalize max.
  * Ability to set the peak falloff rates and types. NO_FALLOFF, LINEAR_FALLOFF, ACCELERATE_FALLOFF, EXPONENTIAL_FALLOFF.
  * Equalizer to adjust the frequency levels for each bucket. Good for lowering the bass or trebble response depending on the enviroment.
* Easy to follow examples
  * `Basic` - Reads I2S microphone data to be viewed in the Serial Plotter.
  * `Frequencies` - Reads I2S microphone data, processes them into frequency buckets to be viewed in the Serial Plotter.
  * `FastLED` - Reads I2S microphone data, processes them into frequency buckets and displays them on a WS2812B led strip.
  * `OLED` - Reads I2S microphone data, processes them into frequency buckets and displays them on a 128x64 OLED display.
  * `WIFI` - *COMING SOON* - Reads I2S microphone data, processes them into frequency buckets and sends it wirelessly to a webpage.


## Hardware 
* ESP32, ESP32 S2, ESP32 C2, ESP32 C3
* INMP441 - MEMS Microphone

## AudioInI2S - Functions
* `#include <AudioInI2S.h>`
* **AudioInI2S(int bck_pin, int ws_pin, int data_pin, int channel_pin, i2s_channel_fmt_t channel_format)** // pin setup 
* **void begin(int sample_size, int sample_rate = 44100, i2s_port_t i2s_port_number = I2S_NUM_0)** - Starts the I2S DMA port.
* **void read(int32_t _samples[])** - Stores the current I2S port buffer into samples.

## AudioAnalysis - Functions
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
* **isClipping()** - is values exceding max
* **float \*getBands()** - gets the last bands calculated from processFrequencies()
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


## Example
Checkout the `examples/Frequencies`, `examples/FastLED` and `examples/OLED_128x64` examples folder for audio analysis.
```c++
/* Basic.ino */
#include <AudioInI2S.h>

#define SAMPLE_SIZE 1024 // Buffer size of read samples
#define SAMPLE_RATE 44100 // Audio Sample Rate

// ESP32 S2 Mini 
#define BCK_PIN 4 // Clock pin from the mic.
#define WS_PIN 39 // WS pin from the mic.
#define DATA_PIN 5 // Data pin from the mic.
#define CHANNEL_SELECT_PIN 40 // Pin to select the channel output from the mic.

AudioInI2S mic(BCK_PIN, WS_PIN, DATA_PIN, CHANNEL_SELECT_PIN); // defaults to RIGHT channel.

int32_t samples[SAMPLE_SIZE]; // I2S sample data is stored here

void setup() {
    Serial.begin(115200);
    mic.begin(SAMPLE_SIZE, SAMPLE_RATE); // Starts the I2S DMA port.
}

void loop() {
    mic.read(samples); // Stores the current I2S port buffer into samples.
    // Send data to serial plotter
    for(int i = 0; i < SAMPLE_SIZE; i++) {
        Serial.println(samples[i]);
    }
}
```

## Known Issues
The `AudioAnalysis.h` library is build on top of ArduinoFF2 V2 develop branch. You can find out more about it here: https://github.com/kosme/arduinoFFT/tree/develop

`AudioAnalsis.h` is not optimized and uses a lot of helper variables and floats. That said it is still very responsive at 1024 sample size and 44100 sample rate.

## Recognition
This library's audio processing would not be possible without ArduinoFFT https://github.com/kosme/arduinoFFT

## Licensing 
MIT Open Source - Free to use anywhere. 
