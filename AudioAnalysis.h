#ifndef AudioAnalysis_H
#define AudioAnalysis_H

#include "Arduino.h"
/*
    AudioAnalysis.h
    By Shea Ivey

    https://github.com/sheaivey/ESP32-AudioInI2S
*/

// arduinoFFT V2
// See the develop branch on GitHub for the latest info and speedups.
// https://github.com/kosme/arduinoFFT/tree/develop
// if you are going for speed over precision uncomment the lines below.
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

class AudioAnalysis
{
public:
  enum falloff_type
  {
    NO_FALLOFF = 0,
    LINEAR_FALLOFF = 1,
    ACCELERATE_FALLOFF = 2,
    EXPONENTIAL_FALLOFF = 3,
  };

  AudioAnalysis();
  AudioAnalysis(int32_t *samples, int sampleSize, int sampleRate, int bandSize);

  /* FFT Functions */
  void computeFFT(int32_t *samples, int sampleSize, int sampleRate); // calculates FFT on sample data
  float *getReal();                                                  // gets the Real values after FFT calculation
  float *getImaginary();                                             // gets the imaginary values after FFT calculation

  /* Band Frequency Functions */
  void setNoiseFloor(float noiseFloor);                         // threshold before sounds are registered
  void computeFrequencies(uint8_t bandSize = -1);               // converts FFT data into frequency bands
  void normalize(bool normalize = true, float min = 0, float max = 1); // normalize all values and constrain to min/max.

  void autoLevel(falloff_type falloffType = EXPONENTIAL_FALLOFF, float falloffRate = 0.01, float min = 10, float max = -1); // auto ballance normalized values to ambient noise levels.
                                                                                                                            // min and max are based on pre-normalized values.
  void setEqualizerLevels(float low = 1, float mid = 1, float high = 1 );   // adjust the frequency levels for a given range - low, medium and high.
                                                                            // 0.5 = 50%, 1.0 = 100%, 1.5 = 150%  the raw value etc.
  void setEqualizerLevels(float *bandEq);                                   // full control over each bands eq value.
  float *getEqualizerLevels(); // gets the last bandEq levels

  bool
  isNormalize();      // is normalize enabled
  bool isAutoLevel(); // is auto level enabled
  bool isClipping();  // is values exceeding max

  void bandPeakFalloff(falloff_type falloffType = EXPONENTIAL_FALLOFF, float falloffRate = 0.5); // set the falloff type and rate for band peaks.
  void vuPeakFalloff(falloff_type falloffType = EXPONENTIAL_FALLOFF, float falloffRate = 0.5);   // set the falloff type and rate for volume unit peak.
  void samplesFalloff(falloff_type falloffType = EXPONENTIAL_FALLOFF, float falloffRate = 0.5);  // set the falloff type and rate for sample values.

  float getBass();       // gets the current bass range value
  float getMid();        // gets the current mid range value
  float getTreble();     // gets the current tweet range value
  float getBassPeak();   // gets the peak bass range value
  float getMidPeak();    // gets the peak mid range value
  float getTreblePeak(); // gets the peak tweet range value

  float *getBands(); // gets the last bands calculated from computeFrequencies()
  float *getPeaks(); // gets the last peaks calculated from computeFrequencies()

  uint16_t *getBandNames(); // gets the band names calculated from calculateFrequencyOffsets()
  uint16_t getBandName(uint8_t index); // gets the band name at index calculated from calculateFrequencyOffsets()

  void setBandSize(uint8_t bandSize = BAND_SIZE); // gets the current calculated _bandSize;
  int getBandSize();            // gets the current _bandSize;
  float getBand(uint8_t index); // gets the value at bands index
  float getBandAvg();           // average value across all bands
  float getBandMax();           // max value across all bands
  int getBandMaxIndex();        // index of the highest value band
  int getBandMinIndex();        // index of the lowest value band

  float getPeak(uint8_t index); // gets the value at peaks index
  float getPeakAvg();           // average value across all peaks
  float getPeakMax();           // max value across all peaks
  int getPeakMaxIndex();        // index of the highest value peak
  int getPeakMinIndex();        // index of the lowest value peak

  /* Volume Unit Functions */
  float getVolumeUnit();        // gets the last volume unit calculated from computeFrequencies()
  float getVolumeUnitPeak();    // gets the last volume unit peak calculated from computeFrequencies()
  float getVolumeUnitMax();     // value of the highest value volume unit
  float getVolumeUnitPeakMax(); // value of the highest value volume unit

  float getSample(uint16_t index); // calculates the normalized sample value at index
  uint16_t getSampleTriggerIndex(); // finds the index of the first cross point at zero
  float getSampleMin(); // gets the lowest value in the samples
  float getSampleMax(); // gets the highest value in the samples

  int sampleSize() {
    return _sampleSize;
  }

  int bandSize()
  {
    return _bandSize;
  }

protected:
  /* Library Settings */
  bool _isAutoLevel = false;
  bool _isClipping = false;
  float _autoMin = 10; // lowest raw value the autoLevel will fall to before stopping. -1 = will auto level down to 0.
  float _autoMax = -1; // highest raw value the autoLevel will rise to before clipping. -1 = will not have any clipping.

  bool _isNormalize = false;
  float _normalMin = 0;
  float _normalMax = 1;

  falloff_type _bandPeakFalloffType = EXPONENTIAL_FALLOFF;
  float _bandPeakFalloffRate = 0.5;
  falloff_type _vuPeakFalloffType = EXPONENTIAL_FALLOFF;
  float _vuPeakFalloffRate = 0.5;
  falloff_type _autoLevelFalloffType = EXPONENTIAL_FALLOFF;
  float _autoLevelFalloffRate = 0.001;
  falloff_type _sampleLevelFalloffType = EXPONENTIAL_FALLOFF;
  float _sampleLevelFalloffRate = 0.001;

  float calculateFalloff(falloff_type falloffType, float falloffRate, float currentRate);
  float mapAndClip(float x, float in_min, float in_max, float out_min, float out_max);

  /* FFT Variables */
  int32_t *_samples = nullptr;
  int _sampleSize = SAMPLE_SIZE;
  int _sampleRate = SAMPLE_RATE;
  float _real[SAMPLE_SIZE];
  float _imag[SAMPLE_SIZE];
  float _weighingFactors[SAMPLE_SIZE];

  /* Band Frequency Variables */
  float _noiseFloor = 0;
  int _bandSize = BAND_SIZE;
  float _bands[BAND_SIZE];
  float _peaks[BAND_SIZE];
  float _peakFallRate[BAND_SIZE];
  float _peaksNorms[BAND_SIZE];
  float _bandsNorms[BAND_SIZE];
  float _bandEq[BAND_SIZE];
  float _low = 1;
  float _mid = 1;
  float _high = 1;
  bool _lowMidHighEq = false;
  float _frequencyOffsets[BAND_SIZE];
  uint16_t _frequencyNames[BAND_SIZE];
  void calculateFrequencyOffsets();
  uint16_t _bassMidTrebleWidths[3];
  uint16_t * getBassMidTrebleWidths();

  float _bandAvg = 0;
  float _peakAvg = 0;
  int8_t _bandMinIndex = 0;
  int8_t _bandMaxIndex = 0;
  int8_t _peakMinIndex = 0;
  int8_t _peakMaxIndex = 0;
  float _bandMin = 0;
  float _bandMax = 1; // used for normalization calculation
  float _peakMin = 0;
  float _autoLevelPeakMax = 1; // used for normalization calculation
  float _autoLevelPeakMaxFalloffRate; // used for auto level calculation

  /* Volume Unit Variables */
  float _vu = 0;
  float _vuPeak = 0;
  float _vuPeakFallRate;
  float _vuMin = 0;
  float _vuMax = 1; // used for normalization calculation
  float _vuPeakMin = 0;
  float _autoLevelVuPeakMax = 1; // used for normalization calculation
  float _autoLevelMaxFalloffRate; // used for auto level calculation

  /* Samples Variables */
  float _samplesMin = 0;
  float _samplesMax = 1;
  float _autoLevelSamplesMaxFalloffRate; // used for auto level calculation

  ArduinoFFT<float> *_FFT = nullptr;
};

AudioAnalysis::AudioAnalysis(int32_t *samples, int sampleSize, int sampleRate, int bandSize)
{
  AudioAnalysis();

  _samples = samples;
  _sampleSize = sampleSize;
  _sampleRate = sampleRate;
  for (int i = 0; i < SAMPLE_SIZE; i++)
  {
    _real[i] = 0;
    _imag[i] = 0;
    //_weighingFactors[i] = 0;
  }
  for (int i = 0; i < BAND_SIZE; i++)
  {
    _bands[i] = 0;
    _peaks[i] = 0;
    _peakFallRate[i] = 0;
    _peaksNorms[i] = 0;
    _bandsNorms[i] = 0;
    _bandEq[i] = 1;
    // _frequencyOffsets[i] = 0; // gets set in setBandSize()
  }
  setBandSize(bandSize);
}

AudioAnalysis::AudioAnalysis()
{
  _samples = nullptr;
  // set default eq levels;
  for (int i = 0; i < _bandSize; i++)
  {
    _bandEq[i] = 1.0;
  }
  calculateFrequencyOffsets();
}

void AudioAnalysis::computeFFT(int32_t *samples, int sampleSize, int sampleRate)
{
  _samples = samples;
  if (_FFT == nullptr || _sampleSize != sampleSize || _sampleRate != sampleRate)
  {
    _sampleSize = sampleSize;
    _sampleRate = sampleRate;
    _FFT = new ArduinoFFT<float>(_real, _imag, _sampleSize, _sampleRate, _weighingFactors);
  }

  if (_isAutoLevel)
  {
    // if (_samplesMax > _autoMin * 0x1FFFF)
    // {
    _autoLevelSamplesMaxFalloffRate = calculateFalloff(_sampleLevelFalloffType, _sampleLevelFalloffRate, _autoLevelSamplesMaxFalloffRate);
    _samplesMax -= _autoLevelSamplesMaxFalloffRate;
    // }
    // if (_samplesMax < _autoMin * 0x1FFFF)
    // {
    //   _samplesMax = _autoMin * 0x1FFFF;
    // }
  }

  // prep samples for analysis
  for (int i = 0; i < _sampleSize; i++)
  {
    _real[i] = samples[i];
    _imag[i] = 0;
    if (abs(samples[i]) > _samplesMax)
    {
      _samplesMax = abs(samples[i]);
      _autoLevelSamplesMaxFalloffRate = 0;
    }
    if (abs(samples[i]) < _samplesMin)
    {
      _samplesMin = abs(samples[i]);
    }
  }

  _FFT->dcRemoval();
  _FFT->windowing(FFTWindow::Hamming, FFTDirection::Forward, false); /* Weigh data (compensated) */
  _FFT->compute(FFTDirection::Forward);                              /* Compute FFT */
  _FFT->complexToMagnitude();                                        /* Compute magnitudes */
}

float *AudioAnalysis::getReal()
{
  return _real;
}

float *AudioAnalysis::getImaginary()
{
  return _imag;
}

void AudioAnalysis::setNoiseFloor(float noiseFloor)
{
  _noiseFloor = noiseFloor;
}

float getPoint(float n1, float n2, float percent)
{
  float diff = n2 - n1;

  return n1 + (diff * percent);
}

uint16_t * AudioAnalysis::getBassMidTrebleWidths() {
  _bassMidTrebleWidths[0] = max(1, (_bandSize / 10)); // 40Hz < bass < 400Hz
  _bassMidTrebleWidths[1] = max(1, (int)((float)(_bandSize - _bassMidTrebleWidths[0]) / 3.5)); // 400Hz < mid < 1800Hz
  _bassMidTrebleWidths[2] = max(1, (_bandSize - _bassMidTrebleWidths[0] - _bassMidTrebleWidths[1])); // 1800Hz < treble < 17000Hz
  return _bassMidTrebleWidths;
};

void AudioAnalysis::setEqualizerLevels(float low, float mid, float high)
{
  _low = low;
  _mid = mid;
  _high = high;
  _lowMidHighEq = true;
  uint16_t * widths = getBassMidTrebleWidths();
  float xa, ya, xb, yb, x, y;
  // low curve
  float x1 = 0;
  float lowSize = widths[0];
  float y1 = low;
  float x2 = lowSize / 2;
  float y2 = low;
  float x3 = lowSize;
  float y3 = (low + mid) / 2.0;
  for (int i = x1; i < lowSize; i++)
  {
    // TODO: fix the curve to use x position
    float p = (float)i / (float)lowSize;
    //xa = getPoint(x1, x2, p);
    ya = getPoint(y1, y2, p);
    //xb = getPoint(x2, x3, p);
    yb = getPoint(y2, y3, p);

    //x = getPoint(xa, xb, p);
    y = getPoint(ya, yb, p);

    _bandEq[i] = y;
  }

  // mid curve
  x1 = lowSize;
  float midSize = widths[1];
  y1 = y3;
  x2 = x1 + widths[0];
  y2 = mid;
  x3 = x1 + midSize;
  y3 = (mid + high) / 2.0;
  for (int i = x1; i < x1+midSize; i++)
  {
    float p = (float)(i - x1) / (float)midSize;
    // xa = getPoint(x1, x2, p);
    ya = getPoint(y1, y2, p);
    // xb = getPoint(x2, x3, p);
    yb = getPoint(y2, y3, p);

    // x = getPoint(xa, xb, p);
    y = getPoint(ya, yb, p);

    _bandEq[i] = y;
  }

  // high curve
  x1 = lowSize + midSize;
  float highSize = widths[2];
  y1 = y3;
  x2 = x1 + highSize / 2;
  y2 = high;
  x3 = x1 + highSize;
  y3 = high;
  for (int i = x1; i < x1+highSize; i++)
  {
    float p = (float)(i - x1) / (float)highSize;
    // xa = getPoint(x1, x2, p);
    ya = getPoint(y1, y2, p);
    // xb = getPoint(x2, x3, p);
    yb = getPoint(y2, y3, p);

    // x = getPoint(xa, xb, p);
    y = getPoint(ya, yb, p);

    _bandEq[i] = y;
  }
}

void AudioAnalysis::setEqualizerLevels(float *bandEq)
{
  _lowMidHighEq = false;
  // blind copy of eq percentages
  for(int i = 0; i < _bandSize; i++) {
    _bandEq[i] = bandEq[i];
  }
}

float *AudioAnalysis::getEqualizerLevels()
{
  return _bandEq;
}

// this look Up Table is used to normalize the buckets against each other. Visually makes the higher frequencies appear to be more equal to the lower frequencies. 
float lut[] PROGMEM = {0.0006637301302, 0.0006793553648, 0.0006966758032, 0.0007158753602, 0.0007371579043, 0.0007607494216, 0.0007869004159, 0.0008158885684, 0.0008480216863, 0.0008836409716, 0.0009231246432, 0.0009668919541, 0.001015407642, 0.001069186866, 0.001128800673, 0.001194882066, 0.001268132722, 0.001349330446, 0.001439337425, 0.001539109388, 0.001649705751, 0.00177230087, 0.001908196507, 0.002058835652, 0.002225817851, 0.002410916183, 0.002616096095, 0.002843536264, 0.003095651737, 0.003375119574, 0.00368490727, 0.004028304269, 0.004408956893, 0.004830907057, 0.005298635188, 0.005817107803, 0.006391830243, 0.00702890513, 0.007735097169, 0.008517904978, 0.009385640709, 0.01034751831, 0.01141375137, 0.01259566156, 0.01390579885, 0.01535807478, 0.01696791017, 0.01875239887, 0.02073048926, 0.02292318547, 0.02535377038, 0.02804805287, 0.03103464187, 0.03434525011, 0.03801503091, 0.04208295139, 0.04659220631, 0.05159067664, 0.05713143806, 0.06327332449, 0.07008155284, 0.07762841548, 0.08599404787, 0.09526727952};
void AudioAnalysis::calculateFrequencyOffsets()
{
  // lookup table 64 buckets
  float maxValue = ((float)_sampleSize / 2.0) * 0.7516249323;
  uint16_t total = 0;
  // Serial.print("frequencyOffsets Size: ");
  // Serial.println(_bandSize);
  // Serial.print("sample goal: ");
  // Serial.println(maxValue);
  int stepSize = 64.0 / (float)_bandSize;
  int offset = 0;
  float v = 0;
  // Serial.print("Step Size: ");
  // Serial.println(stepSize);
  for (int i = 0; i < BAND_SIZE; i++)
  {
    _frequencyOffsets[i] = 0;
  }
  for (int i = 0; i * stepSize < 64; i++)
  {
    offset = i * stepSize;
    v = 0;
    for (int j = 0; offset + j < offset + stepSize && offset + j < 64; j++)
    {
      v += lut[(offset + j)];
    }
    v = v * maxValue;
    // if(v < 1) {
    //   v = 1;
    // }
    _frequencyOffsets[i] = v;
    total += v;
    // Serial.printf("index: %2d = ", (int)i);
    // Serial.println((int)v);
  }
  
  offset = 0;
  _frequencyNames[0] = 20;
  for (int i = 1; i < _bandSize; i++)
  {
    offset += ceil(_frequencyOffsets[i-1]);
    _frequencyNames[i] = offset * _sampleRate / _sampleSize;
  }
  
  // Serial.print("SUM: ");
  // Serial.println(total);
}

void AudioAnalysis::computeFrequencies(uint8_t bandSize)
{
  setBandSize(bandSize);
  if (!_samples)
  {
    return;
  }
  _isClipping = false;
  // for normalize falloff rates
  if (_isAutoLevel)
  {
    if (_autoLevelPeakMax > _autoMin)
    {
      _autoLevelPeakMaxFalloffRate = calculateFalloff(_autoLevelFalloffType, _autoLevelFalloffRate, _autoLevelPeakMaxFalloffRate);
      _autoLevelPeakMax -= _autoLevelPeakMaxFalloffRate;
    }
    if (_autoLevelVuPeakMax > _autoMin)
    {
      _autoLevelMaxFalloffRate = calculateFalloff(_autoLevelFalloffType, _autoLevelFalloffRate, _autoLevelMaxFalloffRate);
      _autoLevelVuPeakMax -= _autoLevelMaxFalloffRate;
    }
    if (_autoLevelPeakMax < _autoMin)
    {
      _autoLevelPeakMax = _autoMin;
    }
    if (_autoLevelVuPeakMax < _autoMin)
    {
      _autoLevelVuPeakMax = _autoMin;
    }
  }
  _vu = 0;
  _bandMax = 0;
  _bandAvg = 0;
  _peakAvg = 0;
  _bandMaxIndex = -1;
  _bandMinIndex = -1;
  _peakMaxIndex = -1;
  _peakMinIndex = -1;
  int offset = 2; // first two values are noise
  for (int i = 0; i < _bandSize; i++)
  {
    _bands[i] = 0;
    // handle band peaks fall off
    _peakFallRate[i] = calculateFalloff(_bandPeakFalloffType, _bandPeakFalloffRate, _peakFallRate[i]);
    if (_peaks[i] - _peakFallRate[i] <= _bands[i])
    {
      _peaks[i] = _bands[i];
    }
    else
    {
      _peaks[i] -= _peakFallRate[i]; // fall off rate
    }
    for (int j = 0; j < ceil(_frequencyOffsets[i]); j++)
    {
      // scale down factor to prevent overflow
      float rv = (_real[offset + j] / (float)(0xFFFF * 0xFF));
      float iv = (_imag[offset + j] / (float)(0xFFFF * 0xFF));
      // some smoothing with imaginary numbers.
      rv = sqrt(rv * rv + iv * iv);
      // apply eq scaling
      rv = rv * _bandEq[i];
      if (_frequencyOffsets[i] < 1)
      {
        rv *= _frequencyOffsets[i]; // band scale down factor
      }
      rv = rv < _noiseFloor ? 0 : rv;
      // combine band amplitudes for current band segment
      _bands[i] += rv;
      _vu += rv;
    }
    offset += ceil(_frequencyOffsets[i]);

    // remove noise
    if (_bands[i] < _noiseFloor)
    {
      _bands[i] = 0;
    }

    if (_bands[i] > _peaks[i])
    {
      _peakFallRate[i] = 0;
      _peaks[i] = _bands[i];
    }

    // handle min/max band
    if (_bands[i] > _bandMax && _bands[i] > _noiseFloor)
    {
      _bandMax = _bands[i];
      _bandMaxIndex = i;
    }
    if (_bands[i] < _bandMin)
    {
      _bandMin = _bands[i];
      _bandMinIndex = i;
    }
    // handle min/max peak
    if (_peaks[i] > _autoLevelPeakMax)
    {
      // TODO: make _autoLevelPeakMax value build up new high over time 
      _autoLevelPeakMax = _peaks[i];
      if (_isAutoLevel && _autoMax != -1 && _peaks[i] > _autoMax)
      {
        _isClipping = true;
        _autoLevelPeakMax = _autoMax;
      }
      _peakMaxIndex = i;
      _autoLevelPeakMaxFalloffRate = 0;
    }
    if (_peaks[i] < _peakMin && _peaks[i] > _noiseFloor)
    {
      _peakMin = _peaks[i];
      _peakMinIndex = i;
    }

    // handle band average
    _bandAvg += _bands[i];
    _peakAvg += _peaks[i];
  } // end bands
  // handle band average
  _bandAvg = _bandAvg / _bandSize;
  _peakAvg = _peakAvg / _bandSize;

  // handle vu peak fall off
  _vu = _vu / 8.0; // get it closer to the band peak values
  _vuPeakFallRate = calculateFalloff(_vuPeakFalloffType, _vuPeakFalloffRate, _vuPeakFallRate);
  _vuPeak -= _vuPeakFallRate;
  if (_vu > _vuPeak)
  {
    _vuPeakFallRate = 0;
    _vuPeak = _vu;
  }
  if (_vu > _vuMax)
  {
    _vuMax = _vu;
  }
  if (_vu < _vuMin)
  {
    _vuMin = _vu;
  }
  if (_vuPeak > _autoLevelVuPeakMax)
  {
    _autoLevelVuPeakMax = _vuPeak;
    if (_isAutoLevel && _autoMax != -1 && _vuPeak > _autoMax)
    {
      _isClipping = true;
      _autoLevelVuPeakMax = _autoMax;
    }
    _autoLevelMaxFalloffRate = 0;
  }
  if (_vuPeak < _vuPeakMin)
  {
    _vuPeakMin = _vuPeak;
  }
}

float AudioAnalysis::mapAndClip(float x, float in_min, float in_max, float out_min, float out_max)
{
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

void AudioAnalysis::normalize(bool normalize, float min, float max)
{
  _isNormalize = normalize;
  _normalMin = min;
  _normalMax = max;
}
void AudioAnalysis::bandPeakFalloff(falloff_type falloffType, float falloffRate)
{
  _bandPeakFalloffType = falloffType;
  _bandPeakFalloffRate = falloffRate;
}

void AudioAnalysis::vuPeakFalloff(falloff_type falloffType, float falloffRate)
{
  _vuPeakFalloffType = falloffType;
  _vuPeakFalloffRate = falloffRate;
}

void AudioAnalysis::samplesFalloff(falloff_type falloffType, float falloffRate)
{
  _sampleLevelFalloffType = falloffType;
  _sampleLevelFalloffRate = falloffRate;
}

float AudioAnalysis::calculateFalloff(falloff_type falloffType, float falloffRate, float currentRate)
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
  case NO_FALLOFF:
  default:
    return 0;
  }
}

void AudioAnalysis::autoLevel(falloff_type falloffType, float falloffRate, float min, float max)
{
  _isAutoLevel = falloffType != NO_FALLOFF;
  _autoLevelFalloffType = falloffType;
  _autoLevelFalloffRate = falloffRate;
  _autoMin = min;
  _autoMax = max;
}

bool AudioAnalysis::isNormalize()
{
  return _isNormalize;
}

bool AudioAnalysis::isAutoLevel()
{
  return _isAutoLevel;
}

bool AudioAnalysis::isClipping()
{
  return _isClipping;
}

int AudioAnalysis::getBandSize() {
  return _bandSize;
}

void AudioAnalysis::setBandSize(uint8_t bandSize)
{
  static uint8_t lastBandSize = -1;
  if (bandSize > 0 && bandSize <= BAND_SIZE)
  {
    if (lastBandSize != bandSize)
    { // changed size
      _bandSize = bandSize;
      calculateFrequencyOffsets();
      if (_lowMidHighEq) 
      {
        setEqualizerLevels(_low, _mid, _high); // set the equlizer offsets
      }
    }
  }
  else {
    _bandSize = BAND_SIZE;
  }
  lastBandSize = _bandSize;
}

float *AudioAnalysis::getBands()
{
  if (_isNormalize)
  {
    for (int i = 0; i < _bandSize; i++)
    {
      _bandsNorms[i] = mapAndClip(_bands[i], 0.0f, _autoLevelPeakMax, _normalMin, _normalMax);
    }
    return _bandsNorms;
  }
  return _bands;
}

uint16_t *AudioAnalysis::getBandNames()
{
  return _frequencyNames;
}

uint16_t AudioAnalysis::getBandName(uint8_t index)
{
  if (index >= _bandSize || index < 0)
  {
    return 0;
  }
  return _frequencyNames[index];
}

float AudioAnalysis::getBand(uint8_t index)
{
  if (index >= _bandSize || index < 0)
  {
    return 0;
  }
  if (_isNormalize)
  {
    return mapAndClip(_bands[index], 0.0f, _autoLevelPeakMax, _normalMin, _normalMax);
  }
  return _bands[index];
}

float AudioAnalysis::getBandAvg()
{
  if (_isNormalize)
  {
    return mapAndClip(_bandAvg, 0.0f, _autoLevelPeakMax, _normalMin, _normalMax);
  }
  return _bandAvg;
}

float AudioAnalysis::getBandMax()
{
  return getBand(getBandMaxIndex());
}

int AudioAnalysis::getBandMaxIndex()
{
  return _bandMaxIndex;
}

int AudioAnalysis::getBandMinIndex()
{
  return _bandMinIndex;
}

float *AudioAnalysis::getPeaks()
{
  if (_isNormalize)
  {
    for (int i = 0; i < _bandSize; i++)
    {
      _peaksNorms[i] = mapAndClip(_peaks[i], 0.0f, _autoLevelPeakMax, _normalMin, _normalMax);
    }
    return _peaksNorms;
  }
  return _peaks;
}

float AudioAnalysis::getPeak(uint8_t index)
{
  if (index >= _bandSize || index < 0)
  {
    return 0;
  }
  if (_isNormalize)
  {
    return mapAndClip(_peaks[index], 0.0f, _autoLevelPeakMax, _normalMin, _normalMax);
  }
  return _peaks[index];
}

float AudioAnalysis::getPeakAvg()
{
  if (_isNormalize)
  {
    return mapAndClip(_peakAvg, 0.0f, _autoLevelPeakMax, _normalMin, _normalMax);
  }
  return _peakAvg;
}

float AudioAnalysis::getPeakMax()
{
  return getPeak(getPeakMaxIndex());
}

int AudioAnalysis::getPeakMaxIndex()
{
  return _peakMaxIndex;
}

int AudioAnalysis::getPeakMinIndex()
{
  return _peakMinIndex;
}

float AudioAnalysis::getBass()
{
  uint16_t *widths = getBassMidTrebleWidths();
  int start = 0;
  int range = widths[0];
  float *bands = getBands();
  float out = bands[start];
  for (int i = start; i < start + range; i++)
  {
    if (out < bands[i])
    {
      out = bands[i];
    }
  }
  return out;
}

float AudioAnalysis::getMid()
{
  uint16_t *widths = getBassMidTrebleWidths();
  int start = widths[0];
  int range = widths[1];
  float *bands = getBands();
  float out = bands[start];
  for (int i = start; i < start + range; i++)
  {
    if (out < bands[i])
    {
      out = bands[i];
    }
  }
  return out;
}

float AudioAnalysis::getTreble()
{
  uint16_t *widths = getBassMidTrebleWidths();
  int start = widths[0] + widths[1];
  if (start >= _bandSize)
  {
    return getMid();
  }
  int range = widths[2];
  float *bands = getBands();
  float out = bands[start];
  for (int i = start; i < start + range; i++)
  {
    if (out < bands[i])
    {
      out = bands[i];
    }
  }
  return out;
}

float AudioAnalysis::getBassPeak()
{
  uint16_t *widths = getBassMidTrebleWidths();
  int start = 0;
  int range = widths[0];
  float *bands = getPeaks();
  float out = bands[start];
  for (int i = start; i < start + range; i++)
  {
    if (out < bands[i])
    {
      out = bands[i];
    }
  }
  return out;
}

float AudioAnalysis::getMidPeak()
{
  uint16_t *widths = getBassMidTrebleWidths();
  int start = widths[0];
  int range = widths[1];
  float *bands = getPeaks();
  float out = bands[start];
  for (int i = start; i < start + range; i++)
  {
    if (out < bands[i])
    {
      out = bands[i];
    }
  }
  return out;
}

float AudioAnalysis::getTreblePeak()
{
  uint16_t *widths = getBassMidTrebleWidths();
  int start = widths[0] + widths[1];
  if(start >= _bandSize) {
    return getMidPeak();
  }
  int range = widths[2];
  float *bands = getPeaks();
  float out = bands[start];
  for (int i = start; i < start + range; i++)
  {
    if (out < bands[i])
    {
      out = bands[i];
    }
  }
  return out;
}

float AudioAnalysis::getVolumeUnit()
{
  if (_isNormalize)
  {
    return mapAndClip(_vu, 0.0f, _autoLevelVuPeakMax, _normalMin, _normalMax);
  }
  return _vu;
}

float AudioAnalysis::getVolumeUnitPeak()
{
  if (_isNormalize)
  {
    return mapAndClip(_vuPeak, 0.0f, _autoLevelVuPeakMax, _normalMin, _normalMax);
  }
  return _vuPeak;
}

float AudioAnalysis::getVolumeUnitMax()
{
  if (_isNormalize)
  {
    return mapAndClip(_vuMax, 0.0f, _autoLevelVuPeakMax, _normalMin, _normalMax);
  }
  return _vuMax;
}

float AudioAnalysis::getVolumeUnitPeakMax()
{
  if (_isNormalize)
  {
    return _normalMax;
  }
  return _autoLevelVuPeakMax;
}

float AudioAnalysis::getSample(uint16_t index)
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

  if (_isNormalize)
  {
    float _tempSamplesMax = _samplesMax <= _autoMin * (float)0xFFFF ? _autoMin * 0xFFFF : _samplesMax;
    return mapAndClip(value, -_tempSamplesMax, _tempSamplesMax, _normalMin, _normalMax);
  }
  return value;
}

uint16_t AudioAnalysis::getSampleTriggerIndex()
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

float AudioAnalysis::getSampleMin()
{
  if (_isNormalize)
  {
    return _normalMin;
  }
  return _samplesMin;
}

float AudioAnalysis::getSampleMax()
{
  if (_isNormalize)
  {
    return _normalMax;
  }
  return _samplesMax;
}

#endif // AudioAnalysis_H