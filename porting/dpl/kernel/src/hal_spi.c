#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <stdint.h>

#include <syscfg/syscfg.h>
#include <hal/hal_spi.h>
#include <linux/uwb-hal.h>

#define UWBHAL_MAX_INSTANCES (2)

static hal_spi_txrx_cb txrx_cb_func[UWBHAL_MAX_INSTANCES] = {0};
static void           *txrx_cb_arg[UWBHAL_MAX_INSTANCES] = {0};

int
hal_spi_set_txrx_cb(int spi_num, hal_spi_txrx_cb txrx_cb, void *arg)
{
    txrx_cb_func[spi_num] = txrx_cb;
    txrx_cb_arg[spi_num] = arg;
    return 0;
}

int
hal_spi_init(int spi_num, void *cfg, uint8_t spi_type)
{
    return uwbhal_enable(uwbhal_get_instance(spi_num));
}

int
hal_spi_config(int spi_num, struct hal_spi_settings *settings)
{
    /* x1000 because hal_spi_settings assumes baudrate in kHz */
    return uwbhal_spi_freq(uwbhal_get_instance(spi_num), settings->baudrate * 1000);
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
    return uwbhal_spi_transfer(uwbhal_get_instance(spi_num), txbuf, rxbuf, cnt);
}

int
hal_spi_txrx_noblock(int spi_num, void *txbuf, void *rxbuf, int cnt)
{
    int ret = uwbhal_spi_transfer(uwbhal_get_instance(spi_num), txbuf, rxbuf, cnt);
    if (txrx_cb_func[spi_num]) {
        txrx_cb_func[spi_num](txrx_cb_arg[spi_num], cnt);
    }
    return ret;
}

int
hal_spi_deinit(int spi_num)
{
    return 0;
}
