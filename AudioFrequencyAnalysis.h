#ifndef AudioFrequencyAnalysis_H
#define AudioFrequencyAnalysis_H

#include "Arduino.h"
#include "RollingAverage.h"

/*
    AudioFrequencyAnalysis.h
    By Shea Ivey

    https://github.com/sheaivey/ESP32-AudioInI2S
*/

// arduinoFFT V2
// See the develop branch on GitHub for the latest info and speedups.
// https://github.com/kosme/arduinoFFT/tree/develop
// if you are going for speed over percision uncomment the lines below.
// #define FFT_SPEED_OVER_PRECISION
// #define FFT_SQRT_APPROXIMATION

#include <arduinoFFT.h>
#ifndef SAMPLE_RATE
#define SAMPLE_RATE 44100
#endif
#ifndef SAMPLE_SIZE
#define SAMPLE_SIZE 1024
#endif
#ifndef BAND_SIZE
#define BAND_SIZE 64
#endif

#ifndef BAND_SIZE_PADDING
#define BAND_SIZE_PADDING 8
#endif

enum falloff_type
{
  NO_FALLOFF = 0,
  LINEAR_FALLOFF = 1,
  ACCELERATE_FALLOFF = 2,
  EXPONENTIAL_FALLOFF = 3,
  ROLLING_AVERAGE_FALLOFF = 4,
};

class AudioFrequencyAnalysis;

class FrequencyRange
{
public:

  AudioFrequencyAnalysis *_audioInfo = nullptr;

  FrequencyRange(); // full 0Hz - 20000Hz range
  FrequencyRange(uint16_t lowHz, uint16_t highHz, float scaling = 1); // scaling for equalizer

  void setAudioInfo(AudioFrequencyAnalysis *audioInfo);

  void loop(); // calculates the value for the current sample frame.

  float getValue(); // returns the raw value
  float getValue(float min, float max); // returns the calculated value
  float getPeak(); // returns the raw peak
  float getPeak(float min, float max); // returns the calculated peak
  
  uint16_t getMaxFrequency(); // gets the max frequency in Hz within the range
  float getMin(); // gets the lowest raw value in the range
  float getMax(); // gets the highest raw value in the range

  float _value = 0;
  float _peak = 0;
  float _min = 0;
  float _max = 1;
  float _scaling = 1;
  int16_t _maxIndex = -1;
  float _autoFloor = 100;

  float _highFrequencyRollOffCompensation = 0; // typically between 0.5 and 1.0, -1 to disable

  falloff_type _maxFalloffType = EXPONENTIAL_FALLOFF;
  float _maxFalloffRate = .000001;
  float _maxFallRate = 0; // -1 use analysis default
  RollingAverage * _maxRollingAverage = nullptr;
  
  falloff_type _peakFalloffType = EXPONENTIAL_FALLOFF;
  float _peakFalloffRate = 2;
  float _peakFallRate = 0; // -1 use analysis default
  RollingAverage * _peakRollingAverage = nullptr;

  float mapAndClip(float x, float in_min, float in_max, float out_min, float out_max);

  boolean _inIsolation = false; // isolate the min/max to this frequency range or all ranges
  uint16_t _lowHz = 0;
  uint16_t _highHz = 20000;
  uint16_t _startSampleIndex = 0;
  uint16_t _endSampleIndex = SAMPLE_SIZE/2;
};


class AudioFrequencyAnalysis
{
public:
  AudioFrequencyAnalysis();
  AudioFrequencyAnalysis(int32_t *samples, int sampleSize, int sampleRate);

  /* FFT Functions */
  void loop(int32_t *samples, int sampleSize, int sampleRate); // calculates FFT on sample data

  void addFrequencyRange(FrequencyRange *_frequencyRange);

  float *getReal();       // gets the Real values after FFT calculation
  float *getImaginary();  // gets the imaginary values after FFT calculation  
  int getSampleRate();    // gets current sample rate
  int getSampleSize();    // gets current sample size

  /* Band Frequency Functions */
  void setNoiseFloor(float noiseFloor);                              // threshold before sounds are registered
  void normalize(bool normalize = true, float min = 0, float max = 1); // normalize all values and constrain to min/max.

  void autoLevel(falloff_type falloffType = EXPONENTIAL_FALLOFF, float falloffRate = 0.01, float min = 10, float max = -1); // auto ballance normalized values to ambient noise levels.

  bool
  isNormalize();      // is normalize enabled
  bool isAutoLevel(); // is auto level enabled

  float getSample(uint16_t index); // gets the raw sample value at index
  float getSample(uint16_t index, float min, float max); // calculates the normalized sample value at index
  uint16_t getSampleTriggerIndex(); // finds the index of the first cross point at zero
  float getSampleMin(); // gets the lowest raw value in the samples
  float getSampleMax(); // gets the highest raw value in the samples

  int sampleSize() {
    return _sampleSize;
  }

  /* Library Settings */
  bool _isAutoLevel = true;
  float _autoMin = 10; // lowest raw value the autoLevel will fall to before stopping. -1 = will auto level down to 0.
  float _autoMax = -1; // highest raw value the autoLevel will rise to before clipping. -1 = will not have any clipping.

  float _min = 0;
  float _max = 0;

  falloff_type _sampleFalloffType = EXPONENTIAL_FALLOFF;
  float _sampleFalloffRate = 0.00001;
  RollingAverage * _samplesRollingAverage = nullptr;

  float mapAndClip(float x, float in_min, float in_max, float out_min, float out_max);

  /* FFT Variables */
  int32_t *_samples = nullptr;
  int _sampleSize = SAMPLE_SIZE;
  int _sampleRate = SAMPLE_RATE;
  float _real[SAMPLE_SIZE];
  float _imag[SAMPLE_SIZE];
  float _weighingFactors[SAMPLE_SIZE];

  FrequencyRange *_frequencyRanges[BAND_SIZE + BAND_SIZE_PADDING]; // allow for extra bands to be monitored
  uint8_t _frequencyRangesLength = 0;

  /* Band Frequency Variables */
  float _noiseFloor = 0;

  /* Samples Variables */
  float _samplesMin = 0;
  float _samplesMax = 1;
  float _autoLevelSamplesMaxFalloffRate; // used for auto level calculation

  ArduinoFFT<float> *_FFT = nullptr;
};

float calculateFalloff(falloff_type falloffType, float falloffRate, float currentRate)
{
  switch (falloffType)
  {
  case LINEAR_FALLOFF:
    return falloffRate;
  case ACCELERATE_FALLOFF:
    return currentRate + falloffRate;
  case EXPONENTIAL_FALLOFF:
    if (currentRate == 0)
    {
      currentRate = falloffRate;
    }
    return currentRate + currentRate;
  case ROLLING_AVERAGE_FALLOFF: // calculated in loop() at set min max
  case NO_FALLOFF:
  default:
    return 0;
  }
}

AudioFrequencyAnalysis::AudioFrequencyAnalysis(int32_t *samples, int sampleSize, int sampleRate)
{
  AudioFrequencyAnalysis();
  _samples = samples;
  _sampleSize = sampleSize;
  _sampleRate = sampleRate;
  for (int i = 0; i < SAMPLE_SIZE; i++)
  {
    _real[i] = 0;
    _imag[i] = 0;
  }
}

AudioFrequencyAnalysis::AudioFrequencyAnalysis()
{
  _samples = nullptr;
}

void AudioFrequencyAnalysis::addFrequencyRange(FrequencyRange *_frequencyRange) {
  _frequencyRange->setAudioInfo(this);
  _frequencyRanges[_frequencyRangesLength] = _frequencyRange;
  _frequencyRangesLength++;
}

void AudioFrequencyAnalysis::loop(int32_t *samples, int sampleSize, int sampleRate)
{
  _samples = samples;
  if (_FFT == nullptr || _sampleSize != sampleSize || _sampleRate != sampleRate)
  {
    _sampleSize = sampleSize;
    _sampleRate = sampleRate;
    _FFT = new ArduinoFFT<float>(_real, _imag, _sampleSize, _sampleRate, _weighingFactors);
  }

  if(_sampleFalloffType != ROLLING_AVERAGE_FALLOFF) {
    if (_isAutoLevel)
    {
      _autoLevelSamplesMaxFalloffRate = ::calculateFalloff(_sampleFalloffType, _sampleFalloffRate, _autoLevelSamplesMaxFalloffRate);
      _samplesMax -= _autoLevelSamplesMaxFalloffRate;
    }
  }
  else if(_samplesRollingAverage == nullptr) {
    // create it;
    _samplesRollingAverage = new RollingAverage();
  }


  // prep samples for analysis
  for (int i = 0; i < _sampleSize; i++)
  {
    _real[i] = samples[i];
    _imag[i] = 0;
    float v = abs(samples[i]);
    if(_sampleFalloffType == ROLLING_AVERAGE_FALLOFF) {
      float _temp = _samplesMax;
      if(_samplesMax > v) {
        _temp = ((_samplesMax - v) * 0.5) + v; // bring max down over time
        //_temp *= 0.90; // bring max down by 10% over time
      }
      else if(v > _samplesMax) {
        _temp = v;
      }
      _samplesRollingAverage->addValue(_temp);
      _samplesMax = _samplesRollingAverage->getAverage();
    }
    else {
      if (v > _samplesMax)
      {
        _samplesMax = v;
        _autoLevelSamplesMaxFalloffRate = 0;
      }
    }
    if (v < _samplesMin)
    {
      _samplesMin = v;
    }
  }

  _FFT->dcRemoval();
  _FFT->windowing(FFTWindow::Hamming, FFTDirection::Forward, false); /* Weigh data (compensated) */
  _FFT->compute(FFTDirection::Forward);                              /* Compute FFT */
  _FFT->complexToMagnitude();                                        /* Compute magnitudes */


  uint8_t seen = 0;
  _min = 0xFFFFFFFF;
  _max = 0;
  for(int i = 0; seen <_frequencyRangesLength && i < BAND_SIZE + BAND_SIZE_PADDING; i++) {
    if(_frequencyRanges[i] != nullptr) {
      _frequencyRanges[i]->loop();
      seen++;
      if(!_frequencyRanges[i]->_inIsolation) {
        if(_frequencyRanges[i]->_min < _min) {
          _min = _frequencyRanges[i]->_min;
        }
        if(_frequencyRanges[i]->_max > _max) {
          _max = _frequencyRanges[i]->_max;
        }
      }
    }
  }

}

int AudioFrequencyAnalysis::getSampleSize()
{
  return _sampleSize;
}

int AudioFrequencyAnalysis::getSampleRate()
{
  return _sampleRate;
}

float *AudioFrequencyAnalysis::getReal()
{
  return _real;
}

float *AudioFrequencyAnalysis::getImaginary()
{
  return _imag;
}

void AudioFrequencyAnalysis::setNoiseFloor(float noiseFloor)
{
  _noiseFloor = noiseFloor;
}


float AudioFrequencyAnalysis::mapAndClip(float x, float in_min, float in_max, float out_min, float out_max)
{
  if(in_max - in_min == 0) {
    in_max = 1; // divide by zero!
  }

  if (_isAutoLevel && _autoMax != -1 && x > _autoMax)
  {
    // clip the value to max
    x = _autoMax;
  }
  else if (x > in_max)
  {
    // value is clipping
    x = in_max;
  }
  else if (x < in_min)
  {
    // value is clipping
    x = in_min;
  }
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


void AudioFrequencyAnalysis::autoLevel(falloff_type falloffType, float falloffRate, float min, float max)
{
  _isAutoLevel = falloffType != NO_FALLOFF;
  _sampleFalloffType = falloffType;
  _sampleFalloffRate = falloffRate;
  _autoMin = min;
  _autoMax = max;
}

bool AudioFrequencyAnalysis::isAutoLevel()
{
  return _isAutoLevel;
}

float AudioFrequencyAnalysis::getSample(uint16_t index)
{
  float value = 0;
  if (_samples)
  {
    value = (float)_samples[index];
    if (index < 0 || index >= _sampleSize)
    {
      value = 0; // make zero
    }
  }

  return value; // raw value
}

float AudioFrequencyAnalysis::getSample(uint16_t index, float min, float max)
{
  float value = 0;
  if (_samples)
  {
    value = (float)_samples[index];
    if (index < 0 || index >= _sampleSize)
    {
      value = 0; // make zero
    }
  }

  float _tempSamplesMax = _samplesMax <= _autoMin * (float)0xFFFF ? _autoMin * 0xFFFF : _samplesMax;
  return mapAndClip(value, -_tempSamplesMax, _tempSamplesMax, min, max);
  return value;
}

uint16_t AudioFrequencyAnalysis::getSampleTriggerIndex()
{
  if (!_samples)
  {
    return 0;
  }
#define ZERO_RANGE 0
  for (int i = 0; i < (_sampleSize/2 - 1); i++)
  {
    float a = _samples[i];
    float b = _samples[i + 1];
    if (a >= ZERO_RANGE && b < -ZERO_RANGE)
    {
      return i;
    }
  }
  return 0;
}

float AudioFrequencyAnalysis::getSampleMin()
{
  return _samplesMin;
}

float AudioFrequencyAnalysis::getSampleMax()
{
  return _samplesMax;
}



FrequencyRange::FrequencyRange(uint16_t lowHz, uint16_t highHz, float scaling) {
  _lowHz = lowHz;
  _highHz = highHz;
  _scaling = scaling;
}

void FrequencyRange::setAudioInfo(AudioFrequencyAnalysis *audioInfo) { // gets called from AudioFrequencyAnalysis::addFrequencyRange();
  _audioInfo = audioInfo;
  // Calculate FFT index from frequency.
  float lowIndex = (float)(_lowHz * _audioInfo->_sampleSize) / (float)_audioInfo->_sampleRate;
  float highIndex = (float)(_highHz * _audioInfo->_sampleSize) / (float)_audioInfo->_sampleRate;
  if(highIndex-lowIndex <= 1.0) {
    _startSampleIndex = floor(lowIndex);
    _endSampleIndex = _startSampleIndex + 1;
  }
  else {
    _startSampleIndex = round(lowIndex);
    _endSampleIndex = round(highIndex);
  }
}

void FrequencyRange::loop() {
  uint16_t offset = _startSampleIndex;
  uint16_t end = _endSampleIndex - _startSampleIndex;

  if(_maxFalloffType != ROLLING_AVERAGE_FALLOFF) {
    _maxFallRate = calculateFalloff(_maxFalloffType, _maxFalloffRate, _maxFallRate);
    _max -= _maxFallRate;
    if(_max < _peak) {
      _max = _peak;
    }
  }
  else if(_maxRollingAverage == nullptr) {
    // create it;
    _maxRollingAverage = new RollingAverage();
  }
  
  if(_max < _autoFloor) {
    _max = _autoFloor; // prevents divide by zero
  }

  // apply peak fall rate
  if(_peakFalloffType != ROLLING_AVERAGE_FALLOFF) {
    _peakFallRate = calculateFalloff(_peakFalloffType, _peakFalloffRate, _peakFallRate);
    _peak -= _peakFallRate;
    if(_peak < _value) {
      _peak = _value;
    }
  }
  else if(_peakRollingAverage == nullptr) {
    // create it;
    _peakRollingAverage = new RollingAverage();
  }

  // reset value
  _value = 0;
  _maxIndex = -1;
  float maxRv = 0;

  for (int i = _startSampleIndex; i < _endSampleIndex; i++)
  {
    // scale down factor to prevent overflow
    float rv = (_audioInfo->_real[i] / (float)(0xFFFF * 0xFF));
    float iv = (_audioInfo->_imag[i] / (float)(0xFFFF * 0xFF));
    // some smoothing with imaginary numbers.
    rv = sqrt(rv * rv + iv * iv);
    // apply eq scaling
    rv = rv * _scaling;

    rv = rv < _audioInfo->_noiseFloor ? 0 : rv;

    if(_highFrequencyRollOffCompensation > 0) {
      uint16_t frequency = (i * _audioInfo->_sampleRate) / _audioInfo->_sampleSize;
      rv = rv * pow(frequency, _highFrequencyRollOffCompensation);
    }

    if(rv > maxRv) {
      maxRv = rv;
      _maxIndex = i;
    }
    // combine band amplitudes for current band segment
    _value += rv;

  }

  // remove noise
  if (_value < _audioInfo->_noiseFloor)
  {
    _value = 0;
  }

  if(_peakFalloffType == ROLLING_AVERAGE_FALLOFF) {
    float _temp = _peak;
    if(_peak > _value) {
      _temp = ((_peak - _value) * 0.5) + _value; // bring max down over time
      //_temp *= 0.90; // bring max down by 10% over time
    }
    else if(_value > _peak) {
      _temp = _value;
    }
    _peakRollingAverage->addValue(_temp);
    _peak = _peakRollingAverage->getAverage();
  }
  else {
    if (_value > _peak)
    {
      _peakFallRate = 0;
      _peak = _value;
    }
  }

  // handle min/max
  if(_maxFalloffType == ROLLING_AVERAGE_FALLOFF) {
    float _temp = _max;
    if(_max > _value) {
      _temp = ((_max - _value) * 0.5) + _value; // bring max down over time
      //_temp *= 0.90; // bring max down by 10% over time
    }
    else if(_value > _max) {
      _temp = _value;
    }

    _maxRollingAverage->addValue(_temp);
    _max = _maxRollingAverage->getAverage();
  }
  else {  
    if (_value > _max)
    {
      _maxFallRate = 0;
      _max = _value;
    }
  }
  if (_value < _min)
  {
    _min = _value;
  }

  if(_max < _audioInfo->_autoMin) {
    _max = _audioInfo->_autoMin;
  }
}

float FrequencyRange::getMin() {
  return _min; // raw value
}

float FrequencyRange::getMax() {
  return _max; // raw value
}

uint16_t FrequencyRange::getMaxFrequency() {
  if(_maxIndex == -1) {
    return 0;
  }
  return (_maxIndex * _audioInfo->_sampleRate) / _audioInfo->_sampleSize;
}

float FrequencyRange::getValue(float min, float max) {
  if(!_inIsolation) {
    return mapAndClip(_value, 0, _audioInfo->_max, min, max);
  }
  // normalize _min/_max
  return mapAndClip(_value, 0, _max, min, max) ;
}

float FrequencyRange::getValue() {
  // apply scaling
  return _value;
}

float FrequencyRange::getPeak(float min, float max) {
  if(!_inIsolation) {
    return mapAndClip(_peak, 0, _audioInfo->_max, min, max);
  }

  // normalize _min/_max
  return mapAndClip(_peak, 0, _max, min, max);
}


float FrequencyRange::getPeak() {
  // apply scaling
  return _peak;
}

float FrequencyRange::mapAndClip(float x, float in_min, float in_max, float out_min, float out_max)
{
  if(in_max - in_min == 0) {
    in_max = 1; // divide by zero!
  }

  if (x > _max)
  {
    // clip the value to max
    x = _max;
  }
  else if (x > in_max)
  {
    // value is clipping
    x = in_max;
  }
  else if (x < in_min)
  {
    // value is clipping
    x = in_min;
  }
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#endif // AudioFrequencyAnalysis_H