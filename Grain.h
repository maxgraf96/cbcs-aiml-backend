//
// Created by Max on 31/03/2021.
//

#ifndef DMLAP_BACKEND_GRAIN_H
#define DMLAP_BACKEND_GRAIN_H
#include <stdio.h>
#include <stdlib.h>
#include <string>

using namespace std;

class Grain {
public:
    Grain();

    Grain(const string &name, const string &path, int idx, float loudness, float spectralCentroid, float spectralFlux);
    Grain(float loudness, float spectralCentroid, float spectralFlux);

private:
    string name = "";
    string path = "";
    int idx = -1;
    float loudness = 0.0f;
    float spectralCentroid = 0.0f;
    float spectralFlux = 0.0f;

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

};


#endif //DMLAP_BACKEND_GRAIN_H
