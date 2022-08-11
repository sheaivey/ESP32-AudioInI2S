/*
    Basic.ino
    By Shea Ivey

    Reads I2S microphone data into samples[] and then outputs it to the serial plotter for viewing.
    Try wistling differrent tones to see the waveform change.
*/

#include <AudioInI2S.h>

#define SAMPLE_SIZE 1024  // Buffer size of read samples
#define SAMPLE_RATE 44100 // Audio Sample Rate

// ESP32 S2 Mini
#define BCK_PIN 4             // Clock pin from the mic.
#define WS_PIN 39             // WS pin from the mic.
#define DATA_PIN 5            // Data pin from the mic.
#define CHANNEL_SELECT_PIN 40 // Pin to select the channel output from the mic.

AudioInI2S mic(BCK_PIN, WS_PIN, DATA_PIN, CHANNEL_SELECT_PIN); // defaults to RIGHT channel.

int32_t samples[SAMPLE_SIZE]; // I2S sample data is stored here

void setup()
{
  Serial.begin(115200);
  mic.begin(SAMPLE_SIZE, SAMPLE_RATE); // Starts the I2S DMA port.
}

void loop()
{
  mic.read(samples); // Stores the current I2S port buffer into samples.

  // Send data to serial plotter
  for (int i = 0; i < SAMPLE_SIZE; i++)
  {
    Serial.println(samples[i]);
  }
}