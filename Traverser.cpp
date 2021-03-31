//
// Created by Max on 31/03/2021.
//

#include "Traverser.h"

Traverser::Traverser(DBConnector& connector)
    : dbConnector(connector) {
    formatManager.registerBasicFormats();
}

AudioBuffer<float> Traverser::initialiseRandomTrajectory(int lengthInGrains) {
    // Clear source and target vectors
    source.clear();
    target.clear();

    // Get min and max values from database to create random values
    float minLoudness = dbConnector.queryMin("LOUDNESS");
    float maxLoudness = dbConnector.queryMax("LOUDNESS");
    float minSC = dbConnector.queryMin("SPECTRAL_CENTROID");
    float maxSC = dbConnector.queryMax("SPECTRAL_CENTROID");

    for(int i = 0; i < lengthInGrains; i++){
        // Generate random values in range for each field
        float rLoudness = minLoudness + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(maxLoudness-minLoudness)));
        float rSC = minSC + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(maxSC-minSC)));

        Grain* grain = new Grain(rLoudness, rSC);
        source.emplace_back(*grain);
    }

    // Find target grains from database
    for(Grain& sourceGrain : source){
        float margin = 100;
        Grain grain = dbConnector.queryClosestGrain(sourceGrain, margin);

        target.emplace_back(grain);
    }

    // Get sound data
    int channels = 2;
    int lengthInSamples = lengthInGrains * GRAIN_LENGTH;
    AudioBuffer<float>* buffer = new AudioBuffer<float>(channels, lengthInSamples);
    int bufferIdx = 0;

    map<string, AudioBuffer<float>*> sourceData;
    for(Grain& grain : target){
        // Check if grain is valid
        if(grain.getPath() != ""){
            // Load audio file if not yet in memory
            ScopedPointer<AudioFormatReader> reader = formatManager.createReaderFor(File(grain.getPath()));
            reader->read(buffer, bufferIdx, GRAIN_LENGTH, grain.getIdx(), true, true);
            bufferIdx += GRAIN_LENGTH;
        }
    }

    return *buffer;
}
