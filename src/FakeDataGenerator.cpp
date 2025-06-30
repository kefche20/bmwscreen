#include "FakeDataGenerator.h"
#include <Arduino.h>

void FakeDataGenerator_updateBMW(BMW_CAN_Context_t* bmw_ctx) {
    if (!bmw_ctx) return;
    BMW_DME1_t* dme1 = bmw_ctx->dme1;
    BMW_DME2_t* dme2 = bmw_ctx->dme2;
    BMW_MS42_Temp_t* ms42_temp = bmw_ctx->ms42_temp;

    static unsigned long lastUpdate = 0;
    unsigned long now = millis();
    if (now - lastUpdate < 40) return;
    lastUpdate = now;

    // RPM: random walk between 2000 and 7500
    static int rpm = 2000;
    int rpmStep = random(-300, 301);
    rpm += rpmStep;
    rpm = constrain(rpm, 2000, 7500);
    dme1->rpm = rpm;

    // Coolant temp: random walk between 60 and 100
    static float coolantTemp = 80.0f;
    float tempStep = random(-10, 11) * 0.2f;
    coolantTemp += tempStep;
    coolantTemp = constrain(coolantTemp, 60.0f, 100.0f);
    dme2->coolantTemp = (int)coolantTemp;

    // Oil temp: slightly lower than coolant
    ms42_temp->oilTemp = dme2->coolantTemp - random(2, 8);
    ms42_temp->oilTemp = constrain(ms42_temp->oilTemp, 60, 98);
    ms42_temp->outletTemp = dme2->coolantTemp - 10;

    // Intake temp: random walk between 20 and 60
    static float intakeTemp = 40.0f;
    float intakeStep = random(-5, 6) * 0.5f;
    intakeTemp += intakeStep;
    intakeTemp = constrain(intakeTemp, 20.0f, 60.0f);
    ms42_temp->intakeTemp = (int)intakeTemp;

    // Torque: based on RPM, with some randomness
    dme1->torque = map(dme1->rpm, 2000, 7500, 30, 100) + random(-5, 6);
    dme1->torque = constrain(dme1->torque, 30, 100);
    dme1->torqueLoss = map(dme1->rpm, 2000, 7500, 0, 30) + random(-3, 4);
    dme1->torqueLoss = constrain(dme1->torqueLoss, 0, 30);
}

void FakeDataGenerator_updateKawasaki(Kawasaki_CAN_Data_t* kawasaki_data) {
    if (!kawasaki_data) return;
    static unsigned long lastUpdate = 0;
    unsigned long now = millis();
    if (now - lastUpdate < 40) return;
    lastUpdate = now;

    // RPM
    kawasaki_data->rpm += random(-400, 401);
    kawasaki_data->rpm = constrain(kawasaki_data->rpm, 1000, 14000);

    // Coolant temp
    kawasaki_data->coolantTemp += random(-2, 3);
    kawasaki_data->coolantTemp = constrain(kawasaki_data->coolantTemp, 60, 120);

    // TPS, IAP, ECT
    kawasaki_data->tps = random(0, 100);
    kawasaki_data->iap = random(80, 120);
    kawasaki_data->ect = random(0, 255);
} 