#include<stdio.h>
#include<stdint.h>
#include<string.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<unistd.h>

#define UWBHAL_FILE  "/dev/uwbhal0"
#define ENABLE_PATH  "/sys/class/uwbhal0/uwbhal0/device/enable"
#define RESET_PATH   "/sys/class/uwbhal0/uwbhal0/device/reset"
#define IRQ_PATH     "/sys/class/uwbhal0/uwbhal0/device/irq"
#define SPI_FREQ     "/sys/class/uwbhal0/uwbhal0/device/spi_freq"
