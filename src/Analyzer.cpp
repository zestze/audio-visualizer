//
// Created by zeke on 9/30/18.
//

#include <array>
#include "Analyzer.h"
#include <algorithm>
#include "utils.h"
#include <range/v3/all.hpp>

Analyzer::Analyzer(Config config, double samplingRate) :
    _FFT_SIZE(config.fftSize),
    _SAMPLE_RATE(samplingRate),
    _EWMA_ALPHA(config.ewmaAlpha),
    _USE_SIMPLE_SCALE(config.useSimpleScale)
    {

    _in = static_cast<double *>(fftw_malloc(sizeof(double) * _FFT_SIZE));
    _out = static_cast<fftw_complex *>(fftw_malloc(sizeof(fftw_complex) * _FFT_SIZE));

    setupFftPlan();

    // set up containers that can be pre-calculated
    calcWindowVals();
    _freqBinList = utils::generateFrequencyAxis(samplingRate);
    _extrema.setFrequencyBin(_freqBinList);
    _extrema.setFrequencyFactor(_SAMPLE_RATE / _FFT_SIZE);
}

void Analyzer::setupFftPlan() {
    const std::string wisdomFileName {"wisdom.config"};
    const int LOADED =
        fftw_import_wisdom_from_filename(wisdomFileName.c_str());

    _plan = fftw_plan_dft_r2c_1d(static_cast<int>(_FFT_SIZE),
        _in, _out, FFTW_EXHAUSTIVE);

    if (!LOADED) {
        fftw_export_wisdom_to_filename(wisdomFileName.c_str());
    }
}

void Analyzer::calcWindowVals() {
    auto windowFunc = utils::windowHanning;
    _windowVals = std::vector<double> (_FFT_SIZE, 0);
    for (size_t i = 0; i < _FFT_SIZE; i++) {
        _windowVals[i] = windowFunc(i, _FFT_SIZE);
    }
}

//@TODO: have two versions of everything... for double channel and mono channel
std::vector<double> Analyzer::transform(const std::vector<Aquila::SampleType>& sampleBufferLeft,
    const std::vector<Aquila::SampleType>& sampleBufferRight) {

    // apply Fast Fourier Transform
    /*
    //@TODO: perform these in separate threads?
    //@TODO: could 'this' context be troublesome...?
    //@TODO: for these two to work, in / out need to be managed separately...

     //@TODO: to work... can modify applyFFT to take 'in' and 'out' as args...
     //@TODO: and also need to take 'plan' separately...
     //@TODO: just kind of need to have multiple definitions for this case
    auto promiseLeft = std::async(std::launch::async,
        &Analyzer::applyFft, this,
        sampleBufferLeft);
    auto promiseRight = std::async(std::launch::async,
        &Analyzer::applyFft, this,
        sampleBufferRight);

    std::vector<double> power = promiseLeft.get();
    std::vector<double> powerRight = promiseRight.get();
     */

    std::vector<double> power = applyFft(sampleBufferLeft);
    std::vector<double> powerRight = applyFft(sampleBufferRight);

    //@TODO: perform these in separate threads?
    // combine the two channels
    utils::parallelTransform<std::vector<double>>(power, powerRight);

    // scale to be a percentage of the absolute peak
    if (_USE_SIMPLE_SCALE) {
        _extrema.simpleScale(power);
    } else {
        _extrema.complexScale(power);
    }

    auto spectrum = spectrumize(power);

    // commenting this out will help bring the 'voice' out
    utils::scaleLog(spectrum);

    // commenting this out will make the display feel 'jittery'
    applyEwma(spectrum);

    return spectrum;
}

void Analyzer::updateExtrema(const std::vector<Aquila::SampleType>& sampleBufferLeft,
    const std::vector<Aquila::SampleType>& sampleBufferRight) {

    //@TODO: run these in separate threads?
    // apply Fast Fourier Transform
    std::vector<double> power = applyFft(sampleBufferLeft);

    std::vector<double> powerRight = applyFft(sampleBufferRight);

    // combine the two channels
    utils::parallelTransform<std::vector<double>>(power, powerRight);

    // check for extrema
    _extrema.update(power);

}

std::vector<double> Analyzer::applyFft(const std::vector<Aquila::SampleType>& sampleBuffer) {
    // setup
    const size_t N = sampleBuffer.size();

    // copy dynamic vals
    for (size_t i = 0; i < N; i++) {
        _in[i]  = sampleBuffer[i];
        _in[i] *= _windowVals[i]; // apply windowing function
    }

    // execute
    fftw_execute(_plan);

    // only half of the values mean anything.
    // get values and pass them back.
    // don't care about complex values.
    using namespace ranges;
    int n = N / 2; // to play nice with view::ints
    auto toPower = [&] (int i) -> double { return std::abs(_out[i][0]); };

    return view::ints(0, n) | view::transform(toPower);
}

void Analyzer::applyEwma(std::vector<double> &currBuffer) {

    // check if first iteration
    if (_prevVals.size() == 0) {
        _prevVals = currBuffer;
        return;
    }

    for (int i = 0; i < currBuffer.size() && i < _prevVals.size(); i++) {
        // apply exponential weighted moving average
        currBuffer[i] = _EWMA_ALPHA * _prevVals[i] + (1 - _EWMA_ALPHA) * currBuffer[i];
        // update prevVals for next time this function is called
        _prevVals[i] = currBuffer[i];
    }
}

std::vector<double> Analyzer::spectrumize(const std::vector<double> magnitudeList) {

    std::vector<double> peakMagnitudes (_freqBinList.size() - 1, 0);

    for (int i = 0; i < magnitudeList.size(); i++) {
        const double MAGNITUDE = magnitudeList[i];
        const double FREQ = i * _SAMPLE_RATE / _FFT_SIZE;

        const int BIN_INDEX = utils::findBin(FREQ, _freqBinList);
        if (BIN_INDEX != -1 && MAGNITUDE > peakMagnitudes[BIN_INDEX]) {
            peakMagnitudes[BIN_INDEX] = MAGNITUDE;
        }
    }

    return peakMagnitudes;
}

