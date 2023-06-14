#include "hdc1080.h"

void hdc1080_set_registers(hdc1080_sensor_t* dev, hdc1080_registers_t registers) {
    uint8_t reg = HDC1080_CONFIGURATION;
    if(i2c_slave_write(dev->bus, dev->address, &reg, (uint8_t*) &(registers.raw), sizeof(registers.raw))) {
    }
    vTaskDelay(10/portTICK_PERIOD_MS);
}

hdc1080_sensor_t* hdc1080_init_sensor (uint8_t bus, uint8_t addr) {
    hdc1080_sensor_t* dev;

    if ((dev = malloc (sizeof(hdc1080_sensor_t))) == NULL) {
        return NULL;
    }

    // init sensor data structure
    dev->bus  = bus;
    dev->address = addr;
    
    //setup resolution for temp and hum
    hdc1080_registers_t reg;

	reg.humidity_bitwidth = hdc1080_14bit;
	reg.temperature_bitwidth = hdc1080_14bit;

	hdc1080_set_registers(dev, reg);
    return dev;
}

bool _hdc1080_read_data(hdc1080_sensor_t* dev, uint8_t reg, uint8_t* data, uint32_t len, TickType_t delay) {
    if (!dev) return 0;

    if(i2c_slave_write(dev->bus, dev->address, &reg, NULL, 0)) {
        return false;
    }

    vTaskDelay(delay/portTICK_PERIOD_MS);

    if(i2c_slave_read(dev->bus, dev->address, NULL, data, len)) {
        return false;
    }

    return true;
}

uint16_t _hdc1080_read(hdc1080_sensor_t* dev, uint8_t reg) {
    uint8_t result[2];
    if(!_hdc1080_read_data(dev, reg, (uint8_t*) &result, 2, 10)) {
        return 0;
    }
    return  (uint16_t) (result[0] <<8) | result[1];
}

hdc1080_registers_t hdc1080_get_registers(hdc1080_sensor_t* sensor) {
    hdc1080_registers_t reg;
	reg.raw = _hdc1080_read(sensor, HDC1080_CONFIGURATION);
	return reg;
}	

bool hdc1080_read(hdc1080_sensor_t* sensor, float* temperature, float* humidity) {
    uint8_t raw[4];
    uint16_t result;
    if(!_hdc1080_read_data(sensor, HDC1080_TEMPERATURE, (uint8_t*) &raw, 4, 20)) {
        return false;
    }
    result = raw[0]<<8 | raw[1];
    *temperature = RAW2TEMP(result);
    result = raw[2]<<8 | raw[3];
    *humidity = RAW2HUM(result);
    return true;
}