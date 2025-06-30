#ifndef CAN_READER_H
#define CAN_READER_H

#include <stdint.h>
#include <mcp_can.h>

// Vehicle type enumeration
typedef enum {
    VEHICLE_BMW,
    VEHICLE_KAWASAKI,
    VEHICLE_UNKNOWN
} VehicleType_t;

// CAN Reader context structure
typedef struct {
    VehicleType_t vehicleType;
    MCP_CAN* canInterface;
    bool* displayUpdated;
} CAN_Reader_Context_t;

// Function prototypes
void CAN_Reader_init(CAN_Reader_Context_t* ctx, VehicleType_t vehicleType, MCP_CAN* canInterface, bool* displayUpdated);
void CAN_Reader_readMessages(CAN_Reader_Context_t* ctx, void* bmw_ctx, void* kawasaki_data);

#endif // CAN_READER_H
