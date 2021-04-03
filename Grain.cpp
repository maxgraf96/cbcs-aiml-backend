//
// Created by Max on 31/03/2021.
//

#include "Grain.h"

#include <utility>

Grain::Grain(string name,
             string path,
             int idx,
             float loudness,
             float spectralCentroid,
             float spectralFlux,
             float pitch
             ):
    name(std::move(name)),
    path(std::move(path)),
    idx(idx),
    loudness(loudness),
    spectralCentroid(spectralCentroid),
    spectralFlux(spectralFlux),
    pitch(pitch)
    {}

Grain::Grain(float loudness,
             float spectralCentroid,
             float spectralFlux,
             float pitch
             ):
             loudness(loudness),
             spectralCentroid(spectralCentroid),
             spectralFlux(spectralFlux),
             pitch(pitch)
             {}

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

float Grain::getSpectralFlux() const {
    return spectralFlux;
}

void Grain::setSpectralFlux(float sf) {
    Grain::spectralFlux = sf;
}

float Grain::getPitch() const {
    return pitch;
}

void Grain::setPitch(float pitch) {
    Grain::pitch = pitch;
}
