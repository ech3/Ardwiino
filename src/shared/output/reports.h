#pragma once
#include "config/defines.h"
#include "config/eeprom.h"
#include "controller/controller.h"
#include "controller_structs.h"
#include "stdint.h"




void (*fillReport)(void *ReportData, uint16_t *const ReportSize,
                      Controller_t* controller);
void initReports(void);