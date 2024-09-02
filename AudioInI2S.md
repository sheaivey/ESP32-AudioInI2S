# AudioInI2S Class
This class sets up the ESP32 DMA to read samples from a MEMS microphone.

## Features
* Simple I2S sample reading and setup. Just choose the pins, sample size and sample rate.

## Hardware 
* ESP32, ESP32 S2, ESP32 C2, ESP32 C3
* INMP441 - MEMS Microphone

## AudioInI2S - Class Functions
* `#include <AudioInI2S.h>`
* **AudioInI2S(int bck_pin, int ws_pin, int data_pin, int channel_pin, i2s_channel_fmt_t channel_format)** // pin setup 
* **void begin(int sample_size, int sample_rate = 44100, i2s_port_t i2s_port_number = I2S_NUM_0)** - Starts the I2S DMA port.
* **void read(int32_t _samples[])** - Stores the current I2S port buffer into samples.

## Example
Checkout the `examples/Basic` example folder for audio analysis.
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
                              // Some Mic's have a pin that lets you choose which I2S_CHANNEL_FMT_ONLY_LEFT
                              // or I2S_CHANNEL_FMT_ONLY_RIGHT the data is sent on. It is best to not leave
                              // this pin floating if your mic does have this feature (Tie it to ground or
                              // use this CHANNEL_SELECT_PIN pin).

AudioInI2S mic(BCK_PIN, WS_PIN, DATA_PIN, CHANNEL_SELECT_PIN, I2S_CHANNEL_FMT_ONLY_RIGHT); // defaults to RIGHT channel.

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

## Trouble Shooting
* Samples only filled with zeros
    * Make sure you are reading the correct left or right channel data from your mic. `I2S_CHANNEL_FMT_ONLY_LEFT` or `I2S_CHANNEL_FMT_ONLY_RIGHT`
* Samples randomly read zeros
    * Your MEMS Microphone might have a channel select pin which is floating and needs to be tied to ground or assigned a pin from the esp32. 

## Licensing 
MIT Open Source - Free to use anywhere. 
