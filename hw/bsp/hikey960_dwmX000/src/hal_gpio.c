#include "SYSFS.h"
#include <syscfg/syscfg.h>
#include <hal/hal_gpio.h>

int
hal_gpio_init_in(int pin, hal_gpio_pull_t pull)
{
    /* Dummy definition */
    return 0;
}

int
hal_gpio_irq_init(int pin, hal_gpio_irq_handler_t handler, void *arg,
                  hal_gpio_irq_trig_t trig, hal_gpio_pull_t pull)
{
    /* Dummy definition */
    return 0;
}

void hal_gpio_irq_enable(int pin)
{
    /* Dummy definition */
}

void hal_gpio_irq_disable(int pin)
{
    /* Dummy definition */
}

int
hal_gpio_init_out(int pin, int val)
{
    hal_gpio_write(pin, val);
    return 0;
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
    int fd=-1,ret=0, rv;
    char buffer[5] = {0};

    char *path = "";
    switch(pin) {
    case (MYNEWT_VAL(DW3000_DEVICE_0_IRQ)):
        path = IRQ_PATH;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_0_RST)):
        path = RESET_PATH;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_0_EN)):
        path = ENABLE_PATH;
        break;
    default:
        return 0;
    }

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open the file...");
        return errno;
    }
    ret=read(fd, buffer, 4);
    if(ret==-1) {
        perror("read:");
        close(fd);
        return 0;
    }
    rv = strtol(buffer, 0, 10);

    close(fd);
    return rv;
}

void
hal_gpio_write(int pin, int val)
{
    int fd=-1,ret=0;

    char *path = "";
    switch(pin) {
    case (MYNEWT_VAL(DW3000_DEVICE_0_IRQ)):
        path = IRQ_PATH;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_0_RST)):
        path = RESET_PATH;
        break;
    case (MYNEWT_VAL(DW3000_DEVICE_0_EN)):
        path = ENABLE_PATH;
        break;
    default:
        return;
    }

    fd = open(path, O_RDWR);
    if (fd < 0) {
        perror("Failed to open the file...");
        return;
    }

    if (val == 1) {
        ret=write(fd, "1", 4);
    } else if (val == 0) {
        ret=write(fd, "0", 4);
    } else {
        printf("Invalid Value\n");
        close(fd);
        return;
    }
    if(ret==-1) {
        perror("write: ");
        return;
    }

    close(fd);
    return;
}
