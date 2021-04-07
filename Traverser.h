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

class Traverser {
public:
    Traverser(DBConnector& connector, Analyser& analyser, AudioBuffer<float>& generatedBuffer, vector<Grain>& target);

    void generateRandomTrajectory();
    void generateTrajectoryFromParams(vector<float> params);
    void generateTrajectoryFromAudio(AudioBuffer<float>& buffer);

    // For sending grains to RL agent
    vector<float> convertAudioBufferToRLFormat(AudioBuffer<float>& buffer);

    void calculateFeatureStatistics();
    bool isMinMaxInitialised() const;


private:
    DBConnector& dbConnector;
    Analyser& analyser;
    AudioBuffer<float>& generatedBuffer;
    vector<Grain> source;
    vector<Grain>& target;
    AudioFormatManager formatManager;

    bool init();

    Grain findBestGrain(Grain& src, vector<Grain>& grains) const;
    float normaliseValue(float in, float ref) const;
    void generateTargetGrains(float margin);
    void generateTargetGrainsAndCreateBuffer();

    float minLoudness = 0.0f;
    float maxLoudness = 0.0f;
    float meanLoudness = 0.0f;
    float stdLoudness = 0.0f;

    float minSC = 0.0f;
    float maxSC = 0.0f;
    float meanSC = 0.0f;
    float stdSC = 0.0f;

    float minSF = 0.0f;
    float maxSF = 0.0f;
    float meanSF = 0.0f;
    float stdSF = 0.0f;

    float minPitch = 0.0f;
    float maxPitch = 0.0f;
    float meanPitch = 0.0f;
    float stdPitch = 0.0f;

    // DSP stuff
    unique_ptr<dsp::WindowingFunction<float>> window;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Traverser)
};


#endif //DMLAP_BACKEND_TRAVERSER_H
