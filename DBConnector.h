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

/**
 * Helper class for database management. Creates a /connects to a sqlite database for storing grain information.
 * Database entries contain paths to source audio files, start indices of the grains, as well as audio feature data.
 */
class DBConnector {
public:
    DBConnector();
    ~DBConnector();

    /**
     * Store a vector of grains in the database
     * @param grains
     */
    void insertGrains(vector<Grain>& grains);

    /**
     * Query the database for the set of grains closest to the input grain.
     * @param grain The input grain
     * @param margin Margin used for db query: This margin is added to the audio feature data in case no grains are found.
     * @return A vector of the closest matching grains in the database
     */
    vector<Grain> queryClosestGrain(Grain& grain, float margin);

    /**
     * Get a random vector of grains from the database.
     * @return The random vector of grains.
     */
    vector<Grain> queryRandomTrajectory();

    /**
     * Query the maximum value of a given db field (column).
     * @param field
     * @return
     */
    float queryMax(const string& field);

    /**
     * Query the minimum value of a given db field.
     * @param field
     * @return
     */
    float queryMin(const string& field);

    /**
     * Query the mean value of a given db field.
     * @param field
     * @return
     */
    float queryMean(const string& field);

    /**
     * Query the standard deviation of a given db field.
     * @param field
     * @return
     */
    float queryStd(const string& field);

    /**
     * Helper method to check whether the database contains any data.
     * @return True if a minimum of 1 grain exists in the database, false otherwise
     */
    bool isPopulated();

private:
    // SQLite Databases: One in memory and one on disk. The idea is that grain data is always managed in memory
    // because it is significantly faster. This imposes a limit on the size of the database.
    // Upon exiting the application, the data from the in-memory db is written to disk in order to persist it.
    // When loading the application, it tries to read data from the disk-db into the in-memory db.

    // Database in memory
    sqlite3 *db;
    // Database on disk
    sqlite3 *dbDisk;

    // Path to database for disk (currently db is generated in temp dir)
    string DB_PATH = "/tmp/test.db";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DBConnector)
};

#endif //DMLAP_BACKEND_DBCONNECTOR_H
