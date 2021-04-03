//
// Created by Max on 30/03/2021.
//

#include "Analyser.h"

Analyser::Analyser(DBConnector& connector)
: dbConnector(connector){

}

void Analyser::analyseAndSaveToDB(AudioBuffer<float>& buffer, const string& filename, const string& path){
    int index = 0;
    int counter = 1;
    vector<Grain> grains;
    while(index + GRAIN_LENGTH < buffer.getNumSamples()){
        // Clear essentia audiobuffer
        eAudioBuffer.clear();

        auto* reader = buffer.getReadPointer(0);
        for (int i = index; i < index + GRAIN_LENGTH; i++){
            eAudioBuffer.push_back(reader[i]);
        }

        if(computeFeatures()){
            grains.emplace_back(Grain(filename + to_string(counter),
                                      path,
                                      index,
                                      eLoudness,
                                      eSpectralCentroid,
                                      eSpectralFlux,
                                      ePitch
            ));
        }

        // Update index -> start of next grain
        index += GRAIN_LENGTH;
        counter++;
    }
    // Save grains to database
    dbConnector.insertGrains(grains);
}

vector<Grain> Analyser::audioBufferToGrains(AudioBuffer<float>& buffer){
    int index = 0;
    int counter = 1;
    vector<Grain> grains;
    while(index + GRAIN_LENGTH < buffer.getNumSamples()){
        // Clear essentia audiobuffer
        eAudioBuffer.clear();

        auto* reader = buffer.getReadPointer(0);
        for (int i = index; i < index + GRAIN_LENGTH; i++){
            eAudioBuffer.push_back(reader[i]);
        }

        computeFeatures();

        Grain* grain = new Grain(eLoudness, eSpectralCentroid, eSpectralFlux, ePitch);
        grains.emplace_back(*grain);

        // Update index -> start of next grain
        index += GRAIN_LENGTH;
        counter++;
    }

    return grains;
}

bool Analyser::computeFeatures(){
    // Essentia algorithms compute routines
    aRMS->compute();
    // Skip the grain if silent
    if(eRMS < 0.01f){
        return false;
    }
    aWindowing->compute();
    aSpectrum->compute();
    aSpectralCentroid->compute();
    aLoudness->compute();
    aSpectralFlux->compute();
    aPitchYINFFT->compute();
    return true;
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
    aSpectralFlux.reset(factory.create("Flux"));
    aPitchYINFFT.reset(factory.create("PitchYinFFT"));
    aRMS.reset(factory.create("RMS"));

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

    // Spectral flux
    aSpectralFlux->input("spectrum").set(eSpectrumData);
    aSpectralFlux->output("flux").set(eSpectralFlux);

    // Pitch
    aPitchYINFFT->input("spectrum").set(eSpectrumData);
    aPitchYINFFT->output("pitch").set(ePitch);
    aPitchYINFFT->output("pitchConfidence").set(ePitchConfidence);

    // RMS
    aRMS->input("array").set(eAudioBuffer);
    aRMS->output("rms").set(eRMS);
}

Analyser::~Analyser() = default;
