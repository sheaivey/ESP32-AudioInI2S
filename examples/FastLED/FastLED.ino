/*
    Frequencies.ino
    By Shea Ivey

    Reads I2S microphone data into samples[], processes them into frequency buckets and then outputs it to an LED strip for viewing.
    Try wistling differrent tones to see which frequency buckets they fall into.

    TIP: uncomment renderBeatRainbow() for a music beat visualization.
*/

#include <AudioInI2S.h>

#define SAMPLE_SIZE 1024  // Buffer size of read samples
#define SAMPLE_RATE 44100 // Audio Sample Rate

/* Required defines for audio analysis */
#define BAND_SIZE 8 // powers of 2 up to 64, defaults to 8
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
#define NUM_LEDS 72
#define LED_PIN 13
#define MAX_BRIGHTNESS 80 // save your eyes
#define FRAME_RATE 30
CRGB leds[NUM_LEDS];
unsigned long nextFrame = 0;
unsigned long tick = 0;

void setup()
{
  mic.begin(SAMPLE_SIZE, SAMPLE_RATE); // Starts the I2S DMA port.

  // audio analysis setup
  audioInfo.setNoiseFloor(10);       // sets the noise floor
  audioInfo.normalize(true, 0, 255); // normalize all values to range provided.

  audioInfo.autoLevel(AudioAnalysis::ACCELERATE_FALLOFF, 1, 255, 1000); // set auto level falloff rate
  audioInfo.bandPeakFalloff(AudioAnalysis::EXPONENTIAL_FALLOFF, 1);     // set the band peak fall off rate
  audioInfo.vuPeakFalloff(AudioAnalysis::ACCELERATE_FALLOFF, 1);        // set the volume unit peak fall off rate

  // FastLED setup
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(MAX_BRIGHTNESS);
  FastLED.show();
}

void loop()
{
  if (nextFrame > millis())
  {
    return;
  }
  // enforce a predictable frame rate
  nextFrame = millis() + (1000 / FRAME_RATE);
  tick++;

  processSamples(); // does all the reading and frequency calculations

  /* RENDER MODES */
  renderBasicTest(); // bands and volume unit visualization
  // renderBeatRainbow(); // uncommnet this line for a music beat visualization
}

void processSamples()
{
  mic.read(samples); // Stores the current I2S port buffer into samples.
  audioInfo.computeFFT(samples, SAMPLE_SIZE, SAMPLE_RATE);
  audioInfo.computeFrequencies(BAND_SIZE);
}

void renderBasicTest()
{
  float *bands = audioInfo.getBands();
  float *peaks = audioInfo.getPeaks();
  int vuMeter = audioInfo.getVolumeUnit();
  int vuMeterPeak = audioInfo.getVolumeUnitPeak();
  int vuMeterPeakMax = audioInfo.getVolumeUnitPeakMax();

  leds[BAND_SIZE] = CRGB(0, 0, 0);

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

void renderBeatRainbow()
{
  float *bands = audioInfo.getBands();
  float *peaks = audioInfo.getPeaks();
  int peakBandIndex = audioInfo.getBandMaxIndex();
  int peakBandValue = audioInfo.getBand(peakBandIndex);
  static int beatCount = 0;

  bool beatDetected = false;
  bool clapsDetected = false;
  // beat detection
  if (peaks[0] == bands[0] && peaks[0] > 0) // new peak for bass must be a beat
  {
    beatCount++;
    beatDetected = true;
  }
  if (peakBandIndex >= BAND_SIZE / 2 && peakBandValue > 0)
  {
    clapsDetected = true;
  }

  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = blend(leds[i], CRGB(0, 0, 0), 100); // fade to black over time

    // bass/beat = rainbow
    if (beatDetected)
    {
      if (random(0, 10 - ((float)peaks[1] / (float)255 * 10.0)) == 0)
      {
        leds[i] = CHSV((beatCount * 10) % 255, 255, 255);
      }
    }

    // claps/highs = white twinkles
    if (clapsDetected)
    {
      if (random(0, 40 - ((float)peakBandIndex / (float)BAND_SIZE * 10.0)) == 0)
      {
        leds[i] = CRGB(255, 255, 255);
      }
    }
  }

  FastLED.show();
}