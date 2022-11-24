/*
    Frequencies.ino
    By Shea Ivey

    Reads I2S microphone data into samples[], processes them into frequency buckets and then outputs it to an I2C 128x64 oled screen for viewing.
    Try wistling differrent tones to see which frequency buckets they fall into.
*/

#include <AudioInI2S.h>

#define SAMPLE_SIZE 1024  // Buffer size of read samples
#define SAMPLE_RATE 44100 // Audio Sample Rate

/* Required defines for audio analysis */
#define BAND_SIZE 64 // powers of 2 up to 64, defaults to 8
#include <AudioAnalysis.h>
AudioAnalysis audioInfo;

// ESP32 S2 Mini
#define MIC_BCK_PIN 4             // Clock pin from the mic.
#define MIC_WS_PIN 39             // WS pin from the mic.
#define MIC_DATA_PIN 5            // Data pin from the mic.
#define MIC_CHANNEL_SELECT_PIN 40 // Pin to select the channel output from the mic.

AudioInI2S mic(MIC_BCK_PIN, MIC_WS_PIN, MIC_DATA_PIN, MIC_CHANNEL_SELECT_PIN); // defaults to RIGHT channel.

int32_t samples[SAMPLE_SIZE]; // I2S sample data is stored here

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

TwoWire fasterWire = TwoWire(0);

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define FRAME_RATE 30
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &fasterWire, OLED_RESET);
unsigned long nextFrame = 0;

void setup()
{
  mic.begin(SAMPLE_SIZE, SAMPLE_RATE); // Starts the I2S DMA port.

  // audio analysis setup
  audioInfo.setNoiseFloor(10);                     // sets the noise floor
  audioInfo.normalize(true, 0, SCREEN_HEIGHT - 1); // normalize all values to range provided.

  audioInfo.autoLevel(AudioAnalysis::ACCELERATE_FALLOFF, 10, 255, 255); // set auto level falloff rate
  audioInfo.bandPeakFalloff(AudioAnalysis::EXPONENTIAL_FALLOFF, .05);   // set the band peak fall off rate

  audioInfo.setEqualizerLevels(0.75, 1.25, 1.5); // set the equlizer offsets

  // OLED setup
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
}

void loop()
{
  if (nextFrame > millis())
  {
    return;
  }
  // enforce a predictable frame rate
  nextFrame = millis() + (1000 / FRAME_RATE);

  processSamples(); // does all the reading and frequency calculations

  /* RENDER MODES */
  renderFrequencies(); // renders the all the bands to the OLED
}

void processSamples()
{
  mic.read(samples); // Stores the current I2S port buffer into samples.
  audioInfo.computeFFT(samples, SAMPLE_SIZE, SAMPLE_RATE);
  audioInfo.computeFrequencies(BAND_SIZE);
}

void renderFrequencies()
{
  float *bands = audioInfo.getBands();
  float *peaks = audioInfo.getPeaks();
  float *bandEq = audioInfo.getEqualizerLevels();

  display.clearDisplay();

  int offset = 0;
#define BAND_WIDTH SCREEN_WIDTH / BAND_SIZE
#define HALF_SCREEN (float)SCREEN_HEIGHT / 2.0
  for (int i = 0; i < BAND_SIZE; i++)
  {
    // band frequency
    display.fillRect(offset, (SCREEN_HEIGHT - 1) - bands[i], BAND_WIDTH - 1, SCREEN_HEIGHT + 2, SSD1306_WHITE);
    display.drawLine(offset, (SCREEN_HEIGHT - 1) - peaks[i], offset + BAND_WIDTH - 2, (SCREEN_HEIGHT - 1) - peaks[i], SSD1306_WHITE);

    // equlizer curve
    if (i != BAND_SIZE - 1)
    {
      display.drawLine(
          offset + (BAND_WIDTH / 2),                           // x1
          (SCREEN_HEIGHT - 1) - (HALF_SCREEN * bandEq[i]),     // y1
          offset + BAND_WIDTH - 1 + (BAND_WIDTH / 2),          // x2
          (SCREEN_HEIGHT - 1) - (HALF_SCREEN * bandEq[i + 1]), // y1
          SSD1306_WHITE                                        // color
      );
    }
    offset += BAND_WIDTH;
  }

  display.display();
}