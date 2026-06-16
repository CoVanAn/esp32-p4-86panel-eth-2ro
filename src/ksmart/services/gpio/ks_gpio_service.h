#ifndef KS_GPIO_SERVICE_H
#define KS_GPIO_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

int set_gpio(int gpio_pin, int val);
int gpio_export(int gpio_pin);
int gpio_out_direction(int gpio_pin);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
