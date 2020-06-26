#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>

#include "SYSFS.h"
#include <syscfg/syscfg.h>
#include <hal/hal_spi.h>

static int uwb_fd=-1;
static hal_spi_txrx_cb txrx_cb_func = 0;
static void           *txrx_cb_arg = 0;

int
hal_spi_set_txrx_cb(int spi_num, hal_spi_txrx_cb txrx_cb, void *arg)
{
    txrx_cb_func = txrx_cb;
    txrx_cb_arg = arg;
    return 0;
}

int
hal_spi_init(int spi_num, void *cfg, uint8_t spi_type)
{
    uwb_fd = open(UWBHAL_FILE, O_RDWR);
    if (uwb_fd < 0) {
        perror("Failed to open the file...");
        return 1;
    }
    return 0;
}

int
hal_spi_config(int spi_num, struct hal_spi_settings *settings)
{
    int fd,ret;
    char buf[16]={'\0'};
    fd = open(SPI_FREQ, O_RDWR);
    if (fd < 0) {
        perror("Failed to open the file...");
        return errno;
    }

    /* x1000 because hal_spi_settings assumes baudrate in kHz */
    sprintf(buf, "%d", settings->baudrate * 1000);
    ret=write(fd, buf , strlen(buf));

    if(ret==-1)
    {
        perror("write: ");
        return -1;
    }
    close(fd);
    return 0;
}

int
hal_spi_enable(int spi_num)
{
    return 0;
}

int
hal_spi_disable(int spi_num)
{
    return 0;
}

int
hal_spi_txrx(int spi_num, void *txbuf, void *rxbuf, int cnt)
{
    int ret;
    ret=write(uwb_fd, (uint8_t*)txbuf, cnt);
    if (rxbuf) {
        ret=read(uwb_fd, (uint8_t*)rxbuf, cnt);
    }

    if(ret==-1) {
        perror("read:");
        return 1;
    }
    return 0;
}

int
hal_spi_txrx_noblock(int spi_num, void *txbuf, void *rxbuf, int cnt)
{
    int ret = hal_spi_txrx(spi_num, txbuf, rxbuf, cnt);
    if (txrx_cb_func) {
        txrx_cb_func(txrx_cb_arg, cnt);
    }
    return ret;
}

int
hal_spi_deinit(int spi_num)
{
    close(uwb_fd);
    return 0;
}
