#ifndef SERIAL_HANDLER_H
#define SERIAL_HANDLER_H

#include <stdint.h>
#include <mcp_can.h>
#include "CAN_Reader.h"

// Serial buffer configuration
#define SERIAL_BUFFER_SIZE 32

// Callback function types for different commands
typedef void (*ScreenChangeCallback_t)(int screen);
typedef void (*ModeChangeCallback_t)(bool devMode);
typedef void (*IntroCallback_t)(void);
typedef void (*VINRequestCallback_t)(void);
typedef void (*VehicleTypeChangeCallback_t)(VehicleType_t vehicleType);
typedef void (*VehicleStatusCallback_t)(VehicleType_t vehicleType);

// Serial Handler context structure
typedef struct {
    char serialBuffer[SERIAL_BUFFER_SIZE];
    int serialBufferIndex;
    
    // Callback functions
    ScreenChangeCallback_t screenChangeCallback;
    ModeChangeCallback_t modeChangeCallback;
    IntroCallback_t introCallback;
    VINRequestCallback_t vinRequestCallback;
    VehicleTypeChangeCallback_t vehicleTypeChangeCallback;
    VehicleStatusCallback_t vehicleStatusCallback;
    
    // State variables (pointers to main variables)
    int* currentScreen;
    bool* devMode;
    VehicleType_t* vehicleType;
    CAN_Reader_Context_t* canReaderCtx;
    MCP_CAN* canInterface;
    bool* displayUpdated;
} Serial_Handler_Context_t;

// Function prototypes
void Serial_Handler_init(Serial_Handler_Context_t* ctx, 
                        int* currentScreen, 
                        bool* devMode, 
                        VehicleType_t* vehicleType,
                        CAN_Reader_Context_t* canReaderCtx,
                        MCP_CAN* canInterface,
                        bool* displayUpdated,
                        ScreenChangeCallback_t screenChangeCallback,
                        ModeChangeCallback_t modeChangeCallback,
                        IntroCallback_t introCallback,
                        VINRequestCallback_t vinRequestCallback,
                        VehicleTypeChangeCallback_t vehicleTypeChangeCallback,
                        VehicleStatusCallback_t vehicleStatusCallback);

void Serial_Handler_processInput(Serial_Handler_Context_t* ctx);
void Serial_Handler_printHelp(void);
void Serial_Handler_printPrompt(void);

#endif // SERIAL_HANDLER_H 