#include "sensors.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(SENSORS, CONFIG_SENSORS_LOG_LEVEL);

const struct device *dev_bmi270 = DEVICE_DT_GET(DT_ALIAS(accel0));
const struct device *dev_adxl367 = DEVICE_DT_GET(DT_ALIAS(accel1));
const struct device *dev_bme680 = DEVICE_DT_GET(DT_ALIAS(env0));
// const struct device *dev_bmm350 = DEVICE_DT_GET(DT_ALIAS(mag0)); // NOTE: The bmm350 device have
// no zephyr drivers yet

uint64_t sample_count = 0; // Counter for the measurements

int sensors_rotate_measurement(struct sensor_value *data, int x, int y, int z)
{
	double x_rad = x * PI / 180.0;
	double y_rad = y * PI / 180.0;
	double z_rad = z * PI / 180.0;

	double cos_x = cos(x_rad);
	double sin_x = sin(x_rad);
	double cos_y = cos(y_rad);
	double sin_y = sin(y_rad);
	double cos_z = cos(z_rad);
	double sin_z = sin(z_rad);

	double x_val = sensor_value_to_double(&data[0]);
	double y_val = sensor_value_to_double(&data[1]);
	double z_val = sensor_value_to_double(&data[2]);

	// Rotation around x-axis
	double y1 = cos_x * y_val - sin_x * z_val;
	double z1 = sin_x * y_val + cos_x * z_val;

	// Rotation around y-axis
	double x2 = cos_y * x_val + sin_y * z1;
	double z2 = -sin_y * x_val + cos_y * z1;

	// Rotation around z-axis
	double x3 = cos_z * x2 - sin_z * y1;
	double y3 = sin_z * x2 + cos_z * y1;

	sensor_value_from_double(&data[0], x3);
	sensor_value_from_double(&data[1], y3);
	sensor_value_from_double(&data[2], z2);

	return 0;
}

int sensors_init(void)
{
	int ret;
	//////////////////////BMI270//////////////////////

	if (!device_is_ready(dev_bmi270)) {
		LOG_ERR("Device %s is not ready", dev_bmi270->name);
		return -1;
	}
	LOG_INF("Device %s is ready", dev_bmi270->name);

	struct sensor_value ful_scale, sampling_freq, oversampling;
	ful_scale.val1 = 2; /* G */
	ful_scale.val2 = 0;
	sampling_freq.val1 = 100; /* Hz */
	sampling_freq.val2 = 0;
	oversampling.val1 = 1; /* Normal mode */
	oversampling.val2 = 0;

	/* Set sampling frequency last as this also sets the appropriate
	 * power mode. If already sampling, change to 0.0Hz before changing
	 * other attributes
	 */
	sensor_attr_set(dev_bmi270, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE, &ful_scale);
	sensor_attr_set(dev_bmi270, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_OVERSAMPLING, &oversampling);
	sensor_attr_set(dev_bmi270, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY,
			&sampling_freq);

	ful_scale.val1 = 500; /* dps */
	ful_scale.val2 = 0;
	sampling_freq.val1 = 100; /* Hz. */
	sampling_freq.val2 = 0;
	oversampling.val1 = 1; /* Normal mode */
	oversampling.val2 = 0;

	/* Set sampling frequency last as this also sets the appropriate
	 * power mode. If already sampling, change to 0.0Hz before changing
	 * other attributes
	 */
	sensor_attr_set(dev_bmi270, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_FULL_SCALE, &ful_scale);
	sensor_attr_set(dev_bmi270, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_OVERSAMPLING, &oversampling);
	sensor_attr_set(dev_bmi270, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY,
			&sampling_freq);

	//////////////////////ADXL367/////////////////////

	if (!device_is_ready(dev_adxl367)) {
		LOG_ERR("Device %s is not ready", dev_adxl367->name);
		return -1;
	}
	LOG_INF("Device %s is ready", dev_adxl367->name);

	struct sensor_value sampling_freq2;
	sampling_freq2.val1 = 100; /* Hz */
	sampling_freq2.val2 = 0;

	ret = sensor_attr_set(dev_adxl367, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY,
			      &sampling_freq2);
	if (ret) {
		LOG_ERR("sensor_attr_set failed ret %d", ret);
		return ret;
	}

	//////////////////////BME680//////////////////////

	if (!device_is_ready(dev_bme680)) {
		LOG_ERR("Device %s is not ready", dev_bme680->name);
		return -1;
	}
	LOG_INF("Device %s is ready", dev_bme680->name);

	// NOTE: The bmm350 device have no zephyr drivers yet
	//////////////////////BMM350//////////////////////
	// if(!device_is_ready(dev_bmm350)) {
	//     LOG_ERR("Device %s is not ready", dev_bmm350->name);
	//     return -1;
	// }
	// LOG_INF("Device %s is ready", dev_bmm350->name);

	return 0;
}

/**
 * @brief Meassure the sensor data
 *
 * @param data Pointer to the data array
 *       [count,                                            \
 *       BMI270_accel_x, BMI270_accel_y, BMI270_accel_z,    \
 *       BMI270_gyro_x, BMI270_gyro_y, BMI270_gyro_z,       \
 *       ADXL367_accel_x, ADXL367_accel_y, ADXL367_accel_z, \
 *       BME680_temp, BME680_press, BME680_hum, BME680_gas, \
 *       BMM350_magn_x, BMM350_magn_y, BMM350_magn_z]
 *
 * All values are in SI units.
 *      Acceleration in m/s^2
 *      Gyroscope in rad/s
 *      Temperature in Celsius
 *      Pressure in Pascal
 *      Humidity in %
 *      Gas resistance in Ohm
 *      Magnetometer in uT // NOTE: The bmm350 device have no zephyr drivers yet, data will be 0
 *
 * Sensor data will be rotated to match the ADXL367 coordinate system.
 *
 * @return 0 if successful, negative error code otherwise.
 */
int sensor_measure(double *data)
{
	int ret;
	struct sensor_value accel0[3], gyr[3];
	struct sensor_value accel1[3];
	struct sensor_value temp, press, hum, gas;
	// struct sensor_value mag[3]; // NOTE: The bmm350 device have no zephyr drivers yet

	//////////////////////BMI270//////////////////////
	ret = sensor_sample_fetch(dev_bmi270);
	if (ret) {
		LOG_ERR("sensor_sample_fetch failed ret %d", ret);
		return -1;
	}

	ret = sensor_channel_get(dev_bmi270, SENSOR_CHAN_ACCEL_XYZ, accel0);
	if (ret) {
		LOG_ERR("sensor_channel_get failed ret %d", ret);
		return -1;
	}

	ret = sensor_channel_get(dev_bmi270, SENSOR_CHAN_GYRO_XYZ, gyr);
	if (ret) {
		LOG_ERR("sensor_channel_get failed ret %d", ret);
		return -1;
	}

	ret = sensors_rotate_measurement(accel0, 0, 180, 90);
	if (ret) {
		LOG_ERR("rotate_measurement failed ret %d", ret);
		return -1;
	}

	ret = sensors_rotate_measurement(gyr, 0, 180, 90);
	if (ret) {
		LOG_ERR("rotate_measurement failed ret %d", ret);
		return -1;
	}

	//////////////////////ADXL367/////////////////////
	ret = sensor_sample_fetch(dev_adxl367);
	if (ret) {
		LOG_ERR("sensor_sample_fetch failed ret %d", ret);
		return -1;
	}

	ret = sensor_channel_get(dev_adxl367, SENSOR_CHAN_ACCEL_XYZ, accel1);
	if (ret) {
		LOG_ERR("sensor_channel_get SENSOR_CHAN_ACCEL_XYZ failed ret %d", ret);
		return -1;
	}

	//////////////////////BME680//////////////////////
	ret = sensor_sample_fetch(dev_bme680);
	if (ret) {
		LOG_ERR("sensor_sample_fetch failed ret %d", ret);
		return -1;
	}

	ret = sensor_channel_get(dev_bme680, SENSOR_CHAN_AMBIENT_TEMP, &temp);
	if (ret) {
		LOG_ERR("sensor_channel_get failed ret %d", ret);
		return -1;
	}

	ret = sensor_channel_get(dev_bme680, SENSOR_CHAN_PRESS, &press);
	if (ret) {
		LOG_ERR("sensor_channel_get failed ret %d", ret);
		return -1;
	}

	ret = sensor_channel_get(dev_bme680, SENSOR_CHAN_HUMIDITY, &hum);
	if (ret) {
		LOG_ERR("sensor_channel_get failed ret %d", ret);
		return -1;
	}

	ret = sensor_channel_get(dev_bme680, SENSOR_CHAN_GAS_RES, &gas);
	if (ret) {
		LOG_ERR("sensor_channel_get failed ret %d", ret);
		return -1;
	}

	// NOTE: The bmm350 device have no zephyr drivers yet
	////////////////////BMM350//////////////////////
	// ret = sensor_sample_fetch(dev_bmm350);
	// if(ret) {
	//     LOG_ERR("sensor_sample_fetch failed ret %d", ret);
	//     return -1;
	// }

	// ret = sensor_channel_get(dev_bmm350, SENSOR_CHAN_MAGN_XYZ, mag);
	// if(ret) {
	//     LOG_ERR("sensor_channel_get failed ret %d", ret);
	//     return -1;
	// }

	// ret = sensor_rotate_measurement(mag, 0, 0, 180);
	// if(ret) {
	//     LOG_ERR("rotate_measurement failed ret %d", ret);
	//     return -1;
	// }
	// mag[2].val1 = -mag[2].val1;   // x = -x
	// }

	sample_count++;

	//////////////////////Set output//////////////////////
	data[0] = sample_count;
	data[1] = sensor_value_to_double(&accel0[0]);
	data[2] = sensor_value_to_double(&accel0[1]);
	data[3] = sensor_value_to_double(&accel0[2]);
	data[4] = sensor_value_to_double(&gyr[0]);
	data[5] = sensor_value_to_double(&gyr[1]);
	data[6] = sensor_value_to_double(&gyr[2]);
	data[7] = sensor_value_to_double(&accel1[0]);
	data[8] = sensor_value_to_double(&accel1[1]);
	data[9] = sensor_value_to_double(&accel1[2]);
	data[10] = sensor_value_to_double(&temp);
	data[11] = sensor_value_to_double(&press);
	data[12] = sensor_value_to_double(&hum);
	data[13] = sensor_value_to_double(&gas);
	// data[14] = sensor_value_to_double(&mag[0]);
	// data[15] = sensor_value_to_double(&mag[1]);
	// data[16] = sensor_value_to_double(&mag[2]);
	data[14] = 0;
	data[15] = 0;
	data[16] = 0;

	return 0;
}

/**
 * @brief Get the sensor data as a JSON string
 *
 * @param buf Pointer to the buffer
 * @param len Length of the buffer
 * @return int Length of the JSON string if successful, negative error code otherwise.
 */
int sensors_get_json(char *buf, size_t len)
{
	int ret;

	const char *sensors_json_template = "{"
					    "\"count\":%d,"
					    "\"bmi270_ax\":%.03f,"
					    "\"bmi270_ay\":%.03f,"
					    "\"bmi270_az\":%.03f,"
					    "\"bmi270_gx\":%.03f,"
					    "\"bmi270_gy\":%.03f,"
					    "\"bmi270_gz\":%.03f,"
					    "\"adxl_ax\":%.03f,"
					    "\"adxl_ay\":%.03f,"
					    "\"adxl_az\":%.03f,"
					    "\"bme680_temperature\":%.03f,"
					    "\"bme680_pressure\":%.03f,"
					    "\"bme680_humidity\":%.03f,"
					    "\"bme680_gas\":%.03f,"
					    "\"bmm350_magn_x\":%.03f,"
					    "\"bmm350_magn_y\":%.03f,"
					    "\"bmm350_magn_z\":%.03f"
					    "}";

	double data[17];
	ret = sensor_measure(data);
	if (ret) {
		LOG_ERR("sensor_measure failed ret %d", ret);
		return ret;
	}

	ret = snprintf(buf, len, sensors_json_template, (int)data[0], data[1], data[2], data[3],
		       data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11],
		       data[12], data[13], data[14], data[15], data[16]);

	// LOG_INF("JSON: %s", buf);

	if (ret >= len) {
		LOG_ERR("Sensor data does not fit in buffer");
		return -ENOSPC;
	}

	return ret;
}