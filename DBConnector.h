//
// Created by Max on 31/03/2021.
//

#ifndef DMLAP_BACKEND_DBCONNECTOR_H
#define DMLAP_BACKEND_DBCONNECTOR_H
#include <stdio.h>
#include <vector>
#include "sqlite3.h"
#include "Grain.h"


class DBConnector {
public:
    DBConnector();
    ~DBConnector();

    void insertGrain(Grain& grain);
    void insertGrains(vector<unique_ptr<Grain>>& grain);
    Grain queryClosestGrain(Grain& grain, float margin);
    float queryMax(string field);
    float queryMin(string field);

private:
    // SQLite Database
    sqlite3 *db;
};


#endif //DMLAP_BACKEND_DBCONNECTOR_H
