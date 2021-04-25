//
// Created by Max on 31/03/2021.
//

#ifndef DMLAP_BACKEND_CONSTANTS_H
#define DMLAP_BACKEND_CONSTANTS_H

/**
 * System-wide constants: These must be the same here and in the python agent!
 */

// Number of grains in trajectory
static int GRAINS_IN_TRAJECTORY = 33;
// Grain length in samples
static int GRAIN_LENGTH = 4096;
// The number of audio features used to calculate distances
static int NUM_FEATURES = 4;

#endif //DMLAP_BACKEND_CONSTANTS_H
