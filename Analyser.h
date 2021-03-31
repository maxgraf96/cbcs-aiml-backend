//
// Created by Max on 30/03/2021.
//

#ifndef DMLAP_BACKEND_ANALYSER_H
#define DMLAP_BACKEND_ANALYSER_H

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "external_libraries/essentia/include/algorithmfactory.h"
#include "Grain.h"
#include "DBConnector.h"
#include "Constants.h"

using namespace juce;
using namespace std;
using namespace essentia;
using namespace essentia::standard;

class Analyser {
public:
    Analyser(DBConnector& connector);
    ~Analyser();
    void initialise(double sr, int samplesPerBlockExpected);

    void analyseFileBuffer(AudioBuffer<float>& buffer, string filename, string path);

private:
    DBConnector& dbConnector;

    void insertGrain();

    double sampleRate = 0.0;
    int samplesPerBlock = -1;

    // Current grain
    unique_ptr<Grain> currentGrain;

    // Values estimated by Essentia are marked with an "e" prefix
    vector<Real> eAudioBuffer;

    // Will contain JUCE audio buffer after windowing
    vector<Real> windowedFrame;
    // Will contain the spectrum data
    vector<Real> eSpectrumData;
    vector<Real> eMelBands;
    Real eSpectralCentroid = 0.0f;
    Real ePitchYIN = 0.0f;
    Real ePitchConfidence = 0.0f;
    Real eLoudness = 0.0f;
    Real eOnsetDetection = 0.0f;
    vector<Real> eSpectralPeaksFrequencies; // in Hz
    vector<Real> eSpectralPeaksMagnitudes;

    // Essentia algorithms are marked by an "a" prefix
    unique_ptr<Algorithm> aWindowing;
    unique_ptr<Algorithm> aSpectrum;
    unique_ptr<Algorithm> aSpectralCentroid;
    unique_ptr<Algorithm> aLoudness;
    unique_ptr<Algorithm> aSpectralPeaks;
    unique_ptr<Algorithm> aMFCC;
};


#endif //DMLAP_BACKEND_ANALYSER_H
