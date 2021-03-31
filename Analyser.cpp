//
// Created by Max on 30/03/2021.
//

#include "Analyser.h"

Analyser::Analyser(DBConnector& connector)
: dbConnector(connector){

}

void Analyser::analyseFileBuffer(AudioBuffer<float>& buffer, string filename, string path){
    int index = 0;
    int counter = 1;
    vector<unique_ptr<Grain>> grains;
    while(index + GRAIN_LENGTH < buffer.getNumSamples()){
        // Clear essentia audiobuffer
        eAudioBuffer.clear();

        auto* reader = buffer.getReadPointer(0);
        for (int i = index; i < index + GRAIN_LENGTH; i++){
            eAudioBuffer.push_back(reader[i]);
        }

        // Essentia algorithms compute routines
        aWindowing->compute();
        aSpectrum->compute();
        aSpectralCentroid->compute();
        aLoudness->compute();
//        aSpectralPeaks->compute();

        // Save grain to database
//        if(index == 0)
        currentGrain = make_unique<Grain>(filename + to_string(counter), path, index, eLoudness, eSpectralCentroid);
//        else
//            currentGrain = make_unique<Grain>(filename + to_string(counter), "", index, eLoudness, eSpectralCentroid);

        grains.push_back(make_unique<Grain>(filename + to_string(counter), path, index, eLoudness, eSpectralCentroid));
//        insertGrain();

        // Update index -> start of next grain
        index += GRAIN_LENGTH;
        counter++;
    }
    dbConnector.insertGrains(grains);
}

void Analyser::insertGrain(){
    dbConnector.insertGrain(*currentGrain);
}

void Analyser::initialise(double sr, int samplesPerBlockExpected) {
    this->sampleRate = sr;
    this->samplesPerBlock = samplesPerBlockExpected;

    // Create algorithms
    standard::AlgorithmFactory& factory = standard::AlgorithmFactory::instance();

    aWindowing.reset(factory.create("Windowing", "type", "blackmanharris62"));
    aSpectrum.reset(factory.create("Spectrum"));
    aMFCC.reset(factory.create("MFCC"));
    aSpectralCentroid.reset(factory.create("SpectralCentroidTime", "sampleRate", sr));
    aLoudness.reset(factory.create("Loudness"));
    aSpectralPeaks.reset(factory.create("SpectralPeaks", "sampleRate", sr));

    // Connect algorithms
    aWindowing->input("frame").set(eAudioBuffer);
    aWindowing->output("frame").set(windowedFrame);
    aSpectrum->input("frame").set(windowedFrame);
    aSpectrum->output("spectrum").set(eSpectrumData);

    // Spectral centroid
    aSpectralCentroid->input("array").set(eAudioBuffer);
    aSpectralCentroid->output("centroid").set(eSpectralCentroid);

    // Loudness
    aLoudness->input("signal").set(eAudioBuffer);
    aLoudness->output("loudness").set(eLoudness);

    // Spectral peaks
    aSpectralPeaks->input("spectrum").set(eSpectrumData);
    aSpectralPeaks->output("frequencies").set(eSpectralPeaksFrequencies);
    aSpectralPeaks->output("magnitudes").set(eSpectralPeaksMagnitudes);
}

Analyser::~Analyser() {
}
