#include "stdint.h"
#include "driver/i2c.h"

#define I2C_BUS       0
#define I2C_SCL_PIN   22
#define I2C_SDA_PIN   21
#define I2C_FREQ      100000

#define I2C_ACK_VAL  0x0
#define I2C_NACK_VAL 0x1

void i2c_init (int bus, gpio_num_t scl, gpio_num_t sda, uint32_t freq);

int i2c_slave_write (uint8_t bus, uint8_t addr, const uint8_t *reg, 
                     uint8_t *data, uint32_t len);

int i2c_slave_read (uint8_t bus, uint8_t addr, const uint8_t *reg, 
                    uint8_t *data, uint32_t len);
