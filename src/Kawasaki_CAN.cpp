#include "Kawasaki_CAN.h"
#include <stdio.h>

void Kawasaki_parseCANMessage(uint32_t rxId, uint8_t len, const uint8_t* buf, Kawasaki_CAN_Data_t* data, bool* displayUpdated) {
    // Kawasaki FI Calibration Tool Main Diagnostic Frame: 0x620 (8 bytes)
    // Byte 0-1: RPM (big endian)
    // Byte 2: Throttle Position Sensor (TPS)
    // Byte 3: Intake Air Pressure (IAP)
    // Byte 4: Engine Coolant Temp (ECT)
    // Byte 5-7: (other sensors, not implemented here)
    if (rxId == 0x620 && len >= 8) {
        data->rpm = (buf[0] << 8) | buf[1];
        data->tps = buf[2];
        data->iap = buf[3];
        data->ect = buf[4];
        data->coolantTemp = buf[4];
        *displayUpdated = true;
    }
} 