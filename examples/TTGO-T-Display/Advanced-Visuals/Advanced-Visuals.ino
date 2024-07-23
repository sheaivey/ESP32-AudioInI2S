/*
    TTGO-Display.ino
    By Shea Ivey

    Reads I2S microphone data into samples[], processes them into frequency buckets and then outputs it to an 135x240 TFT screen for viewing.
    Try wistling differrent tones to see which frequency buckets they fall into.

    Short Press Button 1 to change views
    1. Frequency Bars
    2. Oscilloscope
    3. Oscilloscope Circle
    4. Frequency Radar
    5. Triangles
    6. Spectrograph
    7. Band Matrix
    8. Apple Bars

    Long Press Button 1 go back to previous view

    Short Press Button 2 to change number of band buckets
    1. 2 bands
    2. 4 bands
    3. 8 bands
    4. 16 bands
    5. 32 bands
    6. 64 bands

    Long Button 2 to change fade out effect
    1. No Effect
    2. fade out to black
    3. fade out rainbow
    4. fade up and down from center rainbow
    5. fade in to center rainbow
    6. fade out all directions from center rainbow
*/

#include <AudioInI2S.h>

#define SAMPLE_SIZE 1024 // Buffer size of read samples 23.219ms @ 44100 sample rate
#define SAMPLE_RATE 44100 // Audio Sample Rate
#define FRAME_RATE 120    // every frame 8ms

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
  Serial.begin(115200);
  mic.begin(SAMPLE_SIZE, SAMPLE_RATE); // Starts the I2S DMA port.
  pinMode(BUTTON_PIN1, INPUT);
  pinMode(BUTTON_PIN2, INPUT);

  // audio analysis setup
  audioInfo.setNoiseFloor(5);                     // sets the noise floor
  audioInfo.normalize(true, 0, SCREEN_HEIGHT - 1); // normalize all values to range provided.

  audioInfo.autoLevel(AudioAnalysis::EXPONENTIAL_FALLOFF, .001, 20, -1); // set auto level falloff rate
  audioInfo.bandPeakFalloff(AudioAnalysis::EXPONENTIAL_FALLOFF, 1);   // set the band peak fall off rate
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
int visualMode = 0;
int clearMode = 0;
void loop()
{
  if (nextFrame > millis())
  {
    return;
  }
  frame++;
  // enforce a predictable frame rate
  nextFrame = millis() + (1000 / FRAME_RATE);

  processSamples(); // does all the reading and frequency calculations

  if (!digitalRead(BUTTON_PIN1))
  {
    unsigned long t = millis();
    while (!digitalRead(BUTTON_PIN1));
    t = millis() - t;
    if(t < 250) { // short press
      visualMode++;
      if (visualMode > 10)
      {
        visualMode = 0;
      }
    }
    else { // long press
      visualMode--;
      if (visualMode < 0)
      {
        visualMode = 10;
      }
    }
  }

  if(!digitalRead(BUTTON_PIN2)) {
    unsigned long t = millis();
    while (!digitalRead(BUTTON_PIN2));
    t = millis() - t;
    if(t < 250) { // short press
      bandSize *= 2;
      if (bandSize > 64)
      {
        bandSize = 2;
      }
      audioInfo.setBandSize(bandSize);
    }
    else { // long press
      clearMode++;
      if (clearMode > 5)
      {
        clearMode = 0;
      }
    }
  }

  /* RENDER MODES */
  switch(visualMode) {
    case 1:
      renderAppleFrequencies(); // renders all the bands similar to Apple music visualization bars
      break;
    case 2:
      renderOscilloscope(); // render raw samples
      break;
    case 3:
      renderCircleOscilloscope();
      break;
    case 4:
      renderRadarFrequencies();
      break;
    case 5:
      renderBassMidTrebleLines();
      break;
    case 6:
      renderEnergyLines();
      break;
    case 7:
      renderTriangles();
      break;
    case 8:
      renderFrequenciesHeatmap();
      break;
    case 9:
      renderHeightHistory();
      break;
    case 10:
      renderFrequenciesMatrix();
      break;
    case 0:
    default:
      renderFrequencies();
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
      canvas.fillRect(offset, (SCREEN_HEIGHT - 1) - bands[i], BAND_WIDTH - 1, SCREEN_HEIGHT + 2, getValueColor(bands[i], 0, SCREEN_HEIGHT - 1));
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
          tft.color565(25, 25, 25)                          // color
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

void renderTriangles()
{
  float raw[3] = {audioInfo.getBass(), audioInfo.getMid(), audioInfo.getTreble()};
  float peak[3] = {audioInfo.getBassPeak(), audioInfo.getMidPeak(), audioInfo.getTreblePeak()};
  clearDisplay();
  audioInfo.normalize(true, 0, 20); // normalize all values to range provided.
  float radius;
  float step = (360.0 / (3)) * PI / 180.0;
  static float offsetAngle = (90) * PI / 180.0;
  offsetAngle += raw[0] * 0.005;
  offsetAngle -= raw[2] * 0.0075;
  float x0, x1, x_first;
  float y0, y1, y_first;
  float cx = SCREEN_WIDTH / 2;
  float cy = SCREEN_HEIGHT / 2;
  int OFFSET = 43;
  for (float angle = offsetAngle, i = 0; angle - offsetAngle <= 2 * PI && i < 3; angle += step, i++)
  {
    radius = 5+peak[(int)i];
    x0 = cx + radius * cos(angle);
    y0 = cy + radius * sin(angle);
    if (i > 0)
    {
      for(int jx = -2; jx <= 2; jx++) {
        for (int jy = -1; jy <= 1; jy++)
        {
          canvas.drawLine(x0 + (jx * OFFSET), y0 + (jy * OFFSET), x1 + (jx * OFFSET), y1 + (jy * OFFSET), tft.color565(255, 255, 255));
        }
      }
    }
    else
    {
      x_first = x0;
      y_first = y0;
    }

    x1 = x0;
    y1 = y0;
  }
  for (int jx = -2; jx <= 2; jx++)
  {
    for (int jy = -1; jy <= 1; jy++)
    {
      canvas.drawLine(x0 + (jx * OFFSET), y0 + (jy * OFFSET), x_first + (jx * OFFSET), y_first + (jy * OFFSET), tft.color565(255, 255, 255));
    }
  }
  //canvas.drawLine(x0, y0, x_first, y_first, tft.color565(255, 255, 255));

  tft.pushImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, canvas.getBuffer());
}

void renderBassMidTrebleLines()
{
  audioInfo.normalize(true, 0, 1); // normalize all values to range provided.
  float values[3] = {audioInfo.getBass(), audioInfo.getMid(), audioInfo.getTreble()};
  int x0, x1, y0, y1;
  float wStep = SCREEN_WIDTH / 2;
  clearDisplay();
  for (int i = 0; i < 3-1; i++)
  {
    x0 = (float)i * wStep;
    y0 = values[i] * (SCREEN_HEIGHT / 4.0);
    x1 = (float)(i + 1.0) * wStep;
    y1 = values[i + 1] * (SCREEN_HEIGHT / 4.0);
    canvas.drawLine(x0, SCREEN_HEIGHT / 2 + y0, x1, SCREEN_HEIGHT / 2 + y1, tft.color565(255, 255, 255));
    canvas.drawLine(x0, SCREEN_HEIGHT / 2 - y0, x1, SCREEN_HEIGHT / 2 - y1, tft.color565(255, 255, 255));
    canvas.drawLine(x0, SCREEN_HEIGHT / 2 + y0+1, x1, SCREEN_HEIGHT / 2 + y1+1, tft.color565(255, 255, 255));
    canvas.drawLine(x0, SCREEN_HEIGHT / 2 - y0-1, x1, SCREEN_HEIGHT / 2 - y1-1, tft.color565(255, 255, 255));

    for(int j = 2; j < 10; j += 1) {
      y0 = values[i] * (SCREEN_HEIGHT / 4.0) * (j) + 10;
      y1 = values[i + 1] * (SCREEN_HEIGHT / 4.0) * (j) + 10;
      canvas.drawLine(x0, SCREEN_HEIGHT / 2 + y0, x1, SCREEN_HEIGHT / 2 + y1, tft.color565(values[0] * 255, values[1] * 255, values[2] * 255));
      canvas.drawLine(x0, SCREEN_HEIGHT / 2 - y0, x1, SCREEN_HEIGHT / 2 - y1, tft.color565(values[0] * 255, values[1] * 255, values[2] * 255));
    }
  }

  tft.pushImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, canvas.getBuffer());
}

void renderFrequencyLines()
{
  audioInfo.normalize(true, 0, 1); // normalize all values to range provided.
  float *values = audioInfo.getPeaks();
  float lowMidHigh[3] = {audioInfo.getBass(), audioInfo.getMid(), audioInfo.getTreble()};
  int x0, x1, y0, y1;
  float wStep = (float)SCREEN_WIDTH / ((float)audioInfo.getBandSize() - 1.0);
  clearDisplay();
  for (int i = 0; i < audioInfo.getBandSize() - 1; i++)
  {
    x0 = (float)i * wStep;
    y0 = values[i] * (SCREEN_HEIGHT / 4.0);
    x1 = (float)(i + 1.0) * wStep;
    y1 = values[i + 1] * (SCREEN_HEIGHT / 4.0);
    canvas.drawLine(x0, SCREEN_HEIGHT / 2 + y0, x1, SCREEN_HEIGHT / 2 + y1, tft.color565(255, 255, 255));
    canvas.drawLine(x0, SCREEN_HEIGHT / 2 - y0, x1, SCREEN_HEIGHT / 2 - y1, tft.color565(255, 255, 255));
    canvas.drawLine(x0, SCREEN_HEIGHT / 2 + y0 + 1, x1, SCREEN_HEIGHT / 2 + y1 + 1, tft.color565(255, 255, 255));
    canvas.drawLine(x0, SCREEN_HEIGHT / 2 - y0 - 1, x1, SCREEN_HEIGHT / 2 - y1 - 1, tft.color565(255, 255, 255));

    for (int j = 2; j < 5; j += 1)
    {
      y0 = values[i] * (SCREEN_HEIGHT / 4.0) * (j) + 20;
      y1 = values[i + 1] * (SCREEN_HEIGHT / 4.0) * (j) + 20;
      canvas.drawLine(x0, SCREEN_HEIGHT / 2 + y0, x1, SCREEN_HEIGHT / 2 + y1, tft.color565(lowMidHigh[0] * 255, lowMidHigh[1] * 255, lowMidHigh[2] * 255));
      canvas.drawLine(x0, SCREEN_HEIGHT / 2 - y0, x1, SCREEN_HEIGHT / 2 - y1, tft.color565(lowMidHigh[0] * 255, lowMidHigh[1] * 255, lowMidHigh[2] * 255));
    }
  }

  tft.pushImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, canvas.getBuffer());
}

void renderEnergyLines()
{
  audioInfo.normalize(true, 0, 1.0f); // normalize all values to range provided.
  float raw[3] = {audioInfo.getBass(), audioInfo.getMid(), audioInfo.getTreble()};
  float peak[3] = {audioInfo.getBassPeak(), audioInfo.getMidPeak(), audioInfo.getTreblePeak()};
  float vuPeak = audioInfo.getVolumeUnit();
  uint16_t c565 = tft.color565(peak[0] * 255, peak[1] * 255, peak[2] * 255);
  int x0, x1, y0, y1, cx, cy;
  float value;
  int wStep = SCREEN_WIDTH / 4;
  clearDisplay();
  // //for(int j = 2; j >= 0; j--) {
  //   for (int i = 0; i < 10; i++)
  //   {
  //     // if(j == 0) {
  //     //   c565 = tft.color565(peak[0] * 255, 0, 0);
  //     // }
  //     // else if (j == 1)
  //     // {
  //     //   c565 = tft.color565(0, peak[1] * 255, 0);
  //     // }
  //     // else
  //     // {
  //     //   c565 = tft.color565(0, 0, peak[2] * 255);
  //     // }

  //     //value = peak[j];
  //     value = vuPeak;
  //     cx = (SCREEN_WIDTH / 2) + ((i) * (wStep * (value)));
  //     cy = 50.0 * value;
  //     canvas.drawLine(cx, SCREEN_HEIGHT / 2 - cy, cx, SCREEN_HEIGHT / 2 + cy, c565);
  //     cx = (SCREEN_WIDTH / 2) - ((i) * (wStep * (value)));
  //     canvas.drawLine(cx, SCREEN_HEIGHT / 2 - cy, cx, SCREEN_HEIGHT / 2 + cy, c565);
  //   }
  // //}
  

  for (int i = 0; i < 10; i++)
  {
    // rifht side
    c565 = getValueColor(peak[1] + peak[2], 0, 2);
    cx = (SCREEN_WIDTH / 2) + (i * (wStep * ((peak[0] + peak[1]) + 0.25f)));
    cy = 25;
    // x0 = cx + wStep * 0.5 * (peak[2] + peak[1]);
    // canvas.drawLine(x0, SCREEN_HEIGHT / 2 - cy, x0, SCREEN_HEIGHT / 2 + cy, c565);
    // x0 = cx - wStep * 0.5 * (peak[2] + peak[1]);
    // canvas.drawLine(x0, SCREEN_HEIGHT / 2 - cy, x0, SCREEN_HEIGHT / 2 + cy, c565);
    canvas.drawLine(cx, SCREEN_HEIGHT / 2 - cy, cx, SCREEN_HEIGHT / 2 + cy, getValueColor(peak[0]+peak[1]+peak[2], 0, 1));

    // left side 
    cx = (SCREEN_WIDTH / 2) - (i * (wStep * ((peak[0] + peak[1]) + 0.25f)));
    // x0 = cx + wStep * 0.5 * (peak[2] + peak[1]);
    // canvas.drawLine(x0, SCREEN_HEIGHT / 2 - cy, x0, SCREEN_HEIGHT / 2 + cy, c565);
    // x0 = cx - wStep * 0.5 * (peak[2] + peak[1]);
    // canvas.drawLine(x0, SCREEN_HEIGHT / 2 - cy, x0, SCREEN_HEIGHT / 2 + cy, c565);

    canvas.drawLine(cx, SCREEN_HEIGHT / 2 - cy, cx, SCREEN_HEIGHT / 2 + cy, getValueColor(peak[0]+peak[1]+peak[2], 0, 1));
  }

  tft.pushImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, canvas.getBuffer());
}

void renderHeightHistory()
{
  audioInfo.normalize(true, 0, 255); // normalize all values to range provided.
  float *values = audioInfo.getBands();
  float vu = audioInfo.getVolumeUnit();
  float bassMidHigh[3] = {audioInfo.getBass(), audioInfo.getMid(), audioInfo.getTreble()};

  uint16_t *buffer = canvas.getBuffer();
  uint16_t c565;
  uint16_t step = 1;
  // shift value to the left
  for (int y = 0; y < SCREEN_HEIGHT; y++)
  {
    for (int x = step + SCREEN_WIDTH % step; x < SCREEN_WIDTH; x += step)
    {
      c565 = buffer[y * SCREEN_WIDTH + x];
      buffer[(y * SCREEN_WIDTH) + x - 1] = c565;
    }
  }
  float radius;
  float x0, x1;
  float y0, y1;

  canvas.drawLine(SCREEN_WIDTH - 1, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT, 0);

  if (audioInfo.getBandSize() == 32)
  { // vu
    step = (SCREEN_HEIGHT)/1.5;
    x0 = SCREEN_WIDTH - 1;
    x1 = SCREEN_WIDTH - 1;
    y0 = ((SCREEN_HEIGHT - step) / 2) + ((float)(step / 2.0) * (1.0 - (float)(vu / 255.0)));
    y1 = ((SCREEN_HEIGHT - step) / 2) + step - ((float)(step / 2.0) * (1.0 - (float)(vu / 255.0)));
    c565 = getValueColor(vu, 0, 255);
    canvas.drawLine(x0, y0, x1, y1, c565);
  }
  else if (audioInfo.getBandSize() == 64)
  { // low mid high
    step = (SCREEN_HEIGHT / 3);
    for (int i = 0; i < 3; i++)
    {
      x0 = SCREEN_WIDTH - 1;
      x1 = SCREEN_WIDTH - 1;
      y0 = (3 - 1 - i) * step + ((float)(step / 2.0) * (1.0 - (float)(bassMidHigh[i] / 255.0)));
      y1 = (3 - 1 - i) * step + step - ((float)(step / 2.0) * (1.0 - (float)(bassMidHigh[i] / 255.0)));
      c565 = getValueColor(bassMidHigh[i], 0, 255);
      canvas.drawLine(x0, y0, x1, y1, c565);
    }
  }
  else
  {
    step = (SCREEN_HEIGHT / audioInfo.getBandSize());
    for (int i = 0; i < audioInfo.getBandSize(); i++)
    {
      x0 = SCREEN_WIDTH - 1;
      x1 = SCREEN_WIDTH - 1;
      y0 = (audioInfo.getBandSize() - 1 - i) * step + ((float)(step / 2.0) * (1.0-(float)(values[i]/255.0)));
      y1 = (audioInfo.getBandSize() - 1 - i) * step + step - ((float)(step / 2.0) * (1.0-(float)(values[i] / 255.0)));
      c565 = getValueColor(values[i], 0, 255);
      canvas.drawLine(x0, y0, x1, y1, c565);
    }
  }
  tft.pushImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, canvas.getBuffer());
}

void renderAppleFrequencies()
{
  audioInfo.normalize(true, 0, SCREEN_HEIGHT/2 - 1); // normalize all values to range provided.
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

  clearDisplay();
  // equilizer first audioInfo.getBandSize()
  int BAND_WIDTH = (SCREEN_WIDTH - 36) / audioInfo.getBandSize();
  int START_OFFSET = (SCREEN_WIDTH - (BAND_WIDTH *audioInfo.getBandSize())) / 2;
  int HALF_SCREEN = (float)SCREEN_HEIGHT / 2.0;

  int offset = START_OFFSET;
  for (int i = 0; i < audioInfo.getBandSize(); i++)
  {
    // band frequency
    CHSV hsv = CHSV(i * (230 / audioInfo.getBandSize()), 255, 255);
    CRGB c = hsv;
    uint16_t c16 = tft.color565(255, 255, 255); // tft.color565(c.r, c.g, c.b); // colored bars
    int h = peaks[i];
    int w = max((BAND_WIDTH/2), 1);
    canvas.fillRoundRect(offset + ((BAND_WIDTH  - w) / 2), (HALF_SCREEN)-h, w, h + 1, w / 4, c16);
    canvas.fillRoundRect(offset + ((BAND_WIDTH - w) / 2), (HALF_SCREEN) + 1, w, h + 1, w / 4, c16);
    canvas.fillRect(offset + ((BAND_WIDTH - w) / 2), (HALF_SCREEN) - (h / 2), w, h, c16);
    offset += BAND_WIDTH;
  }
  offset = START_OFFSET;
  for (int i = 0; i < audioInfo.getBandSize(); i++)
  {
    // equlizer curve
    if (i != audioInfo.getBandSize() - 1)
    {
      // upper
      canvas.drawLine(
          offset + (BAND_WIDTH / 2),                           // x1
          (HALF_SCREEN - 1) - (HALF_SCREEN/2 * bandEq[i]),     // y1
          offset + BAND_WIDTH - 1 + (BAND_WIDTH / 2),          // x2
          (HALF_SCREEN - 1) - (HALF_SCREEN/2 * bandEq[i + 1]), // y1
          tft.color565(25, 25, 25)                          // color
      );
      // lower
      canvas.drawLine(
          offset + (BAND_WIDTH / 2),                              // x1
          (HALF_SCREEN - 1) - (HALF_SCREEN / 2 * -bandEq[i]),     // y1
          offset + BAND_WIDTH - 1 + (BAND_WIDTH / 2),             // x2
          (HALF_SCREEN - 1) - (HALF_SCREEN / 2 * -bandEq[i + 1]), // y1
          tft.color565(25, 25, 25)                             // color
      );
    }

    offset += BAND_WIDTH;
  }

  tft.pushImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, canvas.getBuffer());
}

void renderRadarFrequencies()
{
  audioInfo.normalize(true, 0, SCREEN_HEIGHT / 2); // normalize all values to range provided.
  float *peaks = audioInfo.getPeaks();
  clearDisplay();
  float radius;
  float step = (360.0 / (audioInfo.getBandSize())) * PI / 180.0;
  float offsetAngle = (90) * PI / 180.0;
  float x0, x1, x_first;
  float y0, y1, y_first;
  float cx = SCREEN_WIDTH / 2;
  float cy = SCREEN_HEIGHT / 2;

  for (float angle = offsetAngle, i = 0; angle - offsetAngle <= 2 * PI && i < audioInfo.getBandSize(); angle += step, i++)
  {
    radius = peaks[(int)i];
    x0 = cx + radius * cos(angle);
    y0 = cy + radius * sin(angle);
    if (i > 0)
    {
      canvas.drawLine(x0, y0, x1, y1, tft.color565(255, 255, 255));
    }
    else
    {
      x_first = x0;
      y_first = y0;
    }

    x1 = x0;
    y1 = y0;
  }
  canvas.drawLine(x0, y0, x_first, y_first, tft.color565(255, 255, 255));

  tft.pushImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, canvas.getBuffer());
}

void renderFrequenciesHeatmap()
{
  audioInfo.normalize(true, 0, 255); // normalize all values to range provided.
  float *values = audioInfo.getBands();

  uint16_t *buffer = canvas.getBuffer();
  uint16_t c565;
  // shift value to the right
  for (int y = 0; y < SCREEN_HEIGHT; y++)
  {
    for (int x = 1; x < SCREEN_WIDTH; x++)
    {
      c565 = buffer[y * SCREEN_WIDTH + x];
      buffer[(y * SCREEN_WIDTH) + x-1] = c565;
    }
  }
  float radius;
  float step = (SCREEN_HEIGHT / audioInfo.getBandSize());
  float x0, x1;
  float y0, y1;


  canvas.drawLine(SCREEN_WIDTH - 1, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT, 0);
  for (int i = 0; i < audioInfo.getBandSize(); i++)
  {
    x0 = SCREEN_WIDTH - 1;
    x1 = SCREEN_WIDTH - 1;
    y0 = (audioInfo.getBandSize() - 1 - i) * step;
    y1 = (audioInfo.getBandSize() - 1 - i) * step + step;
    c565 = getValueColor(values[i], 0, 255);
    canvas.drawLine(x0, y0, x1, y1, c565);
  }

  tft.pushImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, canvas.getBuffer());
}

void renderFrequenciesMatrix()
{
  audioInfo.normalize(true, 0, 255); // normalize all values to range provided.
  float *values = audioInfo.getPeaks();

  uint16_t *buffer = canvas.getBuffer();
  uint16_t c565;

  // shift value to the right
  clearDisplay();
  float radius;
  int stepX, wTiles;
  int stepY, hTiles;
  float x0, x1;
  float y0, y1;

  switch(audioInfo.getBandSize()) {
    case 2:
      wTiles = 2;
      hTiles = 1;
      break;
    case 4:
      wTiles = 2;
      hTiles = 2;
      break;
    case 8:
      wTiles = 4;
      hTiles = 2;
      break;
    case 16:
      wTiles = 4;
      hTiles = 4;
      break;
    case 32:
      wTiles = 8;
      hTiles = 4;
      break;
    case 64:
      wTiles = 8;
      hTiles = 8;
      break;
  }

  stepX = SCREEN_WIDTH / wTiles;
  stepY = SCREEN_HEIGHT / hTiles;
  for(int y = 0; y < hTiles; y++) {
    for(int x = 0; x < wTiles; x++) {
      int i = y * hTiles + x;
      if(values[i] > 80) {
        x0 = x * stepX;
        y0 = y * stepY;
        canvas.fillRect(x0, y0, stepX - 1, stepY - 1, getValueColor(values[i], 0, 255));
      }
    }
  }
  tft.pushImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, canvas.getBuffer());
}

void renderOscilloscope()
{
  audioInfo.normalize(true, -SCREEN_HEIGHT / 2, SCREEN_HEIGHT / 2); // normalize all values to range provided.
  int triggerIndex = audioInfo.getSampleTriggerIndex();
  clearDisplay();
  int stepSize = (SAMPLE_SIZE / 1.5) / SCREEN_WIDTH;
  float scale = 0;
  float fadeInOutWidth = SCREEN_WIDTH / 8;
  for (int i = triggerIndex + stepSize, x0 = 1; x0 < SCREEN_WIDTH && i < SAMPLE_SIZE; i += stepSize, x0++)
  {
    int x1 = x0-1;
    int y0 = audioInfo.getSample(i); 
    int y1 = audioInfo.getSample(i - stepSize);
    if (x1 < fadeInOutWidth)
    {
      scale = (float)x0 / (fadeInOutWidth);
      y0 = (float)y0 * scale;
      scale = (float)x1 / (fadeInOutWidth);
      y1 = (float)y1 * scale;
    }
    else if (x1 > SCREEN_WIDTH - fadeInOutWidth)
    {
      scale = (float)(SCREEN_WIDTH - x0) / (fadeInOutWidth);
      y0 = (float)y0 * scale;
      scale = (float)(SCREEN_WIDTH - x1) / (fadeInOutWidth);
      y1 = (float)y1 * scale;
    }
    canvas.drawLine(x0, y0 + SCREEN_HEIGHT / 2, x1, y1 + SCREEN_HEIGHT / 2, tft.color565(255, 255, 255));
  }
  tft.pushImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, canvas.getBuffer());
}

void renderCircleOscilloscope()
{
  audioInfo.normalize(true, -SCREEN_HEIGHT / 4, SCREEN_HEIGHT / 4); // normalize all values to range provided.
  int triggerIndex = audioInfo.getSampleTriggerIndex();
  float radius;
  float step = (360.0 / (SAMPLE_SIZE / 2.0)) * PI / 180.0;
  float offsetAngle = (90) * PI / 180.0;
  float x0, x1, x_first;
  float y0, y1, y_first;
  float cx = SCREEN_WIDTH / 2;
  float cy = SCREEN_HEIGHT / 2;
  float scale = 0;
  float fadeInOutWidth = SAMPLE_SIZE / 2 / 8;
  clearDisplay();
  for (float angle = offsetAngle, i = triggerIndex; angle - offsetAngle <= 2 * PI && i < triggerIndex + SAMPLE_SIZE / 2.0; angle += step, i++)
  {
    // TODO: blend with beginning more smoothly
    if (i < (triggerIndex + fadeInOutWidth))
    {
      scale = (float)i / (triggerIndex + fadeInOutWidth);
      radius = audioInfo.getSample(i) * scale;
      radius += audioInfo.getSample((triggerIndex + SAMPLE_SIZE / 2.0)-(i-triggerIndex)+1) * (1 - scale);
    }
    else {
      radius = audioInfo.getSample(i);
    }
    x0 = cx + (radius + SCREEN_HEIGHT / 4) * cos(angle);
    y0 = cy + (radius + SCREEN_HEIGHT / 4) * sin(angle);
    if (i > triggerIndex)
    {
      canvas.drawLine(x0, y0, x1, y1, tft.color565(255, 255, 255));
    }
    else {
      x_first = x0;
      y_first = y0;
    }

    x1 = x0;
    y1 = y0;
  }
  canvas.drawLine(x0, y0, x_first, y_first, tft.color565(255, 255, 255));

  tft.pushImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, canvas.getBuffer());
}

/*
  This function just clears the display in fun ways.
  Press button two to cycle thriough all the clearing modes.
*/
void clearDisplay() {
  uint16_t *buffer = canvas.getBuffer();
  uint16_t c565;
  int r=0, g=0, b=0;

  uint32_t rm = sin8(frame/2 + 85)/3;
  uint32_t gm = sin8(frame/2)/3;
  uint32_t bm = sin8(frame/2 + 170)/3;
  switch (clearMode)
  {
    case 1:
      for(int i = 0; i< SCREEN_HEIGHT * SCREEN_WIDTH; i++) {
        c565 = buffer[i];
        // ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
        r = ((c565 >> 11) & 0b00011111 ) << 3;
        g = ((c565 >> 5)  & 0b00111110 ) << 2;
        b = ((c565)       & 0b00011111 ) << 3;

        r -= 60;
        g -= 60;
        b -= 60;

        if(r < 20) r = 0;
        if(g < 20) g = 0;
        if(b < 20) b = 0;
        buffer[i] = tft.color565(r, g, b);
      }
      break;
    case 2:
      for (int i = 0; i < SCREEN_HEIGHT * SCREEN_WIDTH; i++)
      {
        c565 = buffer[i];
        // ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
        r = ((c565 >> 11) & 0b00011111) << 3;
        g = ((c565 >> 5) & 0b00111110) << 2;
        b = ((c565)&0b00011111) << 3;

        r -= rm;
        g -= gm;
        b -= bm;

        if (r < 20)
          r = 0;
        if (g < 20)
          g = 0;
        if (b < 20)
          b = 0;
        buffer[i] = tft.color565(r, g, b);
      }
      break;
    case 3:
      for (int y = 1; y < SCREEN_HEIGHT / 2 + 1; y++)
      {
        for (int x = 0; x < SCREEN_WIDTH; x++)
        {
          c565 = buffer[y * SCREEN_WIDTH + x];
          r = ((c565 >> 11) & 0b00011111) << 3;
          g = ((c565 >> 5) & 0b00111110) << 2;
          b = ((c565)&0b00011111) << 3;

          r -= rm;
          g -= gm;
          b -= bm;

          if (r < 20)
            r = 0;
          if (g < 20)
            g = 0;
          if (b < 20)
            b = 0;
          buffer[y * SCREEN_WIDTH + x] = tft.color565(r, g, b);
          buffer[((y-1) * SCREEN_WIDTH) + x] = tft.color565(r, g, b);
        }
      }
      for (int y = SCREEN_HEIGHT-1; y > SCREEN_HEIGHT / 2 - 1; y--)
      {
        for (int x = 0; x < SCREEN_WIDTH; x++)
        {
          c565 = buffer[y * SCREEN_WIDTH + x];
          r = ((c565 >> 11) & 0b00011111) << 3;
          g = ((c565 >> 5) & 0b00111110) << 2;
          b = ((c565)&0b00011111) << 3;

          r -= rm;
          g -= gm;
          b -= bm;

          if (r < 20)
            r = 0;
          if (g < 20)
            g = 0;
          if (b < 20)
            b = 0;
          buffer[y * SCREEN_WIDTH + x] = tft.color565(r, g, b);
          buffer[((y + 1) * SCREEN_WIDTH) + x] = tft.color565(r, g, b);
        }
      }
      break;
    case 4:
      for (int y = SCREEN_HEIGHT / 2; y >= 0; y--)
      {
        for (int x = 0; x < SCREEN_WIDTH; x++)
        {
          c565 = buffer[y * SCREEN_WIDTH + x];
          r = ((c565 >> 11) & 0b00011111) << 3;
          g = ((c565 >> 5) & 0b00111110) << 2;
          b = ((c565)&0b00011111) << 3;

          r -= rm;
          g -= gm;
          b -= bm;

          if (r < 20)
            r = 0;
          if (g < 20)
            g = 0;
          if (b < 20)
            b = 0;
          buffer[y * SCREEN_WIDTH + x] = tft.color565(r, g, b);
          buffer[((y + 1) * SCREEN_WIDTH) + x] = tft.color565(r, g, b);
        }
      }
      for (int y = SCREEN_HEIGHT / 2; y < SCREEN_HEIGHT; y++)
      {
        for (int x = 0; x < SCREEN_WIDTH; x++)
        {
          c565 = buffer[y * SCREEN_WIDTH + x];
          r = ((c565 >> 11) & 0b00011111) << 3;
          g = ((c565 >> 5) & 0b00111110) << 2;
          b = ((c565)&0b00011111) << 3;

          r -= rm;
          g -= gm;
          b -= bm;

          if (r < 20)
            r = 0;
          if (g < 20)
            g = 0;
          if (b < 20)
            b = 0;
          buffer[y * SCREEN_WIDTH + x] = tft.color565(r, g, b);
          buffer[((y - 1) * SCREEN_WIDTH) + x] = tft.color565(r, g, b);
        }
      }
      break;
    case 5:
      for (int y = 1; y < SCREEN_HEIGHT / 2 + 1; y++)
      {
        for (int x = 1; x < SCREEN_WIDTH / 2 + 1; x++)
        {
          c565 = buffer[y * SCREEN_WIDTH + x];
          r = ((c565 >> 11) & 0b00011111) << 3;
          g = ((c565 >> 5) & 0b00111110) << 2;
          b = ((c565)&0b00011111) << 3;

          r -= rm;
          g -= gm;
          b -= bm;

          if (r < 20)
            r = 0;
          if (g < 20)
            g = 0;
          if (b < 20)
            b = 0;
          buffer[y * SCREEN_WIDTH + x] = tft.color565(r, g, b);
          buffer[((y - 1) * SCREEN_WIDTH) + x - 1] = tft.color565(r, g, b);
        }
      }
      for (int y = SCREEN_HEIGHT - 1; y > SCREEN_HEIGHT / 2 - 1; y--)
      {
        for (int x = 1; x < SCREEN_WIDTH / 2 + 1; x++)
        {
          c565 = buffer[y * SCREEN_WIDTH + x];
          r = ((c565 >> 11) & 0b00011111) << 3;
          g = ((c565 >> 5) & 0b00111110) << 2;
          b = ((c565)&0b00011111) << 3;

          r -= rm;
          g -= gm;
          b -= bm;

          if (r < 20)
            r = 0;
          if (g < 20)
            g = 0;
          if (b < 20)
            b = 0;
          buffer[y * SCREEN_WIDTH + x] = tft.color565(r, g, b);
          buffer[((y + 1) * SCREEN_WIDTH) + x - 1] = tft.color565(r, g, b);
        }
      }
      for (int y = 1; y < SCREEN_HEIGHT / 2 + 1; y++)
      {
        for (int x = SCREEN_WIDTH - 2; x > SCREEN_WIDTH / 2-1; x--)
        {
          c565 = buffer[y * SCREEN_WIDTH + x];
          r = ((c565 >> 11) & 0b00011111) << 3;
          g = ((c565 >> 5) & 0b00111110) << 2;
          b = ((c565)&0b00011111) << 3;

          r -= rm;
          g -= gm;
          b -= bm;

          if (r < 20)
            r = 0;
          if (g < 20)
            g = 0;
          if (b < 20)
            b = 0;
          buffer[y * SCREEN_WIDTH + x] = tft.color565(r, g, b);
          buffer[((y - 1) * SCREEN_WIDTH) + x + 1] = tft.color565(r, g, b);
        }
      }
      for (int y = SCREEN_HEIGHT - 1; y > SCREEN_HEIGHT / 2 - 1; y--)
      {
        for (int x = SCREEN_WIDTH - 2; x > SCREEN_WIDTH / 2 - 1; x--)
        {
          c565 = buffer[y * SCREEN_WIDTH + x];
          r = ((c565 >> 11) & 0b00011111) << 3;
          g = ((c565 >> 5) & 0b00111110) << 2;
          b = ((c565)&0b00011111) << 3;

          r -= rm;
          g -= gm;
          b -= bm;

          if (r < 20)
            r = 0;
          if (g < 20)
            g = 0;
          if (b < 20)
            b = 0;
          buffer[y * SCREEN_WIDTH + x] = tft.color565(r, g, b);
          buffer[((y + 1) * SCREEN_WIDTH) + x + 1] = tft.color565(r, g, b);
        }
      }
      break;
    case 0:
    default:
      canvas.fillScreen(0x0000);
      break;
  }
}

template <class X, class M, class N, class O, class Q>
X map_Generic(X x, M in_min, N in_max, O out_min, Q out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

uint16_t getValueColor(float value, float min, float max) {
  if (value > max)
  {
    value = max;
  }
  if (value < min)
  {
    value = min;
  }
  float valueScaled = map_Generic(value, min, max, 0.0, 1.0);
  float scale = 0;
  int16_t r=0, g=0, b=0;
  switch (clearMode)
  {
    case 1: {
      r = 255;
      g = (1 - valueScaled) * 255;
      break;
    }
    case 2: {
      r = (float)sin8(frame / 3.0 + 85.0) * valueScaled;
      g = (float)sin8(frame / 3.0) * valueScaled;
      b = (float)sin8(frame / 3.0 + 170.0) * valueScaled;
      break;
    }
    case 3:
    {
      r = sin8(255.0 * valueScaled + 85.0);
      g = sin8(255.0 * valueScaled);
      b = sin8(255.0 * valueScaled + 170.0);
      break;
    }
    case 4: {
      if (valueScaled < .5)
      {
        // blue
        scale = (valueScaled) / .5;
        b = scale * 255.0;
        g = (valueScaled * scale) * 255.0;
      }
      else if (valueScaled < .75)
      {
        // green
        scale = (valueScaled - .5) / .25;
        b = (valueScaled * (1-scale)) * 255.0;
        g = (valueScaled * scale) * 255.0;
        r = (valueScaled * scale) * 255.0;
      }
      else if (valueScaled <= 1)
      {
        // red
        scale = (valueScaled - .75) / .125;
        g = (valueScaled * (1 - scale)) * 255.0;
        if(g < 0) {
          g = 0;
        }
        r = 255;//(valueScaled * scale) * 255.0;
      }
      break;
    }
    case 0:
    default:
      r = g = b = valueScaled * 255;
  }
  return tft.color565(r, g, b);
}