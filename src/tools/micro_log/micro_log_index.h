#ifndef MICRO_LOG_INDEX_H_
#define MICRO_LOG_INDEX_H_

#include "micro_log_comm.h"

#define FW_TILE_DATA_INDEX 0x5
int mirco_log_get_sim_data_from_flash(void *hwdev, struct micro_log_info *log_info);
 
#endif