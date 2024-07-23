#ifndef RollingAverage_h
#define RollingAverage_h

#define MAX_ROLLING_AVERAGE_WINDOW 50

/*
    RollingAverage.h
    By Shea Ivey

    https://github.com/sheaivey/ESP32-AudioInI2S
*/

class RollingAverage {
public:
  RollingAverage() {
    // Initialize values array
    resize(MAX_ROLLING_AVERAGE_WINDOW);
  }

  void resize(uint16_t size) {
    windowSize = size;
    index = 0;
    count = 0;
    sum = 0;
    for (int i = 0; i < windowSize; i++) {
      values[i] = 0.0;
    }
  }

  float addValue(float value) {
    // Subtract the oldest value from the sum
    sum -= values[index];

    // Add the new value to the array and sum
    values[index] = value;
    sum += value;

    // Move to the next index
    index = (index + 1) % windowSize;

    // Keep track of the number of values added
    if (count < windowSize) {
      count++;
    }

    return getAverage();
  }

  float getAverage() {
    if(count == 0) {
      return 1;
    }
    // Calculate the average
    return sum / count;
  }

private:
  uint16_t windowSize = MAX_ROLLING_AVERAGE_WINDOW;
  uint16_t index = 0;
  uint16_t count = 0;
  float values[MAX_ROLLING_AVERAGE_WINDOW];
  float sum = 0.0;
};

#endif