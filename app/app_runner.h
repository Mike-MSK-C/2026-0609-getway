#ifndef APP_RUNNER_H_
#define APP_RUNNER_H_
#include "app_device.h"
#include "unistd.h"
#include "stdlib.h"
#include "app_bt.h"
#include "signal.h"
#include "app_runner.h"
#define DEVICE_FILE "/dev/ttyS1"


int app_runner_init(void);

#endif // APP_RUNNER_H_