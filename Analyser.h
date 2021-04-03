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

    // This one stores in database -> used when initially reading grains from files into corpus
    void analyseAndSaveToDB(AudioBuffer<float>& buffer, const string& filename, const string& path);
    // This one just creates grains in memory -> used when recording audio to prime the agent
    vector<Grain> audioBufferToGrains(AudioBuffer<float>& buffer);

private:
    DBConnector& dbConnector;

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
    Real eSpectralCentroid = 0.0f;
    Real eLoudness = 0.0f;
    Real eSpectralFlux = 0.0f;
    Real ePitch = 0.0f;
    Real ePitchConfidence = 0.0f;
    Real eRMS = 0.0f;

    // Essentia algorithms are marked by an "a" prefix
    unique_ptr<Algorithm> aWindowing;
    unique_ptr<Algorithm> aSpectrum;
    unique_ptr<Algorithm> aSpectralCentroid;
    unique_ptr<Algorithm> aLoudness;
    unique_ptr<Algorithm> aMFCC;
    unique_ptr<Algorithm> aSpectralFlux;
    unique_ptr<Algorithm> aPitchYINFFT;
    unique_ptr<Algorithm> aRMS;

    bool computeFeatures();
};


#endif //DMLAP_BACKEND_ANALYSER_H
