//
// Created by Max on 31/03/2021.
//

#ifndef DMLAP_BACKEND_DBCONNECTOR_H
#define DMLAP_BACKEND_DBCONNECTOR_H
#include <cstdio>
#include <vector>
#include <set>
#include <juce_audio_utils/juce_audio_utils.h>
#include "sqlite3.h"
#include "Grain.h"
#include "Constants.h"


class DBConnector {
public:
    DBConnector();
    ~DBConnector();

    void insertGrains(vector<Grain>& grains);
    vector<Grain> queryClosestGrain(Grain& grain, float margin);
    vector<Grain> queryRandomTrajectory();
    float queryMax(const string& field);
    float queryMin(const string& field);
    float queryMean(const string& field);
    float queryStd(const string& field);

    bool isPopulated();

private:
    // SQLite Database
    sqlite3 *db;
    sqlite3 *dbDisk;
    string DB_PATH = "/Users/max/Desktop/test.db";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DBConnector)
};

struct closestGrainObject {
    int numFound = 0;
    vector<Grain> found;
};

#endif //DMLAP_BACKEND_DBCONNECTOR_H
