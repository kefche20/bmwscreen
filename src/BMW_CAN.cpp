#include "BMW_CAN.h"
#include <string.h>
#include <stdio.h>

void BMW_parseCANMessage(uint32_t rxId, uint8_t len, const uint8_t* buf, BMW_CAN_Context_t* ctx, bool* displayUpdated) {
    if (rxId == 0x316 && len >= 8) {
        ctx->dme1->ignition = (buf[0] & 0x01) > 0;
        ctx->dme1->cranking = (buf[0] & 0x02) > 0;
        ctx->dme1->tcs = (buf[0] & 0x04) > 0;
        ctx->dme1->torque = buf[1];
        ctx->dme1->rpm = (buf[3] << 8) | buf[2];
        ctx->dme1->torqueLoss = buf[5];
        *displayUpdated = true;
    } else if (rxId == 0x329 && len >= 3) {
        ctx->dme2->coolantTemp = (int)((float)buf[1] * 0.75 - 48);
        ctx->dme2->manifoldPressure = buf[2] == 0xFF ? -999 : (int)(buf[2] * 2 + 598);
        *displayUpdated = true;
    } else if (rxId == 0x545 && len >= 1) {
        ctx->dme4->mil = (buf[0] & 0x02) > 0;
        ctx->dme4->cruise = (buf[0] & 0x08) > 0;
        ctx->dme4->eml = (buf[0] & 0x10) > 0;
        *displayUpdated = true;
    }
} 