//
// Created by Max on 31/03/2021.
//

#include "Traverser.h"

Traverser::Traverser(DBConnector& connector, Analyser& analyser)
    : dbConnector(connector), analyser(analyser){
    formatManager.registerBasicFormats();

    calculateFeatureStatistics();
}

AudioBuffer<float> Traverser::generateTrajectoryFromParams(vector<float> params){
    if(!isMinMaxInitialised()){
        fprintf(stderr, "Min-max not initialised");
        AudioBuffer<float>* buffer = new AudioBuffer<float>(2, 512);
        return *buffer;
    }
    // Clear source and target vectors
    source.clear();
    target.clear();

    // Define length of trajectory
    // TODO change when adding new audio features
    int lengthInGrains = (int)(params.size() / NUM_FEATURES);

    // Create new grains from incoming parameters
    for(int i = 0; i < lengthInGrains; i += NUM_FEATURES){
        float loudness = mapFloat(params.at(i), 0.0f, 1.0f, minLoudness, maxLoudness);
        float sc = mapFloat(params.at(i + 1), 0.0f, 1.0f, minSC, maxSC);
        float sf = mapFloat(params.at(i + 2), 0.0f, 1.0f, minSF, maxSF);; // Spectral flux

        Grain* grain = new Grain(loudness, sc, sf);
        source.emplace_back(*grain);
    }

    // Find target grains from database
    for(Grain& sourceGrain : source){
        float margin = 100.0f;
        vector<Grain> found = dbConnector.queryClosestGrain(sourceGrain, margin);
        Grain bestMatch;

        if(!found.empty()){
            // Calculate best grain based on weighted euclidean distance
            bestMatch = findBestGrain(sourceGrain, found);
        }

        target.emplace_back(bestMatch);
    }

    // Get sound data
    int channels = 2;
    int lengthInSamples = lengthInGrains * GRAIN_LENGTH;
    AudioBuffer<float>* buffer = new AudioBuffer<float>(channels, lengthInSamples);
    int bufferIdx = 0;

    for(Grain& grain : target){
        // Check if grain is valid
        if(!grain.getPath().empty()){
            // Load audio file if not yet in memory
            ScopedPointer<AudioFormatReader> reader = formatManager.createReaderFor(File(grain.getPath()));
            reader->read(buffer, bufferIdx, GRAIN_LENGTH, grain.getIdx(), true, true);
            bufferIdx += GRAIN_LENGTH;
        }
    }

    return *buffer;
}

AudioBuffer<float> Traverser::initialiseRandomTrajectory(int lengthInGrains) {
    if(!isMinMaxInitialised()){
        fprintf(stderr, "Min-max not initialised");
        AudioBuffer<float>* buffer = new AudioBuffer<float>(2, 512);
        return *buffer;
    }
    // Clear source and target vectors
    source.clear();
    target.clear();

    for(int i = 0; i < lengthInGrains; i++){
        // Generate random values in range for each field
        float rLoudness = minLoudness + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(maxLoudness-minLoudness)));
        float rSC = minSC + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(maxSC-minSC)));
        float rSF = minSF + static_cast <float> (rand()) / ( static_cast <float> (RAND_MAX / (maxSF - minSF)));

        Grain* grain = new Grain(rLoudness, rSC, rSF);
        source.emplace_back(*grain);
    }

    // Find target grains from database
    float margin = 100;
    for(Grain& sourceGrain : source){
        vector<Grain> found = dbConnector.queryClosestGrain(sourceGrain, margin);
        Grain bestMatch;

        if(!found.empty()){
            // Calculate best grain based on weighted euclidean distance
            bestMatch = findBestGrain(sourceGrain, found);
        }

        target.emplace_back(bestMatch);
    }

    // Get sound data
    int channels = 2;
    int lengthInSamples = lengthInGrains * GRAIN_LENGTH;
    AudioBuffer<float>* buffer = new AudioBuffer<float>(channels, lengthInSamples);
    int bufferIdx = 0;

    map<string, AudioBuffer<float>*> sourceData;
    for(Grain& grain : target){
        // Check if grain is valid
        if(!grain.getPath().empty()){
            // Load audio file if not yet in memory
            ScopedPointer<AudioFormatReader> reader = formatManager.createReaderFor(File(grain.getPath()));
            reader->read(buffer, bufferIdx, GRAIN_LENGTH, grain.getIdx(), true, true);
            bufferIdx += GRAIN_LENGTH;
        }
    }

    return *buffer;
}

void Traverser::calculateFeatureStatistics() {
    // Get min and max values from database
    if (dbConnector.isPopulated()){
        string loudness = "LOUDNESS";
        string sc = "SPECTRAL_CENTROID";
        string sf = "SPECTRAL_FLUX";

        minLoudness = dbConnector.queryMin(loudness);
        maxLoudness = dbConnector.queryMax(loudness);
        meanLoudness = dbConnector.queryMean(loudness);
        stdLoudness = dbConnector.queryStd(loudness);

        minSC = dbConnector.queryMin(sc);
        maxSC = dbConnector.queryMax(sc);
        meanSC = dbConnector.queryMean(sc);
        stdSC = dbConnector.queryStd(sc);

        minSF = dbConnector.queryMin(sf);
        maxSF = dbConnector.queryMax(sf);
        meanSF = dbConnector.queryMean(sf);
        stdSF = dbConnector.queryStd(sf);
    }
}

bool Traverser::isMinMaxInitialised() const {
    return static_cast<int>(maxLoudness) > 0;
}

Grain Traverser::findBestGrain(Grain& src, vector<Grain>& grains) {
    float wLoudness = 0.5f;
    float wSC = 0.4f;
    float wSF = 0.1f;

    Grain bestMatch;
    vector<float> distances;
    for(Grain& candidate : grains){
        float distance = 0.0f;
        distance += (powf((candidate.getLoudness() - src.getLoudness()), 2) / stdLoudness) * wLoudness;
        distance += (powf((candidate.getSpectralCentroid() - src.getSpectralCentroid()), 2) / stdSC) * wSC;
        distance += (powf((candidate.getSpectralFlux() - src.getSpectralFlux()), 2) / stdSF) * wSF;

        distances.emplace_back(distance * 100.0f);
    }
    // Find index of min distance
    int minElementIdx = min_element(distances.begin(), distances.end()) - distances.begin();

    bestMatch = grains.at(minElementIdx);

    return bestMatch;
}

AudioBuffer<float> Traverser::generateTrajectoryFromAudio(AudioBuffer<float>& input) {
    if(!isMinMaxInitialised()){
        fprintf(stderr, "Min-max not initialised");
        AudioBuffer<float>* buffer = new AudioBuffer<float>(2, 512);
        return *buffer;
    }

    // Clear source and target vectors
    source.clear();
    target.clear();

    source = analyser.audioBufferToGrains(input);

    // Find target grains from database
    float margin = 100;
    for(Grain& sourceGrain : source){
        vector<Grain> found = dbConnector.queryClosestGrain(sourceGrain, margin);
        Grain bestMatch;

        if(!found.empty()){
            // Calculate best grain based on weighted euclidean distance
            bestMatch = findBestGrain(sourceGrain, found);
        }

        target.emplace_back(bestMatch);
    }

    // Get sound data
    int channels = 2;
    AudioBuffer<float>* buffer = new AudioBuffer<float>(channels, GRAINS_IN_TRAJECTORY * GRAIN_LENGTH);
    int bufferIdx = 0;

    for(Grain& grain : target){
        // Check if grain is valid
        if(!grain.getPath().empty()){
            // Load audio file if not yet in memory
            ScopedPointer<AudioFormatReader> reader = formatManager.createReaderFor(File(grain.getPath()));
            reader->read(buffer, bufferIdx, GRAIN_LENGTH, grain.getIdx(), true, true);
            bufferIdx += GRAIN_LENGTH;
        }
    }

    return *buffer;
}
