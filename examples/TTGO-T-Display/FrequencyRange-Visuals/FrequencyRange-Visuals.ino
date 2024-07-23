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
#define FRAME_RATE 120     // frame every samples 23.219ms

/* Required defines for audio analysis */
#define BAND_SIZE 64 // powers of 2 up to 64, defaults to 8

#include <AudioFrequencyAnalysis.h>
AudioFrequencyAnalysis audioInfo;

FrequencyRange vuRange(0, 20000);
FrequencyRange bassRange(0, 249);
FrequencyRange midRange(250, 1499);
FrequencyRange trebleRange(1500, 16000);

FrequencyRange bands[BAND_SIZE] = {
// 8 bands
  // FrequencyRange(0, 200),
  // FrequencyRange(200, 400),
  // FrequencyRange(400, 900),
  // FrequencyRange(900, 1500),
  // FrequencyRange(1500, 2500),
  // FrequencyRange(2500, 4500),
  // FrequencyRange(4500, 7500),
  // FrequencyRange(7500, 16000),

// 64 bands
  FrequencyRange(43, 85, 0.15), // compensate for bass frequencies that have a lot more amplitude
  FrequencyRange(86, 128, 0.155889126585757),
  FrequencyRange(129, 171, 0.173393297816237),
  FrequencyRange(172, 214, 0.202027411196592),
  FrequencyRange(215, 257, 0.240997914169797),
  FrequencyRange(258, 300, 0.289224796264701),
  FrequencyRange(301, 344, 0.345371520006041),
  FrequencyRange(345, 387, 0.407882061095048),
  FrequencyRange(388, 430, 0.47502403134623),
  FrequencyRange(431, 473, 0.544936689291226),
  FrequencyRange(474, 516, 0.61568250790611),
  FrequencyRange(517, 559, 0.685300870338004),
  FrequencyRange(560, 602, 0.751862405532536),
  FrequencyRange(603, 645, 0.81352245792969),
  FrequencyRange(646, 688, 0.868572209393697),
  FrequencyRange(689, 731, 0.915486036607447),
  FrequencyRange(732, 774, 0.952963791490543),
  FrequencyRange(775, 817, 0.979966832901072),
  FrequencyRange(818, 860, 0.995746811055189),
  FrequencyRange(861, 903, 0.999866406946448),
  FrequencyRange(904, 946, 1),
  FrequencyRange(947, 990, 1),
  FrequencyRange(991, 1033, 1),
  FrequencyRange(1034, 1076, 1),
  FrequencyRange(1077, 1119, 1),
  FrequencyRange(1120, 1162, 1),
  FrequencyRange(1163, 1205, 1),
  FrequencyRange(1206, 1291, 1),
  FrequencyRange(1292, 1377, 1),
  FrequencyRange(1378, 1463, 1),
  FrequencyRange(1464, 1549, 1),
  FrequencyRange(1550, 1679, 1),
  FrequencyRange(1680, 1808, 1),
  FrequencyRange(1809, 1937, 1),
  FrequencyRange(1938, 2066, 1),
  FrequencyRange(2067, 2238, 1),
  FrequencyRange(2239, 2411, 1),
  FrequencyRange(2412, 2583, 1),
  FrequencyRange(2584, 2755, 1),
  FrequencyRange(2756, 2928, 1),
  FrequencyRange(2929, 3100, 1),
  FrequencyRange(3101, 3272, 1),
  FrequencyRange(3273, 3444, 1),
  FrequencyRange(3445, 3746, 1),
  FrequencyRange(3747, 4047, 1),
  FrequencyRange(4048, 4349, 1),
  FrequencyRange(4350, 4650, 1),
  FrequencyRange(4651, 4995, 1),
  FrequencyRange(4996, 5339, 1),
  FrequencyRange(5340, 5684, 1),
  FrequencyRange(5685, 6028, 1),
  FrequencyRange(6029, 6588, 1),
  FrequencyRange(6589, 7148, 1),
  FrequencyRange(7149, 7708, 1),
  FrequencyRange(7709, 8268, 1),
  FrequencyRange(8269, 9043, 1),
  FrequencyRange(9044, 9818, 1),
  FrequencyRange(9819, 10593, 1),
  FrequencyRange(10594, 11369, 1),
  FrequencyRange(11370, 12661, 1),
  FrequencyRange(12662, 13953, 1),
  FrequencyRange(13954, 15245, 1),
  FrequencyRange(15246, 16537, 1),
  FrequencyRange(16538, 19999, 1),
};

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
  audioInfo.setNoiseFloor(1); // sets the noise floor
  audioInfo._autoMin = 20;
  //audioInfo._sampleFalloffType = ROLLING_AVERAGE_FALLOFF;

  vuRange._inIsolation = true;
  bassRange._inIsolation = true;
  midRange._inIsolation = true;
  trebleRange._inIsolation = true;

  for(int i = 0; i < BAND_SIZE; i++) {
    bands[i]._inIsolation = false;
    //bands[i]._scaling = 1;
    audioInfo.addFrequencyRange(&bands[i]);
  }

  // extra helper frequency ranges 
  audioInfo.addFrequencyRange(&vuRange); // volume unit
  audioInfo.addFrequencyRange(&bassRange); // bass
  audioInfo.addFrequencyRange(&midRange); // mid
  audioInfo.addFrequencyRange(&trebleRange); // treble/highs

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
    if(mode > 2) {
      mode = 0;
    }
  }

  if(!digitalRead(BUTTON_PIN2)) {
    while (!digitalRead(BUTTON_PIN2));
      switch(vuRange._maxFalloffType) {
        case NO_FALLOFF:
          vuRange._maxFalloffType = LINEAR_FALLOFF;
          vuRange._maxFalloffRate = 10;
          break;
        case LINEAR_FALLOFF:
          vuRange._maxFalloffType = ACCELERATE_FALLOFF; // next best
          vuRange._maxFalloffRate = 1;
          break;
        case ACCELERATE_FALLOFF:
          vuRange._maxFalloffType = EXPONENTIAL_FALLOFF; // default
          vuRange._maxFalloffRate = .00001;
          break;
        case EXPONENTIAL_FALLOFF:
          vuRange._maxFalloffType = ROLLING_AVERAGE_FALLOFF; // has clipping but looks good
          break;
        case ROLLING_AVERAGE_FALLOFF:
        default:
          vuRange._maxFalloffType = NO_FALLOFF;
      }
      vuRange._maxFalloffType = vuRange._maxFalloffType;
      bassRange._maxFalloffType = vuRange._maxFalloffType;
      midRange._maxFalloffType = vuRange._maxFalloffType;
      trebleRange._maxFalloffType = vuRange._maxFalloffType;
  }

  /* RENDER MODES */
  switch(mode) {
    case 1:
      renderOscilloscope();
    break;    
    case 2:
      renderFrequenciesAndMidi();
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
  audioInfo.loop(samples, SAMPLE_SIZE, SAMPLE_RATE);
}

void renderFrequencies()
{
  float vu = vuRange.getValue(0, SCREEN_HEIGHT - 1);
  float vuPeak = vuRange.getPeak(0, SCREEN_HEIGHT - 1);

  float bass = bassRange.getValue(0, SCREEN_HEIGHT - 1);
  float mid = midRange.getValue(0, SCREEN_HEIGHT - 1);
  float treble = trebleRange.getValue(0, SCREEN_HEIGHT - 1);

  float bassPeak = bassRange.getPeak(0, SCREEN_HEIGHT - 1);
  float midPeak = midRange.getPeak(0, SCREEN_HEIGHT - 1);
  float treblePeak = trebleRange.getPeak(0, SCREEN_HEIGHT - 1);

  canvas.fillScreen(0x0000);
  // equilizer first BAND_SIZE
  int offset = 0;
  int BAND_WIDTH = (SCREEN_WIDTH - 36) / BAND_SIZE;
  int HALF_SCREEN = (float)SCREEN_HEIGHT / 2.0;
  for (int i = 0; i < BAND_SIZE; i++)
  {
    // band frequency
    canvas.fillRect(offset, (SCREEN_HEIGHT - 1) - bands[i].getValue(0, SCREEN_HEIGHT - 1), BAND_WIDTH - 1, SCREEN_HEIGHT + 2, 0xFFFFFF);
    canvas.drawLine(offset, (SCREEN_HEIGHT - 1) - bands[i].getPeak(0, SCREEN_HEIGHT - 1), offset + BAND_WIDTH - 2, (SCREEN_HEIGHT - 1) - bands[i].getPeak(0, SCREEN_HEIGHT - 1), 0xFFFFFF);
    offset += BAND_WIDTH;
  }
  offset = 0;
  for (int i = 0; i < BAND_SIZE; i++)
  {
    // equlizer curve
    if (i != BAND_SIZE - 1)
    {
      canvas.drawLine(
          offset + (BAND_WIDTH / 2),                           // x1
          (SCREEN_HEIGHT - 1) - (HALF_SCREEN * bands[i]._scaling),     // y1
          offset + BAND_WIDTH - 1 + (BAND_WIDTH / 2),          // x2
          (SCREEN_HEIGHT - 1) - (HALF_SCREEN * bands[i + 1]._scaling), // y1
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
  canvas.fillRect(offset, (SCREEN_HEIGHT - 1) - vu, 12, SCREEN_HEIGHT + 2, tft.color565(0, 255, 0));
  canvas.drawLine(offset, (SCREEN_HEIGHT - 1) - vuPeak, offset + 12, (SCREEN_HEIGHT - 1) - vuPeak, 0xFFFFFF);

  tft.pushImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, canvas.getBuffer());
}

void renderOscilloscope()
{
  int triggerIndex = audioInfo.getSampleTriggerIndex();
  canvas.fillScreen(0x0000);
  int stepSize = (SAMPLE_SIZE / 1.5) / SCREEN_WIDTH;
  float scale = 0;
  float fadeInOutWidth = SCREEN_WIDTH / 8;
  for (int i = triggerIndex + stepSize, x0 = 1; x0 < SCREEN_WIDTH && i < SAMPLE_SIZE; i += stepSize, x0++)
  {
    int x1 = x0-1;
    int y0 = audioInfo.getSample(i, -SCREEN_HEIGHT / 2, SCREEN_HEIGHT / 2); 
    int y1 = audioInfo.getSample(i - stepSize, -SCREEN_HEIGHT / 2, SCREEN_HEIGHT / 2);
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


void renderFrequenciesAndMidi()
{
  canvas.fillScreen(0x0000);
  canvas.setTextSize(3);
  canvas.setCursor(0,0);
  canvas.setTextColor(tft.color565(255, 255, 255), tft.color565(0, 0, 0));

  // TODO: only update the max frequency if it crosses a threshold or use rolling average so its not jumping all over the place.

  canvas.printf("Hz: %d\n", vuRange.getMaxFrequency());
  static String note;
  note = midiToNoteName(frequencyToMidi(vuRange.getMaxFrequency()));
  canvas.printf("Note: %s\n", note);
  canvas.setTextSize(2);
  canvas.printf("Bass Hz: %d\n", bassRange.getMaxFrequency());
  canvas.printf("Mid Hz: %d\n", midRange.getMaxFrequency());
  canvas.printf("Treble Hz: %d\n", trebleRange.getMaxFrequency());

  tft.pushImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, canvas.getBuffer());
}


int frequencyToMidi(float frequency) {
    int v = round(69 + 12 * log2(frequency / 440.0));
    if(v < 0 || v > 127) {
      v = -1;
    }
    return v;
}

String midiToNoteName(int midiNote) {
    static const String noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

    if (midiNote < 0 || midiNote > 127) {
        return "--";
    }

    int noteIndex = midiNote % 12; // Find the note within the octave
    int octave = (midiNote / 12) - 1; // Calculate the octave (MIDI note 0 is C-1)

    return noteNames[noteIndex] + octave;
}