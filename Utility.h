//
// Created by Max on 01/04/2021.
//

#ifndef DMLAP_BACKEND_UTILITY_H
#define DMLAP_BACKEND_UTILITY_H
static float mapFloat(float val, float inLow, float inHigh, float outLow, float outHigh){
    float x = (val - inLow) / (inHigh - inLow);
    return outLow + (outHigh - outLow) * x;
}
#endif //DMLAP_BACKEND_UTILITY_H
