//
// Created by Max on 31/03/2021.
//

#ifndef DMLAP_BACKEND_DBCONNECTOR_H
#define DMLAP_BACKEND_DBCONNECTOR_H
#include <cstdio>
#include <vector>
#include <set>
#include "sqlite3.h"
#include "Grain.h"


class DBConnector {
public:
    DBConnector();
    ~DBConnector();

    void insertGrains(vector<Grain>& grains);
    vector<Grain> queryClosestGrain(Grain& grain, float margin);
    float queryMax(const string& field);
    float queryMin(const string& field);
    float queryMean(const string& field);
    float queryStd(const string& field);

    bool isPopulated();

private:
    // SQLite Database
    sqlite3 *db;
    string DB_PATH = "/Users/max/Desktop/test.db";
};


#endif //DMLAP_BACKEND_DBCONNECTOR_H
