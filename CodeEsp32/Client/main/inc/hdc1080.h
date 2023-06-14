#include "stdio.h"
#include "i2c_lib.h"

#define	HDC1080_TEMPERATURE		0x00
#define HDC1080_HUMIDITY		0x01
#define HDC1080_CONFIGURATION	0x02
#define HDC1080_SERIAL_ID_FIRST	0xFB
#define HDC1080_SERIAL_ID_MID	0xFC
#define HDC1080_SERIAL_ID_LAST	0xFD
#define HDC1080_MANUFACTURER_ID 0xFE
#define HDC1080_DEVICE_ID		0xFF

#define HDC1080_ADDR 0x40

typedef struct {
    uint8_t bus;
    uint8_t address;
} hdc1080_sensor_t;

typedef enum {
	hdc1080_8bit = 0x02,
	hdc1080_11bit = 0x01,
	hdc1080_14bit = 0x00
} hdc1080_measurement_bitwidth_t;

typedef union {
	uint16_t raw;
	struct {
		uint8_t humidity_bitwidth : 2;
		uint8_t temperature_bitwidth : 1;
		uint8_t battery_status : 1;
		uint8_t acquisition_mode : 1;
		uint8_t heater : 1;
		uint8_t reserved_again : 1;
		uint8_t software_reset : 1;
        uint8_t unused;
	};
} hdc1080_registers_t;

#define RAW2HUM(raw) (((float)raw) * 100/65536)
#define RAW2TEMP(raw) (((float)raw) * 165/65536 - 40)

hdc1080_sensor_t* hdc1080_init_sensor (uint8_t bus, uint8_t addr);

hdc1080_registers_t hdc1080_get_registers(hdc1080_sensor_t* sensor);	

void hdc1080_set_registers(hdc1080_sensor_t* sensor, hdc1080_registers_t registers);

void hdc1080_set_resolution(hdc1080_sensor_t* sensor, hdc1080_measurement_bitwidth_t humidity, hdc1080_measurement_bitwidth_t temperature);

bool hdc1080_read(hdc1080_sensor_t* sensor, float* temperature, float* humidity);
