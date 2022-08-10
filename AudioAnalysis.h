#ifndef AudioAnalysis_H
#define AudioAnalysis_H

#include "Arduino.h"

// arduinoFFT V2
// See the develop branch on GitHub for the latest info and speedups.
// https://github.com/kosme/arduinoFFT/tree/develop
// if you are going for speed over percision uncomment the lines below.
//#define FFT_SPEED_OVER_PRECISION
//#define FFT_SQRT_APPROXIMATION
#include <arduinoFFT.h>

class AudioAnalysis
{
    public:
        AudioAnalysis();
        /* FFT Functions */
        void computeFFT(int32_t *samples, int sample_size, int sample_rate); // calculates FFT on sample data
        float *getReal(); // gets the Real values after FFT calculation
        float *getImaginary(); // gets the imaginary values after FFT calculation

        /* Band Frequency Functions */
        void setNoiseFloor(float noiseFloor); // threshold before sounds are registered
        void computeFrequencies(uint8_t band_size = BAND_SIZE); // converts FFT data into frequency bands
        void normalize(bool normalize = true, float min = 0, float max = 1); // normalize all values and constrain to min/max.
        void autoLevel(bool autoLevel, float min = 600, float max = 10000); // auto ballance normalized values to ambient noise levels.

        float *getBands(); // gets the last bands calculated from processFrequencies()
        float *getPeaks(); // gets the last peaks calculated from processFrequencies()

        float getBand(uint8_t index); // gets the value at bands index
        float getBandAvg();    // average value across all bands
        int getBandMaxIndex(); // index of the highest value band
        int getBandMinIndex(); // index of the lowest value band

        float getPeak(uint8_t index); // gets the value at peaks index
        float getPeakAvg();    // average value across all peaks
        int getPeakIndexMax(); // index of the highest value peak
        int getPeakIndexMin(); // index of the lowest value peak

        /* Volume Unit Functions */
        float getVolumeUnit(); // gets the last volume unit calculated from processFrequencies()
        float getVolumeUnitPeak(); // gets the last volume unit peak calculated from processFrequencies()
        float getVolumeUnitMax(); // value of the highest value volume unit
        float getVolumeUnitMin(); // value of the lowest value volume unit
        float getVolumeUnitPeakMax(); // value of the highest value volume unit
        float getVolumeUnitPeakMin(); // value of the lowest value volume unit

    protected:
        int32_t *_samples;
        int _sample_size;
        int _sample_rate;
        bool _isAutoLevel = false;
        float _autoMin = 600;
        float _autoMax = 10000;

        bool _isNormalize = false;
        float _normalMin = 0;
        float _normalMax = 1;

        /* FFT Variables */
        float _real[SAMPLE_SIZE];
        float _imag[SAMPLE_SIZE];
        float _weighingFactors[SAMPLE_SIZE];

        /* Band Frequency Variables */
        float _noiseFloor = NOISE_THRESHOLD;
        int _band_size = BAND_SIZE;
        float _bands[BAND_SIZE];
        float _peaks[BAND_SIZE];
        float _peakFallRate[BAND_SIZE];
        float _peaksNorms[BAND_SIZE];
        float _bandsNorms[BAND_SIZE];

        float _bandAvg;
        float _peakAvg;
        int8_t _bandMinIndex;
        int8_t _bandMaxIndex;
        int8_t _peakMinIndex;
        int8_t _peakMaxIndex;
        float _bandMin;
        float _bandMax; // used for normalization calculation
        float _peakMin;
        float _peakMax; // used for normalization calculation
        float _peakMinFalloffRate;
        float _peakMaxFalloffRate;

        /* Volume Unit Variables */
        float _vu;
        float _vuPeak;
        float _vuPeakFallRate;
        float _vuMin;
        float _vuMax; // used for normalization calculation
        float _vuPeakMin;
        float _vuPeakMax; // used for normalization calculation
        float _vuPeakMinFalloffRate;
        float _vuPeakMaxFalloffRate;
        ArduinoFFT<float> *_FFT = nullptr;
};

AudioAnalysis::AudioAnalysis()
{
}

void AudioAnalysis::computeFFT(int32_t *samples, int sample_size, int sample_rate)
{
    _samples = samples;
    if (_FFT == nullptr || _sample_size != sample_size || _sample_rate != sample_rate)
    {
        _sample_size = sample_size;
        _sample_rate = sample_rate;
        _FFT = new ArduinoFFT<float>(_real, _imag, _sample_size, _sample_rate, _weighingFactors);
    }

    // prep samples for analysis
    for (int i = 0; i < _sample_size; i++)
    {
        _real[i] = samples[i];
        _imag[i] = 0;
    }

    // process the audio into FFT for later processes.
    // _FFT.DCRemoval();
    // _FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    // _FFT.Compute(FFT_FORWARD);
    // _FFT.ComplexToMagnitude();

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

void AudioAnalysis::computeFrequencies(uint8_t band_size)
{
    // band offsets helpers based on 1024 samples
    const static uint16_t _2frequencyOffsets[2] = {24, 359};
    const static uint16_t _4frequencyOffsets[4] = {6, 18, 72, 287};
    const static uint16_t _8frequencyOffsets[8] = {2, 4, 6, 12, 25, 47, 92, 195};
    const static uint16_t _16frequencyOffsets[16] = {1, 1, 2, 2, 2, 4, 5, 7, 11, 14, 19, 28, 38, 54, 75, 120};
    const static uint16_t _32frequencyOffsets[32] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 5, 5, 7, 7, 8, 8, 14, 14, 19, 19, 27, 27, 37, 37, 60, 60};
    const uint16_t *_frequencyOffsets;
    try_frequency_offsets_again:
    switch (band_size)
    {
        case 2:_frequencyOffsets = _2frequencyOffsets; break;
        case 4:_frequencyOffsets = _4frequencyOffsets; break;
        case 8:_frequencyOffsets = _8frequencyOffsets; break;
        case 16:_frequencyOffsets = _16frequencyOffsets; break;
        case 32:_frequencyOffsets = _32frequencyOffsets; break;
        default:
            band_size = BAND_SIZE;
            goto try_frequency_offsets_again;
    }
    _band_size = band_size;

     // for normalize falloff rates
    if (_isAutoLevel)
    {
        //_peakMin += _peakMinFalloffRate;
        if (_peakMax > _autoMin)
        {
            _peakMax -= _peakMaxFalloffRate;
        }
        //_vuPeakMin += _vuPeakMinFalloffRate;
        if (_vuPeakMax > _autoMin * 1.5)
        {
            _vuPeakMax -= _vuPeakMaxFalloffRate;
        }
    }
    int offset = 2; // first two values are noise
    _vu = 0;
    for (int i = 0; i < _band_size; i++)
    {
        _bands[i] = 0;
        for (int j = 0; j < _frequencyOffsets[i]; j++)
        {
            // scale down factor to prevent overflow
            int rv = (_real[offset + j] / (0xFFFF * 0xFF));
            int iv = (_imag[offset + j] / (0xFFFF * 0xFF));
            // some smoothing with imaginary numbers.
            rv = sqrt(rv * rv + iv * iv);
            // combine band amplitudes for current band segment
            _bands[i] += rv;
            _vu += rv;
            // _bands[i] = rv > _bands[i] ? rv : _bands[i];
        }
        offset += _frequencyOffsets[i];

        // remove noise
        if (_bands[i] < _noiseFloor)
        {
            _bands[i] = 0;
        }
        // handle band peaks fall off
        _peakFallRate[i] += _peakFallRate[i];
        _peaks[i] -= _peakFallRate[i]; // fall off rate
        if (_bands[i] > _peaks[i])
        {
            _peakFallRate[i] = PEAK_FALLOFF_RATE;
            _peaks[i] = _bands[i];
        }

        // handle min/max band
        if(_bands[i] > _bandMax) {
            _bandMax = _bands[i];
            _bandMaxIndex = i;
        }
        if (_bands[i] < _bandMin)
        {
            _bandMin = _bands[i];
            _bandMinIndex = i;
        }
        // handle min/max peak
        if(_peaks[i] > _peakMax) {
            _peakMax = _peaks[i];
            _peakMaxIndex = i;
            _peakMaxFalloffRate = _peakMax * .01;
        }
        if (_peaks[i] < _peakMin)
        {
            _peakMin = _peaks[i];
            _peakMinIndex = i;
            //_peakMinFalloffRate = PEAK_FALLOFF_RATE;
        }

        // handle band average
        _bandAvg += _bands[i];
        _peakAvg += _peaks[i];
    } // end bands
    // handle band average
    _bandAvg = _bandAvg / _band_size;
    _peakAvg = _peakAvg / _band_size;


    // handle vu peak fall off
    _vuPeakFallRate += _vuPeakFallRate;
    _vuPeak -= _vuPeakFallRate;
    if (_vu > _vuPeak)
    {
        _vuPeakFallRate = PEAK_FALLOFF_RATE;
        _vuPeak = _vu;
    }
    if(_vu > _vuMax) {
        _vuMax = _vu;
    }
    if (_vu < _vuMin)
    {
        _vuMin = _vu;
    }
    if (_vuPeak > _vuPeakMax)
    {
        _vuPeakMax = _vuPeak;
        _vuPeakMaxFalloffRate = _vuPeak * .01;
    }
    if (_vuPeak < _vuPeakMin)
    {
        //_vuPeakMinFalloffRate = PEAK_FALLOFF_RATE;
        _vuPeakMin = _vuPeak;
    }

}

template <class X>
X map_Generic(X x, X in_min, X in_max, X out_min, X out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void AudioAnalysis::normalize(bool normalize, float min, float max)
{
    _isNormalize = normalize;
    _normalMin = min;
    _normalMax = max;
}

void AudioAnalysis::autoLevel(bool autoLevel, float min, float max)
{
    _isAutoLevel = autoLevel;
    _autoMin = min;
    _autoMax = max;
}

float *AudioAnalysis::getBands()
{
    if (_isNormalize)
    {
        for (int i = 0; i < _band_size; i++)
        {
            _bandsNorms[i] = ::map_Generic(_bands[i], 0.0f, _peakMax, _normalMin, _normalMax);
        }
        return _bandsNorms;
    }
    return _bands;
}

float AudioAnalysis::getBand(uint8_t index)
{
    if(index >= _band_size) {
        return 0;
    }
    if (_isNormalize) {
        return ::map_Generic(_bands[index], 0.0f, _peakMax, _normalMin, _normalMax);
    }
    return _bands[index];
}

float AudioAnalysis::getBandAvg()
{
    if (_isNormalize)
    {
        return ::map_Generic(_bandAvg, 0.0f, _peakMax, _normalMin, _normalMax);
    }
    return _bandAvg;
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
        for (int i = 0; i < _band_size; i++)
        {
            _peaksNorms[i] = ::map_Generic(_peaks[i], 0.0f, _peakMax, _normalMin, _normalMax);
        }
        return _peaksNorms;
    }
    return _peaks;
}

float AudioAnalysis::getPeak(uint8_t index)
{
    if (index >= _band_size)
    {
        return 0;
    }
    if (_isNormalize)
    {
        return ::map_Generic(_peaks[index], 0.0f, _peakMax, _normalMin, _normalMax);
    }
    return _peaks[index];
}

float AudioAnalysis::getPeakAvg()
{
    if (_isNormalize)
    {
        return ::map_Generic(_peakAvg, 0.0f, _peakMax, _normalMin, _normalMax);
    }
    return _peakAvg;
}

int AudioAnalysis::getPeakIndexMax()
{
    return _peakMaxIndex;
}

int AudioAnalysis::getPeakIndexMin()
{
    return _peakMinIndex;
}


float AudioAnalysis::getVolumeUnit()
{
    if (_isNormalize)
    {
        return ::map_Generic(_vu, 0.0f, _vuPeakMax, _normalMin, _normalMax);
    }
    return _vu;
}

float AudioAnalysis::getVolumeUnitPeak()
{
    if (_isNormalize)
    {
        return ::map_Generic(_vuPeak, 0.0f, _vuPeakMax, _normalMin, _normalMax);
    }
    return _vuPeak;
}

float AudioAnalysis::getVolumeUnitMax() {
    if (_isNormalize)
    {
        return ::map_Generic(_vuMax, 0.0f, _vuPeakMax, _normalMin, _normalMax);
    }
    return _vuMax;
}

float AudioAnalysis::getVolumeUnitMin() {
    if (_isNormalize)
    {
        return ::map_Generic(_vuMin, 0.0f, _vuPeakMax, _normalMin, _normalMax);
    }
    return _vuMin;
}

float AudioAnalysis::getVolumeUnitPeakMax() {
    if (_isNormalize)
    {
        return _normalMax;
    }
    return _vuPeakMax;
}

float AudioAnalysis::getVolumeUnitPeakMin() {
    if (_isNormalize)
    {
        return ::map_Generic(_vuPeakMin, 0.0f, _vuPeakMax, _normalMin, _normalMax);
    }
    return _vuPeakMin;
}

#endif // AudioAnalysis_H