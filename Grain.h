//
// Created by Max on 31/03/2021.
//

#ifndef DMLAP_BACKEND_GRAIN_H
#define DMLAP_BACKEND_GRAIN_H
#include <stdio.h>
#include <stdlib.h>
#include <string>

using namespace std;

/**
 * Representation of a grain object.
 * In this systems grains are comprised of a path to the source audio file, a start index (of the audio data), and the
 * grain length defined in "Constants.h". The audio data of a grain is given by this triplet. In addition to this,
 * a grain also contains the values of the audio features computed in "Analyser.h".
 */
class Grain {
public:
    Grain();

    Grain(string name,
          string path,
          int idx,
          float loudness,
          float spectralCentroid,
          float spectralFlux,
          float pitch
          );
    Grain(float loudness,
          float spectralCentroid,
          float spectralFlux,
          float pitch);

private:
    // The name of the grain (composed of the path of the source audio file and a number)
    string name;
    // Absolute path to the source audio file
    string path;
    // Start index of the grain
    int idx = -1;
    // Loudness of the grain
    float loudness = 0.0f;
    // Spectral centroid ("brightness") of the grain
    float spectralCentroid = 0.0f;
    // Spectral flux of the grain
    float spectralFlux = 0.0f;
    // Pitch of the grain (in Hz)
    float pitch = 0.0f;

public:

    const string &getName() const;

    void setName(const string &name);

    const string &getPath() const;

    void setPath(const string &path);

    int getIdx() const;

    void setIdx(int idx);

    float getLoudness() const;

    void setLoudness(float loudness);

    float getSpectralCentroid() const;

    void setSpectralCentroid(float spectralCentroid);

    float getSpectralFlux() const;

    void setSpectralFlux(float sf);

    float getPitch() const;

    void setPitch(float pitch);

};


#endif //DMLAP_BACKEND_GRAIN_H
