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

/**
 * The Analyser class is responsible for extracting audio features from grains using the "Essentia" C++ software library (https://essentia.upf.edu/index.html).
 * The audio features extracted here are used to calculate distance measures from grains during the synthesis stage.
 */
class Analyser {
public:
    explicit Analyser(DBConnector& connector);
    ~Analyser();
    // Initialise all fields - sets up essentia algorithms
    void initialise(double sr);

    // This method stores grain audio feature data in database -> used when initially reading grains from files into corpus
    void analyseAndSaveToDB(AudioBuffer<float>& buffer, const string& filename, const string& path);
    // This method creates grains in memory -> used when recording audio to prime the agent
    vector<Grain> audioBufferToGrains(AudioBuffer<float>& buffer);

private:
    // Connection to the grain database
    DBConnector& dbConnector;

    // Sample rate of the JUCE application (injected in initialise(...))
    double sampleRate = 0.0;

    // Values used with Essentia are marked with an "e" prefix
    // Raw audio buffer
    vector<Real> eAudioBuffer;
    // Will contain JUCE audio buffer after windowing
    vector<Real> windowedFrame;
    // Will contain the spectrum data (obtained via FFT)
    vector<Real> eSpectrumData;
    // The spectral centroid of the grain
    Real eSpectralCentroid = 0.0f;
    // The loudness of the grain
    Real eLoudness = 0.0f;
    // The spectral flux of the grain
    Real eSpectralFlux = 0.0f;
    // The pitch of the grain in Hz
    Real ePitch = 0.0f;
    // Confidence for an estimated pitch
    Real ePitchConfidence = 0.0f;
    // The root mean square of the audio data of a grain
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

    // Computes audio features for the current audio buffer
    void computeFeatures();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Analyser)
};


#endif //DMLAP_BACKEND_ANALYSER_H
