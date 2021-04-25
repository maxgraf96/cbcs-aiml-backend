//
// Created by Max on 31/03/2021.
//

#include "Traverser.h"

Traverser::Traverser(DBConnector& connector, Analyser& analyser, AudioBuffer<float>& generatedBuffer, vector<Grain>& target)
    : dbConnector(connector), analyser(analyser), generatedBuffer(generatedBuffer), target(target){
    formatManager.registerBasicFormats();

    calculateFeatureStatistics();

    // Initialise window
    window = make_unique<dsp::WindowingFunction<float>>(GRAIN_LENGTH, dsp::WindowingFunction<float>::WindowingMethod::hann);
}

void Traverser::generateTrajectoryFromParams(vector<float> params){
    if(!init()){
        return;
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
    generateTargetGrainsAndCreateBuffer();
}

void Traverser::generateTrajectoryFromAudio(AudioBuffer<float>& input) {
    if(!init()){
        return;
    }
    // Create source from input audio
    source = analyser.audioBufferToGrains(input);
    return generateTargetGrainsAndCreateBuffer();
}

void Traverser::generateRandomTrajectory() {
    if(!init()){
        return;
    }
    source = dbConnector.queryRandomTrajectory();
    return generateTargetGrainsAndCreateBuffer();
}

Grain Traverser::findBestGrain(Grain& src, vector<Grain>& grains) const {
    float wLoudness = 1.0f;
    float wSC = 2.0f;
    float wSF = 1.0f;
    float wPitch = 3.0f;

    Grain bestMatch;
    vector<float> distances;
    for(Grain& candidate : grains){
        float distance = 0.0f;
        // Normalise values
        float distLoudness = powf((normaliseValue(candidate.getLoudness(), maxLoudness) - normaliseValue(src.getLoudness(), maxLoudness)), 2) * wLoudness;
        float distSC = powf((normaliseValue(candidate.getSpectralCentroid(), maxSC) - normaliseValue(src.getSpectralCentroid(), maxSC)), 2) * wSC;
        float distSF = powf((normaliseValue(candidate.getSpectralFlux(), maxSF) - normaliseValue(src.getSpectralFlux(), maxSF)), 2) * wSF;
        float distPitch = powf((normaliseValue(candidate.getPitch(), maxPitch) - normaliseValue(src.getPitch(), maxPitch)), 2) * wPitch;

        distance = distLoudness + distSC + distSF + distPitch;
        distances.emplace_back(distance);
    }
    // Find index of min distance
    int minElementIdx = min_element(distances.begin(), distances.end()) - distances.begin();

    bestMatch = grains.at(minElementIdx);

    return bestMatch;
}

void Traverser::generateTargetGrains(float margin){
    target.clear();

    for(Grain& sourceGrain : source){
        vector<Grain> found = dbConnector.queryClosestGrain(sourceGrain, margin);
        Grain bestMatch;

        if(!found.empty()){
            // Calculate best grain based on weighted euclidean distance
            bestMatch = findBestGrain(sourceGrain, found);
        }

        target.emplace_back(bestMatch);
    }
}
void Traverser::generateTargetGrainsAndCreateBuffer(){
    // Find target grains from database
    float margin = 100;

    generateTargetGrains(margin);

    // Get sound data
    int channels = 2;
    int bufferIdx = 0;

    for(Grain& grain : target){
        // Check if grain is valid
        if(!grain.getPath().empty()){
            // Load audio file if not yet in memory
            ScopedPointer<AudioFormatReader> reader = formatManager.createReaderFor(File(grain.getPath()));
            reader->read(&generatedBuffer, bufferIdx, GRAIN_LENGTH, grain.getIdx(), true, true);
            // Apply window
            window->multiplyWithWindowingTable(generatedBuffer.getWritePointer(0, bufferIdx), GRAIN_LENGTH);
            window->multiplyWithWindowingTable(generatedBuffer.getWritePointer(1, bufferIdx), GRAIN_LENGTH);
            bufferIdx += GRAIN_LENGTH;
        }
    }
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
        fprintf(stdout, "Min-max not initialised. AudioBuffer from trajectory will be empty.");
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

float Traverser::normaliseValue(float in, float ref) {
    return in / ref;
}
