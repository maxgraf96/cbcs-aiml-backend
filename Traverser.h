//
// Created by Max on 31/03/2021.
//

#ifndef DMLAP_BACKEND_TRAVERSER_H
#define DMLAP_BACKEND_TRAVERSER_H

#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "sqlite3.h"
#include "DBConnector.h"
#include "Grain.h"
#include "Constants.h"

using namespace std;
using namespace juce;

class Traverser {
public:
    Traverser(DBConnector& connector);

    AudioBuffer<float> initialiseRandomTrajectory(int lengthInGrains);

private:
    DBConnector& dbConnector;
    vector<Grain> source;
    vector<Grain> target;
    AudioFormatManager formatManager;
};


#endif //DMLAP_BACKEND_TRAVERSER_H
