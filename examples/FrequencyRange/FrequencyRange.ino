/*
    Frequencies.ino
    By Shea Ivey

    Reads I2S microphone data into samples[], processes them into frequency buckets and then outputs it to the serial plotter for viewing.
    Try wistling differrent tones to see which frequency buckets they fall into.

    TIP: uncomment the audioInfo.autoLevel() to see how the loud and quiet noises are handled.
*/

#include <AudioInI2S.h>

#define SAMPLE_SIZE 1024  // Buffer size of read samples
#define SAMPLE_RATE 44100 // Audio Sample Rate

/* Required defines for audio analysis */
#define BAND_SIZE 8 // powers of 2 up to 64, defaults to 8

#include <AudioFrequencyAnalysis.h>
AudioFrequencyAnalysis audioInfo;

// ESP32 S2 Mini
// #define MIC_BCK_PIN 4             // Clock pin from the mic.
// #define MIC_WS_PIN 39             // WS pin from the mic.
// #define MIC_DATA_PIN 5            // SD pin data from the mic.
// #define MIC_CHANNEL_SELECT_PIN 40 // Left/Right pin to select the channel output from the mic.

// ESP32 TTGO T-Display
#define MIC_BCK_PIN 32            // Clock pin from the mic.
#define MIC_WS_PIN 25             // WS pin from the mic.
#define MIC_DATA_PIN 33           // SD pin data from the mic.
#define MIC_CHANNEL_SELECT_PIN 27 // Left/Right pin to select the channel output from the mic.

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
  audioInfo.setNoiseFloor(5);       // sets the noise floor
  audioInfo.normalize(true, 0, 255); // normalize all values to range provided.
  vuMeter._inIsolation = true;
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
  Serial.printf("maxFrequency:%d,", vuMeter.getMaxFrequency());
  Serial.printf("vu:%.2f,vuPeak:%.2f,", vuMeter.getValue(0,255), vuMeter.getPeak(0,255));
  Serial.printf("bass:%.2f,bassPeak:%.2f,", bass.getValue(0,255), bass.getPeak(0,255));
  Serial.printf("mid:%.2f,midPeak:%.2f,", mid.getValue(0,255), mid.getPeak(0,255));
  Serial.printf("high:%.2f,highPeak:%.2f", high.getValue(0,255), high.getPeak(0,255));

  Serial.println();
}
