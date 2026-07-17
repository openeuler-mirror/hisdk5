#ifndef MICRO_LOG_PROCFS_CMD_H_
#define MICRO_LOG_PROCFS_CMD_H_

#include <linux/types.h>

int micro_log_procfs_init(void *hwdev);
void micro_log_procfs_exit(void);

#endif /* MICRO_LOG_PROCFS_CMD_H_ */
