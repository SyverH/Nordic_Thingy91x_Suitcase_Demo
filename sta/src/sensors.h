#pragma once

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <math.h>

#define GRAVITY 9.80665
#define PI      3.14159265359

// Number of sensor measurements (timestamp, 6x bmi270, 3x adxl367, 4x bme680, 3x bmm350)
#define GYRO_OFFSET_SAMPLES 500
#define NUM_SENSOR_MEASUREMENTS 17

int sensors_init(void);
int sensors_measure(void);
int sensor_offset_calibration(double *offset_data);
int sensor_apply_offset_calibration(double *data, double *offset_data);
int sensors_get_json(char *buf, size_t len, double *offset_data);
int sensor_rotate_measurement(struct sensor_value *data, int x, int y, int z);