#include "CAN_Reader.h"
#include "BMW_CAN.h"
#include "Kawasaki_CAN.h"

void CAN_Reader_init(CAN_Reader_Context_t* ctx, VehicleType_t vehicleType, MCP_CAN* canInterface, bool* displayUpdated) {
    ctx->vehicleType = vehicleType;
    ctx->canInterface = canInterface;
    ctx->displayUpdated = displayUpdated;
}

void CAN_Reader_readMessages(CAN_Reader_Context_t* ctx, void* bmw_ctx, void* kawasaki_data) {
    if (ctx->canInterface->checkReceive() == CAN_MSGAVAIL) {
        unsigned long rxId;
        unsigned char len = 0;
        unsigned char buf[8];
        ctx->canInterface->readMsgBuf(&rxId, &len, buf);

        Serial.printf("ID: 0x%03lX  LEN: %d  DATA:", rxId, len);
        for (int i = 0; i < len; i++) {
            Serial.printf(" %02X", buf[i]);
        }
        Serial.println();

        // Parse CAN messages based on vehicle type
        switch (ctx->vehicleType) {
            case VEHICLE_BMW:
                if (bmw_ctx != nullptr) {
                    BMW_parseCANMessage(rxId, len, buf, (BMW_CAN_Context_t*)bmw_ctx, ctx->displayUpdated);
                }
                break;
                
            case VEHICLE_KAWASAKI:
                if (kawasaki_data != nullptr) {
                    Kawasaki_parseCANMessage(rxId, len, buf, (Kawasaki_CAN_Data_t*)kawasaki_data, ctx->displayUpdated);
                }
                break;
                
            case VEHICLE_UNKNOWN:
            default:
                // For unknown vehicle type, try both parsers
                if (bmw_ctx != nullptr) {
                    BMW_parseCANMessage(rxId, len, buf, (BMW_CAN_Context_t*)bmw_ctx, ctx->displayUpdated);
                }
                if (kawasaki_data != nullptr) {
                    Kawasaki_parseCANMessage(rxId, len, buf, (Kawasaki_CAN_Data_t*)kawasaki_data, ctx->displayUpdated);
                }
                break;
        }
    }
}
