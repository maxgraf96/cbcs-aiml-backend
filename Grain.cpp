//
// Created by Max on 31/03/2021.
//

#include "Grain.h"

Grain::Grain(const string &name, const string &path, int idx, float loudness, float spectralCentroid)
    : name(name), path(path), idx(idx), loudness(loudness), spectralCentroid(spectralCentroid){
}

Grain::Grain(float loudness, float spectralCentroid)
    : loudness(loudness), spectralCentroid(spectralCentroid) {
}

Grain::Grain() {

}

const string &Grain::getName() const {
    return name;
}

void Grain::setName(const string &name) {
    Grain::name = name;
}

const string &Grain::getPath() const {
    return path;
}

void Grain::setPath(const string &path) {
    Grain::path = path;
}

int Grain::getIdx() const {
    return idx;
}

void Grain::setIdx(int idx) {
    Grain::idx = idx;
}

float Grain::getSpectralCentroid() const {
    return spectralCentroid;
}

void Grain::setSpectralCentroid(float spectralCentroid) {
    Grain::spectralCentroid = spectralCentroid;
}

float Grain::getLoudness() const {
    return loudness;
}

void Grain::setLoudness(float loudness) {
    Grain::loudness = loudness;
}
