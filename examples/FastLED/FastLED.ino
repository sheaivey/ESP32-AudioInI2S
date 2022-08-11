/*
    Frequencies.ino
    By Shea Ivey

    Reads I2S microphone data into samples[], processes them into frequency buckets and then outputs it to an LED strip for viewing.
    Try wistling differrent tones to see which frequency buckets they fall into.
*/

#include <AudioInI2S.h>

#define SAMPLE_SIZE 1024  // Buffer size of read samples
#define SAMPLE_RATE 44100 // Audio Sample Rate

/* Required defines for audio analysis */
#define BAND_SIZE 8        // powers of 2 up to 32, defaults to 8
#define NOISE_THRESHOLD 10 // threshold to be calculated into the bands
#include <AudioAnalysis.h>
AudioAnalysis audioInfo;

// ESP32 S2 Mini
#define MIC_BCK_PIN 4             // Clock pin from the mic.
#define MIC_WS_PIN 39             // WS pin from the mic.
#define MIC_DATA_PIN 5            // Data pin from the mic.
#define MIC_CHANNEL_SELECT_PIN 40 // Pin to select the channel output from the mic.

AudioInI2S mic(MIC_BCK_PIN, MIC_WS_PIN, MIC_DATA_PIN, MIC_CHANNEL_SELECT_PIN); // defaults to RIGHT channel.

int32_t samples[SAMPLE_SIZE]; // I2S sample data is stored here

#include "FastLED.h"
#define NUM_LEDS 70
#define LED_PIN 13
CRGB leds[NUM_LEDS];

void setup()
{
  mic.begin(SAMPLE_SIZE, SAMPLE_RATE); // Starts the I2S DMA port.

  // audio analysis setup
  audioInfo.setNoiseFloor(10);       // sets the noise floor
  audioInfo.normalize(true, 0, 255); // normalize all values to range provided.

  audioInfo.autoLevel(AudioAnalysis::ACCELERATE_FALLOFF, 1, 100, 10000); // set auto level falloff rate
  audioInfo.bandPeakFalloff(AudioAnalysis::EXPONENTIAL_FALLOFF, 0.05);   // set the band peak fall off rate
  audioInfo.vuPeakFalloff(AudioAnalysis::ACCELERATE_FALLOFF, 0.5);       // set the volume unit peak fall off rate

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(80);
  FastLED.show();
}

void loop()
{
  mic.read(samples); // Stores the current I2S port buffer into samples.
  audioInfo.computeFFT(samples, SAMPLE_SIZE, SAMPLE_RATE);
  audioInfo.computeFrequencies(BAND_SIZE);

  float *bands = audioInfo.getBands();
  float *peaks = audioInfo.getPeaks();
  int vuMeter = audioInfo.getVolumeUnit();
  int vuMeterPeak = audioInfo.getVolumeUnitPeak();
  int vuMeterPeakMax = audioInfo.getVolumeUnitPeakMax();

  // equilizer first BAND_SIZE
  for (int i = 0; i < BAND_SIZE; i++)
  {
    leds[i] = CHSV(i * (200 / BAND_SIZE), 255, (int)peaks[i]);
  }
  // volume unit meter rest of leds
  uint8_t vuLed = (uint8_t)map(vuMeter, 0, vuMeterPeakMax, BAND_SIZE + 1, NUM_LEDS - 1);
  uint8_t vuLedPeak = (uint8_t)map(vuMeterPeak, 0, vuMeterPeakMax, BAND_SIZE + 1, NUM_LEDS - 1);
  for (int i = BAND_SIZE + 1; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB(0, 0, 0);
    if (i < vuLed)
    {
      leds[i] = i > (NUM_LEDS - ((NUM_LEDS - BAND_SIZE) * 0.2)) ? CRGB(50, 0, 0) : CRGB(0, 50, 0);
    }
    if (i == vuLedPeak)
    {
      leds[i] = CRGB(50, 50, 50);
    }
  }

  FastLED.show();
}