#ifndef BMW_CAN_H
#define BMW_CAN_H

#include <stdint.h>

// BMW DME1 (0x316) - Engine Status and Performance
typedef struct {
    bool ignition;
    bool cranking;
    bool tcs;
    int torque;
    int rpm;
    int torqueLoss;
} BMW_DME1_t;

// BMW DME2 (0x329) - Engine Temperatures and Pressure
typedef struct {
    int coolantTemp;
    int manifoldPressure;
} BMW_DME2_t;

// BMW DME4 (0x545) - Warning Lights
typedef struct {
    bool mil;
    bool cruise;
    bool eml;
} BMW_DME4_t;

// MS42 Temperatures
typedef struct {
    int intakeTemp;
    int oilTemp;
    int outletTemp;
} BMW_MS42_Temp_t;

// MS42 Status
typedef struct {
    int fuelPressure;
    int lambda;
    int maf;
} BMW_MS42_Status_t;

// Instrument Cluster Data
typedef struct {
    char vin[8];
    bool vinReceived;
} BMW_Kombi_t;

// Parsing function
typedef struct {
    BMW_DME1_t* dme1;
    BMW_DME2_t* dme2;
    BMW_DME4_t* dme4;
    BMW_MS42_Temp_t* ms42_temp;
    BMW_MS42_Status_t* ms42_status;
    BMW_Kombi_t* kombi;
} BMW_CAN_Context_t;

void BMW_parseCANMessage(uint32_t rxId, uint8_t len, const uint8_t* buf, BMW_CAN_Context_t* ctx, bool* displayUpdated);

#endif // BMW_CAN_H 