//
// Created by Max on 31/03/2021.
//

#include "DBConnector.h"

DBConnector::DBConnector() {
    char *zErrMsg = nullptr;
    int rc;
    char *sql;

    /* Open database */
    rc = sqlite3_open("/tmp/test.db", &db);
//    rc = sqlite3_open(":memory:", &db);

    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    } else {
        fprintf(stdout, "Opened database successfully\n");
    }

    // Drop table
//    sql = "DROP TABLE GRAIN;";
//    rc = sqlite3_exec(db, sql, nullptr, nullptr, &zErrMsg);

    // Create new table
    sql = "CREATE TABLE IF NOT EXISTS GRAIN("  \
      "ID INT PRIMARY KEY," \
      "NAME           TEXT    NOT NULL," \
      "PATH           TEXT    NOT NULL," \
      "IDX            INT     NOT NULL," \
      "LOUDNESS       REAL    NOT NULL," \
      "SPECTRAL_CENTROID         REAL NOT NULL);";
    rc = sqlite3_exec(db, sql, nullptr, nullptr, &zErrMsg);

    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "Table created successfully\n");
    }
}

void DBConnector::insertGrain(Grain &grain) {
    std::string sql;

    sql = "INSERT INTO GRAIN (NAME, PATH, IDX, LOUDNESS, SPECTRAL_CENTROID) VALUES ('"
          + grain.getName() +  "', '"
          + grain.getPath() + "', " + to_string(grain.getIdx()) + ", "
          + to_string(grain.getLoudness()) + ", "
          + to_string(grain.getSpectralCentroid()) + " );";


    const char* cstr = sql.c_str();

    char *zErrMsg = nullptr;
    int rc;

    /* Execute SQL statement */
    rc = sqlite3_exec(db, cstr, nullptr, nullptr, &zErrMsg);

    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
}

void DBConnector::insertGrains(vector<unique_ptr<Grain>>& grains) {
    std::string sql;

    sql = "INSERT INTO GRAIN (NAME, PATH, IDX, LOUDNESS, SPECTRAL_CENTROID) VALUES ";

    for(unsigned long i = 0; i < grains.size(); i++){
        Grain& grain = *grains.at(i);
        sql += "('"
                + grain.getName() +  "', '"
                + grain.getPath() + "', " + to_string(grain.getIdx()) + ", "
                + to_string(grain.getLoudness()) + ", "
                + to_string(grain.getSpectralCentroid()) + ")";
        if(i < grains.size() - 1){
            sql += ",";
        }
    }

    sql += ";";


    const char* cstr = sql.c_str();

    char *zErrMsg = nullptr;
    int rc;

    /* Execute SQL statement */
    rc = sqlite3_exec(db, cstr, nullptr, nullptr, &zErrMsg);

    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
}

DBConnector::~DBConnector() {
    sqlite3_close(db);
}

static int minMaxCallback(void *data, int argc, char **argv, char **azColName){
    float* minMax = static_cast<float*>(data);
    try{
        float test = (float)strtod(argv[0],NULL);
        *minMax = test;
    } catch (int e) {
        fprintf(stderr, "Exception: No min/max found in database");
        return 1;
    }
    return 0;
}

float DBConnector::queryMax(string field) {
    std::string sql;

    sql = "SELECT MAX(" + field + ") FROM GRAIN;";
    char *zErrMsg = nullptr;

    float max;
    /* Execute SQL statement */
    int rc = sqlite3_exec(db, sql.c_str(), minMaxCallback, &max, &zErrMsg);

    return max;
}

float DBConnector::queryMin(string field) {
    std::string sql;

    sql = "SELECT MIN(" + field + ") FROM GRAIN;";
    char *zErrMsg = nullptr;

    float min;
    /* Execute SQL statement */
    int rc = sqlite3_exec(db, sql.c_str(), minMaxCallback, &min, &zErrMsg);

    return min;
}

static int closestGrainCallback(void *data, int argc, char **argv, char **azColName){
    fprintf(stdout, "%s: ", (const char*)data);

    int i;
//    for(i = 0; i<argc; i++){
//        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
//    }

    Grain* grain = static_cast<Grain*>(data);

    try{
        // Parse grain
        string name(argv[1]);
        string path(argv[2]);
        int idx = (int)strtod(argv[3],NULL);
        float loudness = (float)strtod(argv[4],NULL);
        float sc = (float)strtod(argv[5],NULL);

        grain->setIdx(idx);
        grain->setPath(path);
        grain->setLoudness(loudness);
        grain->setSpectralCentroid(sc);
    } catch (int e) {
        fprintf(stderr, "Exception while parsing grain from database.");
        return 1;
    }
    return 0;
}


Grain DBConnector::queryClosestGrain(Grain &grain, float margin) {
    // Get grain values
    string marginStr = to_string(margin);
    string loudness = to_string(grain.getLoudness());
    string sc = to_string(grain.getSpectralCentroid());

    float scMargin = margin * 5.0f;

    string sql;
    sql = "SELECT * FROM GRAIN WHERE"
          " LOUDNESS BETWEEN " + to_string(grain.getLoudness() - margin) + " AND " + to_string(grain.getLoudness() + margin) +
          " AND " +
          " SPECTRAL_CENTROID BETWEEN " + to_string(grain.getSpectralCentroid() - scMargin) + " AND " + to_string(grain.getSpectralCentroid() + scMargin) +
          " ORDER BY ABS(LOUDNESS - " + to_string(grain.getLoudness()) + ")" +
          " LIMIT 1"
          ";";

    char *zErrMsg = nullptr;
    int rc;

    Grain found;

    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql.c_str(), closestGrainCallback, &found, &zErrMsg);

    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    return found;
}
