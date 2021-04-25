//
// Created by Max on 31/03/2021.
//

#ifndef DMLAP_BACKEND_TRAVERSER_H
#define DMLAP_BACKEND_TRAVERSER_H

#include <cstdio>
#include <vector>
#include <random>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "sqlite3.h"
#include "DBConnector.h"
#include "Grain.h"
#include "Constants.h"
#include "Utility.h"
#include "Analyser.h"

using namespace std;
using namespace juce;

/**
 * The traverser class is responsible for creating trajectories through the grain space. It can use different modalities to create the trajectories.
 */
class Traverser {
public:
    Traverser(DBConnector& connector, Analyser& analyser, AudioBuffer<float>& generatedBuffer, vector<Grain>& target);

    /**
     * Generate a random trajectory through the grain space
     */
    void generateRandomTrajectory();

    /**
     * Generate a trajectory through the grain space using parameters from the python RL agent
     * @param params
     */
    void generateTrajectoryFromParams(vector<float> params);

    /**
     * Generate a trajectory through the grain space using recorded audio data
     * @param buffer
     */
    void generateTrajectoryFromAudio(AudioBuffer<float>& buffer);

    /**
     * Helper method for sending grains to the RL agent: Converts a JUCE audio buffer (raw audio data) to RL parameters
     * @param buffer
     * @return
     */
    vector<float> convertAudioBufferToRLFormat(AudioBuffer<float>& buffer);

    /**
     * Helper method to calculate statistics for all the audio features in the database
     */
    void calculateFeatureStatistics();

    /**
     * Helper method to check whether db statistics are available
     * @return
     */
    bool isMinMaxInitialised() const;


private:
    // DB connection
    DBConnector& dbConnector;

    // The analyser
    Analyser& analyser;

    // Ref to generatedBuffer from MainComponent
    AudioBuffer<float>& generatedBuffer;

    // Source and target grain vectors
    vector<Grain> source;
    vector<Grain>& target;

    // Format manager for dealing with ".wav" files
    AudioFormatManager formatManager;

    /**
     * Initialisation - clears source and target grain vectors
     * @return True if db is intact (statistics available) false otherwise
     */
    bool init();

    /**
     * Find the best single grain for a given input grain.
     * This singles out the best grain from a vector of possible best grains (coming from the db) using a higher-dimensional euclidean distance measure.
     * @param src The input grain
     * @param grains The possible candidates
     * @return The best found candidate
     */
    Grain findBestGrain(Grain& src, vector<Grain>& grains) const;

    /**
     * Helper function to convert a value to [0..1]
     * @param in input value
     * @param ref max reference value
     * @return The normalised value
     */
    static float normaliseValue(float in, float ref);

    /**
     * Goes through the "source" grain vector and finds the best grains using the "margin" for audio features
     * @param margin The margin to apply to audio features
     */
    void generateTargetGrains(float margin);

    /**
     * Does the same as above but also fills the "generatedBuffer" field with the resulting audio data.
     */
    void generateTargetGrainsAndCreateBuffer();

    // Fields for statistics
    // Loudness
    float minLoudness = 0.0f;
    float maxLoudness = 0.0f;
    float meanLoudness = 0.0f;
    float stdLoudness = 0.0f;

    // Spectral centroid
    float minSC = 0.0f;
    float maxSC = 0.0f;
    float meanSC = 0.0f;
    float stdSC = 0.0f;

    // Spectral flux
    float minSF = 0.0f;
    float maxSF = 0.0f;
    float meanSF = 0.0f;
    float stdSF = 0.0f;

    // Pitch
    float minPitch = 0.0f;
    float maxPitch = 0.0f;
    float meanPitch = 0.0f;
    float stdPitch = 0.0f;

    // Window function that's applied to each grain to avoid clicking
    unique_ptr<dsp::WindowingFunction<float>> window;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Traverser)
};


#endif //DMLAP_BACKEND_TRAVERSER_H
