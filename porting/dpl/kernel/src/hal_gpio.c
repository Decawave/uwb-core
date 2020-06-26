#include <dpl/dpl.h>
#include <dpl/dpl_tasks.h>
#include <hal/hal_gpio.h>
#include <linux/uwb-hal.h>

int
hal_gpio_init_in(int pin, hal_gpio_pull_t pull)
{
    int rc, gpio, inst_idx=0;
    struct uwbhal_data* inst = 0;

    switch(pin) {
    case (MYNEWT_VAL(DW3000_DEVICE_0_IRQ)):
        inst_idx = 0;
        gpio = UWBHAL_GPIO_IRQ;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_0_RST)):
        inst_idx = 0;
        gpio = UWBHAL_GPIO_RST;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_0_EN)):
        inst_idx = 0;
        gpio = UWBHAL_GPIO_EN;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_1_IRQ)):
        inst_idx = 1;
        gpio = UWBHAL_GPIO_IRQ;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_1_RST)):
        inst_idx = 1;
        gpio = UWBHAL_GPIO_RST;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_1_EN)):
        inst_idx = 1;
        gpio = UWBHAL_GPIO_EN;
        break;
    default:
        return -1;
    }

    inst = uwbhal_get_instance(inst_idx);
    if (!IS_ERR_OR_NULL(inst)) {
        rc = uwbhal_gpio_in(inst, gpio);
    } else {
        rc = -EINVAL;
    }
    return rc;
}

int
hal_gpio_irq_init(int pin, hal_gpio_irq_handler_t handler, void *arg,
                  hal_gpio_irq_trig_t trig, hal_gpio_pull_t pull)
{
    int rc, inst_idx=0;
    struct uwbhal_data* inst = 0;

    switch(pin) {
    case (MYNEWT_VAL(DW3000_DEVICE_0_IRQ)):
        inst_idx = 0;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_1_IRQ)):
        inst_idx = 1;
        break;
    default:
        return -1;
    }

    inst = uwbhal_get_instance(inst_idx);
    if (!IS_ERR_OR_NULL(inst)) {
        rc = uwbhal_irq_cb_set(inst, handler, arg);
    } else {
        rc = -EINVAL;
    }
    return rc;
}

void
hal_gpio_irq_release(int pin)
{
    int rc, inst_idx=0;
    struct uwbhal_data* inst = 0;

    switch(pin) {
    case (MYNEWT_VAL(DW3000_DEVICE_0_IRQ)):
        inst_idx = 0;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_1_IRQ)):
        inst_idx = 1;
        break;
    default:
        return;
    }

    inst = uwbhal_get_instance(inst_idx);
    if (!IS_ERR_OR_NULL(inst)) {
        rc = uwbhal_irq_cb_set(inst, 0, 0);
    }

    return;
}

void hal_gpio_irq_enable(int pin)
{
    /* Dummy definition */
    pr_info("%s:%s:%d: unimplemented", __FILE__, __func__, __LINE__);
}

void hal_gpio_irq_disable(int pin)
{
    /* Dummy definition */
    pr_info("%s:%s:%d: unimplemented", __FILE__, __func__, __LINE__);
}

int
hal_gpio_init_out(int pin, int val)
{
    int rc, gpio, inst_idx=0;
    struct uwbhal_data* inst = 0;

    switch(pin) {
    case (MYNEWT_VAL(DW3000_DEVICE_0_IRQ)):
        inst_idx = 0;
        gpio = UWBHAL_GPIO_IRQ;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_0_RST)):
        inst_idx = 0;
        gpio = UWBHAL_GPIO_RST;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_0_EN)):
        inst_idx = 0;
        gpio = UWBHAL_GPIO_EN;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_1_IRQ)):
        inst_idx = 1;
        gpio = UWBHAL_GPIO_IRQ;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_1_RST)):
        inst_idx = 1;
        gpio = UWBHAL_GPIO_RST;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_1_EN)):
        inst_idx = 1;
        gpio = UWBHAL_GPIO_EN;
        break;
    default:
        return -1;
    }

    inst = uwbhal_get_instance(inst_idx);
    if (!IS_ERR_OR_NULL(inst)) {
        rc = uwbhal_gpio_out(inst, gpio, val);
    } else {
        rc = -EINVAL;
    }
    return rc;
}

int
hal_gpio_toggle(int pin)
{
    int pin_state = (hal_gpio_read(pin) != 1);
    hal_gpio_write(pin, pin_state);
    return pin_state;
}

int hal_gpio_read(int pin)
{
    int rc, gpio, inst_idx=0;
    struct uwbhal_data* inst = 0;

    switch(pin) {
    case (MYNEWT_VAL(DW3000_DEVICE_0_IRQ)):
        inst_idx = 0;
        gpio = UWBHAL_GPIO_IRQ;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_0_RST)):
        inst_idx = 0;
        gpio = UWBHAL_GPIO_RST;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_0_EN)):
        inst_idx = 0;
        gpio = UWBHAL_GPIO_EN;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_1_IRQ)):
        inst_idx = 1;
        gpio = UWBHAL_GPIO_IRQ;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_1_RST)):
        inst_idx = 1;
        gpio = UWBHAL_GPIO_RST;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_1_EN)):
        inst_idx = 1;
        gpio = UWBHAL_GPIO_EN;
        break;
    default:
        return -1;
    }

    inst = uwbhal_get_instance(inst_idx);
    if (!IS_ERR_OR_NULL(inst)) {
        rc = uwbhal_gpio_read(inst, gpio);
    } else {
        rc = -EINVAL;
    }
    return rc;
}

void
hal_gpio_write(int pin, int val)
{
    int gpio, inst_idx=0;
    bool is_enable = false;
    struct uwbhal_data* inst = 0;

    switch(pin) {
    case (MYNEWT_VAL(DW3000_DEVICE_0_IRQ)):
        inst_idx = 0;
        gpio = UWBHAL_GPIO_IRQ;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_0_RST)):
        inst_idx = 0;
        gpio = UWBHAL_GPIO_RST;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_0_EN)):
        inst_idx = 0;
        gpio = UWBHAL_GPIO_EN;
        is_enable = true;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_1_IRQ)):
        inst_idx = 1;
        gpio = UWBHAL_GPIO_IRQ;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_1_RST)):
        inst_idx = 1;
        gpio = UWBHAL_GPIO_RST;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_1_EN)):
        inst_idx = 1;
        gpio = UWBHAL_GPIO_EN;
        is_enable = true;
        break;
    default:
        return;
    }

    inst = uwbhal_get_instance(inst_idx);
    if (IS_ERR_OR_NULL(inst)) {
        return;
    }

    if (is_enable) {
        if (val) {
            uwbhal_enable(inst);
        } else {
            uwbhal_disable(inst);
        }
    } else {
        uwbhal_gpio_write(inst, gpio, val);
    }

    return;
}
