//
// Created by Max on 31/03/2021.
//

#include "Traverser.h"

Traverser::Traverser(DBConnector& connector, Analyser& analyser)
    : dbConnector(connector), analyser(analyser){
    formatManager.registerBasicFormats();

    calculateFeatureStatistics();
}

tuple<AudioBuffer<float>, vector<Grain>> Traverser::generateTrajectoryFromParams(vector<float> params){
    if(!init()){
        return make_tuple(AudioBuffer<float>(0, 0), vector<Grain>());
    }
    // Create source from RL parameters
    // Define length of trajectory
    for(int i = 0; i < params.size(); i += NUM_FEATURES){
        float loudness = mapFloat(params[i], 0.0f, 1.0f, minLoudness, maxLoudness);
        float sc = mapFloat(params[i + 1], 0.0f, 1.0f, minSC, maxSC);
        float sf = mapFloat(params[i + 2], 0.0f, 1.0f, minSF, maxSF);; // Spectral flux
        // Limit pitch to 0-10000
        float pitchHi = 4.0f;
        float pitch = mapFloat(params[i + 3], 0.0f, pitchHi, minPitch, maxPitch);

        Grain* grain = new Grain(loudness, sc, sf, pitch);
        source.emplace_back(*grain);
    }

    return generateTargetGrains();
}

tuple<AudioBuffer<float>, vector<Grain>> Traverser::generateTrajectoryFromAudio(AudioBuffer<float>& input) {
    if(!init()){
        return make_tuple(AudioBuffer<float>(0, 0), vector<Grain>());
    }

    // Create source from input audio
    source = analyser.audioBufferToGrains(input);
    return generateTargetGrains();
}

Grain Traverser::findBestGrain(Grain& src, vector<Grain>& grains) const {
    float wLoudness = 1.0f;
    float wSC = 1.5f;
    float wSF = 0.5f;
    float wPitch = 2.5f;

    Grain bestMatch;
    vector<float> distances;
    for(Grain& candidate : grains){
        float distance = 0.0f;
        distance += (powf((candidate.getLoudness() - src.getLoudness()), 2) / stdLoudness) * wLoudness;
        distance += (powf((candidate.getSpectralCentroid() - src.getSpectralCentroid()), 2) / stdSC) * wSC;
        distance += (powf((candidate.getSpectralFlux() - src.getSpectralFlux()), 2) / stdSF) * wSF;
        distance += (powf((candidate.getPitch() - src.getPitch()), 2) / stdPitch) * wPitch;

        distances.emplace_back(distance * 100.0f);
    }
    // Find index of min distance
    int minElementIdx = min_element(distances.begin(), distances.end()) - distances.begin();

    bestMatch = grains.at(minElementIdx);

    return bestMatch;
}

tuple<AudioBuffer<float>, vector<Grain>> Traverser::initialiseRandomTrajectory(int lengthInGrains) {
    if(!init()){
        return make_tuple(AudioBuffer<float>(0, 0), vector<Grain>());
    }

    // Create random source
    for(int i = 0; i < lengthInGrains; i++){
        // Generate random values in range for each field
        float rLoudness = minLoudness + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(maxLoudness-minLoudness)));
        float rSC = minSC + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(maxSC-minSC)));
        float rSF = minSF + static_cast <float> (rand()) / ( static_cast <float> (RAND_MAX / (maxSF - minSF)));
        float rPitch = minPitch + static_cast <float> (rand()) / ( static_cast <float> (RAND_MAX / (maxPitch - minPitch)));

        Grain* grain = new Grain(rLoudness, rSC, rSF, rPitch);
        source.emplace_back(*grain);
    }

    return generateTargetGrains();
}

tuple<AudioBuffer<float>, vector<Grain>> Traverser::generateTargetGrains(){
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

    return make_tuple(*buffer, target);
}

void Traverser::calculateFeatureStatistics() {
    // Get min and max values from database
    if (dbConnector.isPopulated()){
        string loudness = "LOUDNESS";
        string sc = "SPECTRAL_CENTROID";
        string sf = "SPECTRAL_FLUX";
        string pitch = "PITCH";

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

        minPitch = dbConnector.queryMin(pitch);
        maxPitch = dbConnector.queryMax(pitch);
        meanPitch = dbConnector.queryMean(pitch);
        stdPitch = dbConnector.queryStd(pitch);
    }
}

bool Traverser::isMinMaxInitialised() const {
    return static_cast<int>(maxLoudness) > 0;
}

bool Traverser::init() {
    // Clear source and target vectors
    source.clear();
    target.clear();

    if(isMinMaxInitialised()){
        return true;
    } else {
        fprintf(stderr, "Min-max not initialised. AudioBuffer from trajectory will be empty.");
    }

    return false;
}

vector<float> Traverser::convertAudioBufferToRLFormat(AudioBuffer<float> &buffer) {
    // Convert audio into grains
    vector<Grain> grains = analyser.audioBufferToGrains(buffer);

    vector<float> data;
    data.reserve(grains.size() * static_cast<unsigned long>(NUM_FEATURES));

    for(auto& grain : grains){
        // Map each property
        data.emplace_back(mapFloat(grain.getLoudness(), minLoudness, maxLoudness, 0.0f, 1.0f));
        data.emplace_back(mapFloat(grain.getSpectralCentroid(), minSC, maxSC, 0.0f, 1.0f));
        data.emplace_back(mapFloat(grain.getSpectralFlux(), minSF, maxSF, 0.0f, 1.0f));
        data.emplace_back(mapFloat(grain.getPitch(), minPitch, maxPitch, 0.0f, 1.0f));
    }

    return data;
}
