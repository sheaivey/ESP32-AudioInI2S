/*
    TTGO-Display.ino
    By Shea Ivey

    Reads I2S microphone data into samples[], processes them into frequency buckets and then outputs it to an 135x240 TFT screen for viewing.
    Try wistling differrent tones to see which frequency buckets they fall into.

    Short Press Button 1 to change views
    1. Frequency Bars
    2. Oscilloscope

    Short Press Button 2 to change number of band buckets
    1. 2 bands
    2. 4 bands
    3. 8 bands
    4. 16 bands
    5. 32 bands
    6. 64 bands
*/

#include <AudioInI2S.h>

#define SAMPLE_SIZE 1024  // Buffer size of read samples 23.219ms @ 44100 sample rate
#define SAMPLE_RATE 44100 // Audio Sample Rate
#define FRAME_RATE 43     // frame every samples 23.219ms

/* Required defines for audio analysis */
#define BAND_SIZE 64 // powers of 2 up to 64, defaults to 8
#include <AudioAnalysis.h>
AudioAnalysis audioInfo;

// ESP32 TTGO Display
#define DISPLAY_TFT
#define MIC_BCK_PIN 32            // Clock pin from the mic.
#define MIC_WS_PIN 25             // WS pin from the mic.
#define MIC_DATA_PIN 33           // SD pin data from the mic.
#define MIC_CHANNEL_SELECT_PIN 27 // Left/Right pin to select the channel output from the mic.

#define BUTTON_PIN1 35             // change view button
#define BUTTON_PIN2 0              // clear display

AudioInI2S mic(MIC_BCK_PIN, MIC_WS_PIN, MIC_DATA_PIN, MIC_CHANNEL_SELECT_PIN); // defaults to RIGHT channel.

int32_t samples[SAMPLE_SIZE]; // I2S sample data is stored here

#define SCREEN_WIDTH 240  // TFT display width, in pixels
#define SCREEN_HEIGHT 135 // TFT display height, in pixels
#include <TFT_eSPI.h>
#include <Adafruit_GFX.h>
#include "FastLED.h"
GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT); // 240x135 pixel canvas
#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN 0x10
#endif

#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_CS 5
#define TFT_DC 16
#define TFT_RST 23

#define TFT_BL 4                                      // Display backlight control pin
TFT_eSPI tft = TFT_eSPI(SCREEN_HEIGHT, SCREEN_WIDTH); // Invoke custom library

unsigned long nextFrame = 0;

void setup()
{
  mic.begin(SAMPLE_SIZE, SAMPLE_RATE); // Starts the I2S DMA port.
  pinMode(BUTTON_PIN1, INPUT);
  pinMode(BUTTON_PIN2, INPUT);

  // audio analysis setup
  audioInfo.setNoiseFloor(10);                     // sets the noise floor
  audioInfo.normalize(true, 0, SCREEN_HEIGHT - 1); // normalize all values to range provided.

  audioInfo.autoLevel(AudioAnalysis::EXPONENTIAL_FALLOFF, .0001, 20, -1); // set auto level falloff rate
  audioInfo.bandPeakFalloff(AudioAnalysis::EXPONENTIAL_FALLOFF, .1);   // set the band peak fall off rate
  audioInfo.vuPeakFalloff(AudioAnalysis::EXPONENTIAL_FALLOFF, .01);       // set the volume unit peak fall off rate

  audioInfo.setEqualizerLevels(1, 1, 1); // set the equlizer offsets

  // TFT setup
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(0, 0);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);
  tft.setSwapBytes(true);
}
float frame = 0;
int bandSize = BAND_SIZE;
void loop()
{
  static int mode = 0;
  if (nextFrame > millis())
  {
    return;
  }
  frame++;
  // enforce a predictable frame rate
  nextFrame = millis() + (1000 / FRAME_RATE);

  processSamples(); // does all the reading and frequency calculations

  if(!digitalRead(BUTTON_PIN1)) {
    while (!digitalRead(BUTTON_PIN1));
    // change visual mode
    mode++;
    if(mode > 1) {
      mode = 0;
    }
  }

  if(!digitalRead(BUTTON_PIN2)) {
    while (!digitalRead(BUTTON_PIN2));
    // change number of frequency bands
    bandSize *= 2;
    if (bandSize > 64)
    {
      bandSize = 2;
    }
    audioInfo.setBandSize(bandSize);
  }

  /* RENDER MODES */
  switch(mode) {
    case 1:
    renderOscilloscope();
    break;
    case 0:
    default:
      renderFrequencies(); // render raw samples
      break;
  }
}

void processSamples()
{
  mic.read(samples); // Stores the current I2S port buffer into samples.
  audioInfo.computeFFT(samples, SAMPLE_SIZE, SAMPLE_RATE);
  audioInfo.computeFrequencies(bandSize);
}

void renderFrequencies()
{
  audioInfo.normalize(true, 0, SCREEN_HEIGHT - 1); // normalize all values to range provided.
  float *bands = audioInfo.getBands();
  float *peaks = audioInfo.getPeaks();
  float *bandEq = audioInfo.getEqualizerLevels();
  float vuMeter = audioInfo.getVolumeUnit();
  float vuMeterPeak = audioInfo.getVolumeUnitPeak();

  float bass = audioInfo.getBass();
  float mid = audioInfo.getMid();
  float treble = audioInfo.getTreble();

  float bassPeak = audioInfo.getBassPeak();
  float midPeak = audioInfo.getMidPeak();
  float treblePeak = audioInfo.getTreblePeak();

  canvas.fillScreen(0x0000);
  // equilizer first BAND_SIZE
  int offset = 0;
  int BAND_WIDTH = (SCREEN_WIDTH - 36) / audioInfo.getBandSize();
  int HALF_SCREEN = (float)SCREEN_HEIGHT / 2.0;
  for (int i = 0; i < audioInfo.getBandSize(); i++)
  {
    // band frequency
    canvas.fillRect(offset, (SCREEN_HEIGHT - 1) - bands[i], BAND_WIDTH - 1, SCREEN_HEIGHT + 2, 0xFFFFFF);
    canvas.drawLine(offset, (SCREEN_HEIGHT - 1) - peaks[i], offset + BAND_WIDTH - 2, (SCREEN_HEIGHT - 1) - peaks[i], 0xFFFFFF);
    offset += BAND_WIDTH;
  }
  offset = 0;
  for (int i = 0; i < audioInfo.getBandSize(); i++)
  {
    // equlizer curve
    if (i != audioInfo.getBandSize() - 1)
    {
      canvas.drawLine(
          offset + (BAND_WIDTH / 2),                           // x1
          (SCREEN_HEIGHT - 1) - (HALF_SCREEN * bandEq[i]),     // y1
          offset + BAND_WIDTH - 1 + (BAND_WIDTH / 2),          // x2
          (SCREEN_HEIGHT - 1) - (HALF_SCREEN * bandEq[i + 1]), // y1
          tft.color565(127, 127, 127)                          // color
      );
    }

    offset += BAND_WIDTH;
  }

  offset = SCREEN_WIDTH-34;
  canvas.fillRect(offset, (SCREEN_HEIGHT - 1) - bass, 3, SCREEN_HEIGHT + 2, tft.color565(255, 0, 0));
  canvas.drawLine(offset, (SCREEN_HEIGHT - 1) - bassPeak, offset + 3, (SCREEN_HEIGHT - 1) - bassPeak, 0xFFFFFF);

  offset += 4;
  canvas.fillRect(offset, (SCREEN_HEIGHT - 1) - mid, 3, SCREEN_HEIGHT + 2, tft.color565(255, 255, 0));
  canvas.drawLine(offset, (SCREEN_HEIGHT - 1) - midPeak, offset + 3, (SCREEN_HEIGHT - 1) - midPeak, 0xFFFFFF);

  offset += 4;
  canvas.fillRect(offset, (SCREEN_HEIGHT - 1) - treble, 3, SCREEN_HEIGHT + 2, tft.color565(0, 255, 0));
  canvas.drawLine(offset, (SCREEN_HEIGHT - 1) - treblePeak, offset + 3, (SCREEN_HEIGHT - 1) - treblePeak, 0xFFFFFF);

  // draw VU meter on right
  offset += 10;
  canvas.fillRect(offset, (SCREEN_HEIGHT - 1) - vuMeter, 12, SCREEN_HEIGHT + 2, tft.color565(0, 255, 0));
  canvas.drawLine(offset, (SCREEN_HEIGHT - 1) - vuMeterPeak, offset + 12, (SCREEN_HEIGHT - 1) - vuMeterPeak, 0xFFFFFF);

  tft.pushImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, canvas.getBuffer());
}

void renderOscilloscope()
{
  audioInfo.normalize(true, 0, SCREEN_HEIGHT); // normalize all values to range provided.
  int triggerIndex = audioInfo.getSampleTriggerIndex();
  canvas.fillScreen(0x0000);
  int stepSize = (SAMPLE_SIZE / 1.5) / SCREEN_WIDTH;
  for (int i = triggerIndex + stepSize, x0 = 1; x0 < SCREEN_WIDTH && i < SAMPLE_SIZE; i += stepSize, x0++)
  {
    int x1 = x0-1;
    int y0 = audioInfo.getSample(i); 
    int y1 = audioInfo.getSample(i - stepSize);
    canvas.drawLine(x0, y0, x1, y1, tft.color565(255, 255, 255));
  }
  tft.pushImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, canvas.getBuffer());
}
