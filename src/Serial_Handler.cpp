#include "Serial_Handler.h"
#include <Arduino.h>

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
                        VehicleStatusCallback_t vehicleStatusCallback) {
    
    // Initialize buffer
    ctx->serialBufferIndex = 0;
    memset(ctx->serialBuffer, 0, SERIAL_BUFFER_SIZE);
    
    // Store pointers to main variables
    ctx->currentScreen = currentScreen;
    ctx->devMode = devMode;
    ctx->vehicleType = vehicleType;
    ctx->canReaderCtx = canReaderCtx;
    ctx->canInterface = canInterface;
    ctx->displayUpdated = displayUpdated;
    
    // Store callback functions
    ctx->screenChangeCallback = screenChangeCallback;
    ctx->modeChangeCallback = modeChangeCallback;
    ctx->introCallback = introCallback;
    ctx->vinRequestCallback = vinRequestCallback;
    ctx->vehicleTypeChangeCallback = vehicleTypeChangeCallback;
    ctx->vehicleStatusCallback = vehicleStatusCallback;
}

void Serial_Handler_processInput(Serial_Handler_Context_t* ctx) {
    while (Serial.available()) {
        char c = Serial.read();
        
        // Handle backspace
        if (c == '\b' || c == 127) {
            if (ctx->serialBufferIndex > 0) {
                ctx->serialBufferIndex--;
                Serial.print("\b \b"); // Erase the character on screen
            }
            continue;
        }
        
        // Handle enter key
        if (c == '\n' || c == '\r') {
            if (ctx->serialBufferIndex > 0) {
                ctx->serialBuffer[ctx->serialBufferIndex] = '\0'; // Null terminate the string
                
                // Process the command
                if (strcmp(ctx->serialBuffer, "screen1") == 0) {
                    *ctx->currentScreen = 0;
                    Serial.println("Switched to RPM Screen");
                }
                else if (strcmp(ctx->serialBuffer, "screen2") == 0) {
                    *ctx->currentScreen = 1;
                    Serial.println("Switched to Temperature Screen");
                }
                else if (strcmp(ctx->serialBuffer, "screen3") == 0) {
                    *ctx->currentScreen = 2;
                    Serial.println("Switched to RPM Meter Screen");
                }
                else if (strcmp(ctx->serialBuffer, "screen4") == 0) {
                    *ctx->currentScreen = 3;
                    Serial.println("Switched to Detailed Temperature Screen");
                }
                else if (strcmp(ctx->serialBuffer, "demo") == 0) {
                    *ctx->devMode = true;
                    if (ctx->modeChangeCallback) {
                        ctx->modeChangeCallback(true);
                    }
                    Serial.println("Switched to Development/Demo Mode");
                }
                else if (strcmp(ctx->serialBuffer, "real") == 0) {
                    *ctx->devMode = false;
                    if (ctx->modeChangeCallback) {
                        ctx->modeChangeCallback(false);
                    }
                    Serial.println("Switched to Real Mode - Waiting for CAN data...");
                }
                else if (strcmp(ctx->serialBuffer, "showintro") == 0) {
                    if (ctx->introCallback) {
                        ctx->introCallback();
                    }
                    Serial.println("Showing Intro");
                }
                else if (strcmp(ctx->serialBuffer, "help") == 0) {
                    Serial_Handler_printHelp();
                }
                else if (strcmp(ctx->serialBuffer, "getvin") == 0) {
                    if (!*ctx->devMode) {
                        if (ctx->vinRequestCallback) {
                            ctx->vinRequestCallback();
                        }
                        Serial.println("Requesting VIN from instrument cluster...");
                    } else {
                        Serial.println("VIN request only works in real mode");
                    }
                }
                else if (strcmp(ctx->serialBuffer, "vehicle bmw") == 0) {
                    *ctx->vehicleType = VEHICLE_BMW;
                    CAN_Reader_init(ctx->canReaderCtx, *ctx->vehicleType, ctx->canInterface, ctx->displayUpdated);
                    if (ctx->vehicleTypeChangeCallback) {
                        ctx->vehicleTypeChangeCallback(VEHICLE_BMW);
                    }
                    Serial.println("Switched to BMW vehicle mode");
                }
                else if (strcmp(ctx->serialBuffer, "vehicle kawasaki") == 0) {
                    *ctx->vehicleType = VEHICLE_KAWASAKI;
                    CAN_Reader_init(ctx->canReaderCtx, *ctx->vehicleType, ctx->canInterface, ctx->displayUpdated);
                    if (ctx->vehicleTypeChangeCallback) {
                        ctx->vehicleTypeChangeCallback(VEHICLE_KAWASAKI);
                    }
                    Serial.println("Switched to Kawasaki vehicle mode");
                }
                else if (strcmp(ctx->serialBuffer, "vehicle unknown") == 0) {
                    *ctx->vehicleType = VEHICLE_UNKNOWN;
                    CAN_Reader_init(ctx->canReaderCtx, *ctx->vehicleType, ctx->canInterface, ctx->displayUpdated);
                    if (ctx->vehicleTypeChangeCallback) {
                        ctx->vehicleTypeChangeCallback(VEHICLE_UNKNOWN);
                    }
                    Serial.println("Switched to Unknown vehicle mode (tries both parsers)");
                }
                else if (strcmp(ctx->serialBuffer, "vehicle status") == 0) {
                    if (ctx->vehicleStatusCallback) {
                        ctx->vehicleStatusCallback(*ctx->vehicleType);
                    } else {
                        Serial.print("Current vehicle type: ");
                        switch (*ctx->vehicleType) {
                            case VEHICLE_BMW:
                                Serial.println("BMW");
                                break;
                            case VEHICLE_KAWASAKI:
                                Serial.println("Kawasaki");
                                break;
                            case VEHICLE_UNKNOWN:
                                Serial.println("Unknown (tries both parsers)");
                                break;
                        }
                    }
                }
                else {
                    Serial.println("Unknown command. Type 'help' for available commands.");
                }
                
                // Reset buffer
                ctx->serialBufferIndex = 0;
                Serial_Handler_printPrompt();
            }
        }
        // Handle regular character input
        else if (ctx->serialBufferIndex < SERIAL_BUFFER_SIZE - 1) {
            ctx->serialBuffer[ctx->serialBufferIndex++] = c;
            Serial.print(c); // Echo the character
        }
    }
}

void Serial_Handler_printHelp(void) {
    Serial.println("\nAvailable commands:");
    Serial.println("screen1 - RPM Screen");
    Serial.println("screen2 - Temperature Screen");
    Serial.println("screen3 - RPM Meter Screen");
    Serial.println("screen4 - Detailed Temperature Screen");
    Serial.println("demo - Switch to Development/Demo Mode");
    Serial.println("real - Switch to Real Mode (CAN data)");
    Serial.println("showintro - Show Intro");
    Serial.println("getvin - Request VIN from instrument cluster");
    Serial.println("vehicle bmw - Switch to BMW vehicle mode");
    Serial.println("vehicle kawasaki - Switch to Kawasaki vehicle mode");
    Serial.println("vehicle unknown - Switch to Unknown vehicle mode (tries both parsers)");
    Serial.println("vehicle status - Show current vehicle type");
}

void Serial_Handler_printPrompt(void) {
    Serial.print("> "); // Show prompt
} 