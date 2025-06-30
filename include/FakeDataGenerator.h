#ifndef FAKE_DATA_GENERATOR_H
#define FAKE_DATA_GENERATOR_H

#include "BMW_CAN.h"
#include "Kawasaki_CAN.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void FakeDataGenerator_updateBMW(BMW_CAN_Context_t* bmw_ctx);
void FakeDataGenerator_updateKawasaki(Kawasaki_CAN_Data_t* kawasaki_data);

#ifdef __cplusplus
}
#endif

#endif // FAKE_DATA_GENERATOR_H 