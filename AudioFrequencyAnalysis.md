# AudioFrequencyAnalysis Class
This is the next version of `AudioAnalysis.h` but with a focus on user defined frequencies.
It uses a supporting class `FrequencyRange` to define frequency buckets. Each Frequency bucket 
maintains its own min/max, peak and fall off type. It also allows for compensation.
`AudioFrequencyAnalysis` calculates the FFT from samples read then loops over all the registered 
`FrequencyRanges` and updates their values.


## Features
* Simple I2S sample reading and setup. Just choose the pins, sample size and sample rate.
* Robust audio processing library for analysis.
  * Simple FFT compute on your I2S samples.
  * Set a noise floor to ignore values below it.
  * Normalize values into desired min/max ranges.
  * Auto level values for noisy/quiet environments where you want to keep values around the normalize max.
  * Ability to set the peak falloff rates and types. NO_FALLOFF, LINEAR_FALLOFF, ACCELERATE_FALLOFF, EXPONENTIAL_FALLOFF, ROLLING_AVERAGE_FALLOFF.
  * Equalizer to adjust the frequency levels for each bucket. Good for lowering the bass or treble response depending on the environment.
* Easy to follow examples
  * `FrequencyRange` - Reads I2S microphone data, processes them into frequency buckets to be viewed in the Serial Plotter.
  * `FrequencyRange-Visuals` - Reads I2S microphone data, processes them into frequency buckets and displays them on a 240x135 16bit TFT display.


## Hardware 
* ESP32, ESP32 S2, ESP32 C2, ESP32 C3
* INMP441 - MEMS Microphone

## FrequencyRange - Class Functions
* `#include <AudioFrequencyAnalysis.h>`
* **FrequencyRange()** full 0Hz - 20000Hz range
* **FrequencyRange(uint16_t lowHz, uint16_t highHz, float scaling = 1)** - scaling for equalizer
* **float getValue()** - returns the raw value
* **float getValue(float** min, float max) - returns the calculated value
* **float getPeak()** - returns the raw peak
* **float getPeak(float** min, float max) - returns the calculated peak
* **uint16_t getMaxFrequency()** - gets the max frequency in Hz within the range
* **float getMin()** - gets the lowest raw value in the range
* **float getMax()** - gets the highest raw value in the range

## AudioFrequencyAnalysis - Class Functions
* `#include <AudioFrequencyAnalysis.h>`
**AudioFrequencyAnalysis(int32_t *samples, int sampleSize, int sampleRate)**

**void loop(int32_t *samples, int sampleSize, int sampleRate)** - calculates FFT on sample data

**void addFrequencyRange(FrequencyRange *_frequencyRange)** - register a frequency range for processing

**float *getReal()** - gets the Real values after FFT calculation
**float *getImaginary()** - gets the imaginary values after FFT calculation  
**int getSampleRate()** - gets the current sample rate
**int getSampleSize()** - gets the current sample size

**void setNoiseFloor(float noiseFloor)** - raw threshold before sounds are registered
**void normalize(bool normalize = true, float min = 0, float max = 1)** - normalize all values and constrain to min/max.
**void autoLevel(falloff_type falloffType = EXPONENTIAL_FALLOFF, float falloffRate = 0.01, float min = 10, float max = -1)** - auto ballance normalized values to ambient noise levels.
**bool isNormalize()** - is normalize enabled
**bool isAutoLevel()** - is auto level enabled

**float getSample(uint16_t index)** - gets the raw sample value at index
**float getSample(uint16_t index, float min, float max)** - calculates the normalized sample value at index
**uint16_t getSampleTriggerIndex()** - finds the index of the first cross point at zero
**float getSampleMin()** - gets the lowest raw value in the samples
**float getSampleMax()** - gets the highest raw value in the samples


## Example
Checkout the `examples/FrequencyRange` and `examples/TTGO-T-Display/FrequencyRange-Visuals` examples folder for audio analysis.
```c++
/* FrequencyRange-Visuals.ino */
#include <AudioInI2S.h>

#define SAMPLE_SIZE 1024  // Buffer size of read samples
#define SAMPLE_RATE 44100 // Audio Sample Rate

#include <AudioFrequencyAnalysis.h>
AudioFrequencyAnalysis audioInfo;

// ESP32 S2 Mini
#define MIC_BCK_PIN 4             // Clock pin from the mic.
#define MIC_WS_PIN 39             // WS pin from the mic.
#define MIC_DATA_PIN 5            // SD pin data from the mic.
#define MIC_CHANNEL_SELECT_PIN 40 // Left/Right pin to select the channel output from the mic.

AudioInI2S mic(MIC_BCK_PIN, MIC_WS_PIN, MIC_DATA_PIN, MIC_CHANNEL_SELECT_PIN); // defaults to RIGHT channel.

int32_t samples[SAMPLE_SIZE]; // I2S sample data is stored here

FrequencyRange vuMeter(0, 20000);
FrequencyRange bass(0, 249);
FrequencyRange mid(250, 1499);
FrequencyRange high(1500, 16000);

void setup()
{
  Serial.begin(115200);
  mic.begin(SAMPLE_SIZE, SAMPLE_RATE); // Starts the I2S DMA port.

  // audio analysis setup
  audioInfo.setNoiseFloor(1);  // sets the noise floor

  vuMeter._inIsolation = true; // isolates the vu meters min/max from the rest of the frequency ranges

  // register the frequency ranges to audioInfo
  audioInfo.addFrequencyRange(&vuMeter);
  audioInfo.addFrequencyRange(&bass);
  audioInfo.addFrequencyRange(&mid);
  audioInfo.addFrequencyRange(&high);
}

void loop()
{
  mic.read(samples); // Stores the current I2S port buffer into samples.
  audioInfo.loop(samples, SAMPLE_SIZE, SAMPLE_RATE);

  // also send the vu meter data
  Serial.printf("maxFrequency: %4d, ", vuMeter.getMaxFrequency()); // returns the frequency with the highest amplitude
  Serial.printf("vuRaw: %8.2f, vuNorm: %6.2f, vuPeak: %6.2f, ", vuMeter.getValue(), vuMeter.getValue(0,255), vuMeter.getPeak(0,255)); 
  Serial.printf("bassRaw: %8.2f, bassNorm: %6.2f, bassPeak: %6.2f, ", bass.getValue(), bass.getValue(0,255), bass.getPeak(0,255));
  Serial.printf("midRaw: %8.2f, midNorm: %6.2f, midPeak: %6.2f, ", mid.getValue(), mid.getValue(0,255), mid.getPeak(0,255));
  Serial.printf("highRaw: %8.2f, highNorm: %6.2f, highPeak: %6.2f", high.getValue(), high.getValue(0,255), high.getPeak(0,255));

  Serial.println();
}
```

## Known Issues
The `AudioFrequencyAnalysis.h` library is build on top of ArduinoFF2 V2 develop branch. You can find out more about it here: https://github.com/kosme/arduinoFFT/tree/develop

`AudioAnalysis.h` is not optimized and uses a lot of helper variables and floats. That said it is still very responsive at 1024 sample size and 44100 sample rate.

## Recognition
This library's audio processing would not be possible without ArduinoFFT https://github.com/kosme/arduinoFFT

## Licensing 
MIT Open Source - Free to use anywhere. 
