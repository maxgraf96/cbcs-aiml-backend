//
// Created by Max on 31/03/2021.
//

#include "DBConnector.h"

DBConnector::DBConnector() {
    char *zErrMsg = nullptr;
    int rc;
    string sql;

    /* Open database */
    rc = sqlite3_open("/tmp/test.db", &db);
//    rc = sqlite3_open(":memory:", &db);

    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    } else {
        fprintf(stdout, "Opened database successfully\n");
    }

    // Create new table
    sql = "CREATE TABLE IF NOT EXISTS GRAIN("
      "ID INT PRIMARY KEY," \
      "NAME           TEXT    NOT NULL,"
      "PATH           TEXT    NOT NULL,"
      "IDX            INT     NOT NULL,"
      "LOUDNESS       REAL    NOT NULL,"
      "SPECTRAL_CENTROID         REAL NOT NULL,"
      "SPECTRAL_FLUX REAL NOT NULL,"
      "PITCH REAL NOT NULL"
      ");";
    rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &zErrMsg);

    if( rc != SQLITE_OK ){
//        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
//        fprintf(stdout, "Table created successfully\n");
    }
}

void DBConnector::insertGrains(vector<Grain>& grains) {
    std::string sql;

    sql = "INSERT INTO GRAIN (NAME, PATH, IDX, LOUDNESS, SPECTRAL_CENTROID, SPECTRAL_FLUX, PITCH) VALUES ";

    for(unsigned long i = 0; i < grains.size(); i++){
        Grain& grain = grains.at(i);
        sql += "('"
                + grain.getName() +  "', '"
                + grain.getPath() + "', " + to_string(grain.getIdx()) + ", "
                + to_string(grain.getLoudness()) + ", "
                + to_string(grain.getSpectralCentroid()) + ", "
                + to_string(grain.getSpectralFlux()) + ", "
                + to_string(grain.getPitch())
                + ")";
        if(i < grains.size() - 1){
            sql += ",";
        }
    }
    sql += ";";

    char *zErrMsg = nullptr;
    int rc;

    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &zErrMsg);
    if( rc != SQLITE_OK ){
//        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
}

DBConnector::~DBConnector() {
    sqlite3_close(db);
}

static int closestGrainCallback(void *data, int argc, char **argv, char **azColName){
    vector<Grain>* grains = static_cast<vector<Grain>*>(data);

    try{
        // Parse grain
        Grain grain;

        string name(argv[1]);
        string path(argv[2]);
        int idx = (int)strtod(argv[3], nullptr);
        float loudness = (float)strtod(argv[4], nullptr);
        float sc = (float)strtod(argv[5], nullptr);
        float spectralFlux = (float)strtod(argv[6], nullptr);
        float pitch = (float)strtod(argv[7], nullptr);

        grain.setIdx(idx);
        grain.setPath(path);
        grain.setLoudness(loudness);
        grain.setSpectralCentroid(sc);
        grain.setSpectralFlux(spectralFlux);
        grain.setPitch(pitch);

        grains->emplace_back(grain);
    } catch (...) {
        fprintf(stderr, "Exception while parsing grain from database.");
        return 1;
    }
    return 0;
}


vector<Grain> DBConnector::queryClosestGrain(Grain &grain, float margin) {
    // Get grain values
    string marginStr = to_string(margin);
    string loudness = to_string(grain.getLoudness());
    string sc = to_string(grain.getSpectralCentroid());

    float scMargin = margin * 5.0f;
    float sfMargin = 0.1f;

    string sql;
    sql = "SELECT * FROM GRAIN WHERE"
          " LOUDNESS BETWEEN " + to_string(grain.getLoudness() - 10) + " AND " + to_string(grain.getLoudness() + 10) +
          " AND " +
          " SPECTRAL_CENTROID BETWEEN " + to_string(grain.getSpectralCentroid() - scMargin) + " AND " + to_string(grain.getSpectralCentroid() + scMargin) +
          " AND " +
          " SPECTRAL_FLUX BETWEEN " + to_string(grain.getSpectralFlux() - sfMargin) + " AND " + to_string(grain.getSpectralFlux() + sfMargin) +
          " AND " +
          " PITCH BETWEEN " + to_string(grain.getPitch() - scMargin) + " AND " + to_string(grain.getPitch() + scMargin) +
//          " ORDER BY ABS(SPECTRAL_CENTROID - " + to_string(grain.getSpectralCentroid()) + ")" +
          " LIMIT 10"
          ";";

    char *zErrMsg = nullptr;
    int rc;
    vector<Grain> found;
    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql.c_str(), closestGrainCallback, &found, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    return found;
}

static int isPopulatedCallback(void *data, int argc, char **argv, char **azColName){
    bool* populated = static_cast<bool*>(data);
    int count = (int)strtod(argv[0], nullptr);
    if(count > 0){
        *populated = true;
        return 0;
    }
    return 1;
}

bool DBConnector::isPopulated() {
    string sql = "SELECT count(*) FROM (select 0 from GRAIN limit 1);";

    char *zErrMsg = nullptr;
    int rc;

    bool populated = false;
    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql.c_str(), isPopulatedCallback, &populated, &zErrMsg);

    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    return populated;
}

static int minMaxMeanStdCallback(void *data, int argc, char **argv, char **azColName){
    float* minMax = static_cast<float*>(data);
    try{
        float test = (float)strtod(argv[0],NULL);
        *minMax = test;
    } catch (int e) {
        fprintf(stderr, "Exception: No min/max/mean/std found in database");
        return 1;
    }
    return 0;
}

float DBConnector::queryMax(const string& field) {
    std::string sql;

    sql = "SELECT MAX(" + field + ") FROM GRAIN;";
    char *zErrMsg = nullptr;

    float max;
    /* Execute SQL statement */
    int rc = sqlite3_exec(db, sql.c_str(), minMaxMeanStdCallback, &max, &zErrMsg);

    return max;
}

float DBConnector::queryMin(const string& field) {
    std::string sql;

    sql = "SELECT MIN(" + field + ") FROM GRAIN;";
    char *zErrMsg = nullptr;

    float min;
    /* Execute SQL statement */
    int rc = sqlite3_exec(db, sql.c_str(), minMaxMeanStdCallback, &min, &zErrMsg);

    return min;
}

float DBConnector::queryMean(const string& field) {
    std::string sql;

    sql = "SELECT AVG(" + field + ") FROM GRAIN;";
    char *zErrMsg = nullptr;

    float mean;
    /* Execute SQL statement */
    int rc = sqlite3_exec(db, sql.c_str(), minMaxMeanStdCallback, &mean, &zErrMsg);

    return mean;
}

float DBConnector::queryStd(const string& field) {
    std::string sql;

    sql = "SELECT AVG(" + field + "*" + field + ") - AVG(" + field + ") * AVG(" + field + ") FROM GRAIN;";
    char *zErrMsg = nullptr;

    float var;
    /* Execute SQL statement */
    int rc = sqlite3_exec(db, sql.c_str(), minMaxMeanStdCallback, &var, &zErrMsg);
    float std = sqrtf(var);

    return std;
}