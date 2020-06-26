#ifndef _UWBHAL_H_
#define _UWBHAL_H_

#include <linux/kernel.h>
#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <linux/uio_driver.h>
#include <linux/pm_qos.h>

#define UWBHAL_BUF_LEN (1032)
typedef void (*uwbhal_irq_cb_fn)(void *arg);

//! Structure of uwbhal_data
struct uwbhal_data {
	struct spi_device *spi;
	struct device *dev;
	struct mutex spilock;
	struct mutex activelock;

    int index;
    bool initialized;               //!< Flag that's only set once init is complete
    char name[16];                  //!< device name used
	int irq_gpio;                   //!< Gpio for Interrupts from device
	int reset_gpio  ;               //!< Gpio for hard resetting the device
	unsigned long reset_flags;      //!< Gpio flags for reset pin, indicates if reset needs to be driven high

    struct pm_qos_request qos;

    struct uio_info *uio;
	int irq;
	u8 irq_state;
	atomic_t is_enable;

    uwbhal_irq_cb_fn irq_direct_cb; //!< Callback called from interrupt context on irq received from device
    void* irq_direct_cb_arg;        //!< Argument used in irq_direct_cb

    u8 state;
	u8 *tx_buf;
	u8 *rx_buf;

	dev_t devt;
	struct cdev cdev;
	struct class *class;
};

enum uwbhal_state_t {
	UWBHAL_STATE_POWER_OFF,
	UWBHAL_STATE_POWER_ON,
	UWBHAL_STATE_ENABLED,
	UWBHAL_STATE_DISABLED,
	UWBHAL_STATE_INVALID,
};

enum uwbhal_irq_state_t {
	UWBHAL_IRQ_STATE_DIS,
	UWBHAL_IRQ_STATE_EN,
};

enum uwbhal_gpio_t {
    UWBHAL_GPIO_INVALID = 0,
    UWBHAL_GPIO_RST,
    UWBHAL_GPIO_IRQ,
    UWBHAL_GPIO_EN,
    UWBHAL_GPIO_WAKE,
};

struct uwbhal_data* uwbhal_get_instance(int idx);
int uwbhal_spi_transfer(struct uwbhal_data *uwbhal, u8 *tx_buf, u8 *rx_buf, int len);
int uwbhal_spi_freq(struct uwbhal_data *uwbhal, int freq);
int uwbhal_enable(struct uwbhal_data *uwbhal);
void uwbhal_disable(struct uwbhal_data *uwbhal);

int uwbhal_gpio_in(struct uwbhal_data *uwbhal, int gpio);
int uwbhal_gpio_out(struct uwbhal_data *uwbhal, int gpio, int val);
int uwbhal_gpio_read(struct uwbhal_data *uwbhal, int gpio);
void uwbhal_gpio_write(struct uwbhal_data *uwbhal, int gpio, int value);

int uwbhal_irq_cb_set(struct uwbhal_data *uwbhal, uwbhal_irq_cb_fn cb, void *arg);

#endif
