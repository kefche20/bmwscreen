#ifndef KAWASAKI_CAN_H
#define KAWASAKI_CAN_H

#include <stdint.h>

typedef struct {
    int rpm;
    int coolantTemp;
    int tps; // Throttle Position Sensor
    int iap; // Intake Air Pressure
    int ect;
} Kawasaki_CAN_Data_t;

// Parsing function prototype
void Kawasaki_parseCANMessage(uint32_t rxId, uint8_t len, const uint8_t* buf, Kawasaki_CAN_Data_t* data, bool* displayUpdated);

#endif // KAWASAKI_CAN_H 