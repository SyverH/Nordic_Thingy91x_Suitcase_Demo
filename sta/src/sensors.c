#include "sensors.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(SENSORS, CONFIG_SENSORS_LOG_LEVEL);

void sensor_gas_thread();

K_THREAD_STACK_DEFINE(gas_stack_area, 1024);
struct k_thread gas_thread_data;
k_tid_t gas_thread_id;

const struct device *dev_bmi270 = DEVICE_DT_GET(DT_ALIAS(accel0));
const struct device *dev_adxl367 = DEVICE_DT_GET(DT_ALIAS(accel1));
const struct device *dev_bme680 = DEVICE_DT_GET(DT_ALIAS(env0));
// const struct device *dev_bmm350 = DEVICE_DT_GET(DT_ALIAS(mag0)); // NOTE: The bmm350 device have
// no zephyr drivers yet

/**
 * @brief Rotate a measurement around an axis.
 *
 * @param val pointer to the sensor value.
 * @param angle angle to rotate in degrees.
 * @param axis axis to rotate around (0 = x, 1 = y, 2 = z).
 *
 * @return int 0 if successful, negative errno otherwise.
 */
int rotate_measurement(struct sensor_value *val, int angle, int axis)
{
	double angle_rad = angle * PI / 180.0;
	double cos_angle = cos(angle_rad);
	double sin_angle = sin(angle_rad);

	double x = sensor_value_to_double(&val[0]);
	double y = sensor_value_to_double(&val[1]);
	double z = sensor_value_to_double(&val[2]);

	double x_new = 0.0;
	double y_new = 0.0;
	double z_new = 0.0;

	if (axis == 0) {
		// Rotate around x-axis
		x_new = x;
		y_new = y * cos_angle - z * sin_angle;
		z_new = y * sin_angle + z * cos_angle;
	} else if (axis == 1) {
		// Rotate around y-axis
		x_new = x * cos_angle + z * sin_angle;
		y_new = y;
		z_new = -x * sin_angle + z * cos_angle;
	} else if (axis == 2) {
		// Rotate around z-axis
		x_new = x * cos_angle - y * sin_angle;
		y_new = x * sin_angle + y * cos_angle;
		z_new = z;
	} else {
		LOG_ERR("Invalid axis for rotation");
		return -EINVAL;
	}

	sensor_value_from_double(&val[0], x_new);
	sensor_value_from_double(&val[1], y_new);
	sensor_value_from_double(&val[2], z_new);

	return 0;
}

/**
 * @brief Initialize the sensors
 *
 * @return int 0 if successful, negative error code otherwise.
 */
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
	sampling_freq.val1 = 200; /* Hz */
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

	ful_scale.val1 = 1000; /* dps */
	ful_scale.val2 = 0;
	sampling_freq.val1 = 200; /* Hz. */
	sampling_freq.val2 = 0;
	oversampling.val1 = 2; /* Normal mode */
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

    // Start the gas sensor thread
    
    
	gas_thread_id =
    k_thread_create(&gas_thread_data, gas_stack_area,
            K_THREAD_STACK_SIZEOF(gas_stack_area), sensor_gas_thread,
            NULL, NULL, NULL, 7, 0, K_NO_WAIT);

	return 0;
}

struct sensor_value temp, press, hum, gas;
// Thread to measure the gas sensor continuously and update a global variable as the gas measurement are slow
void sensor_gas_thread(){
    int ret;
    while(1) {
        //////////////////////BME680//////////////////////
        LOG_DBG("BME680");
        ret = sensor_sample_fetch(dev_bme680);
        if (ret) {
            LOG_ERR("sensor_sample_fetch failed ret %d", ret);
            return;
        }

        ret = sensor_channel_get(dev_bme680, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        if (ret) {
            LOG_ERR("sensor_channel_get failed ret %d", ret);
            return;
        }

        ret = sensor_channel_get(dev_bme680, SENSOR_CHAN_PRESS, &press);
        if (ret) {
            LOG_ERR("sensor_channel_get failed ret %d", ret);
            return;
        }

        ret = sensor_channel_get(dev_bme680, SENSOR_CHAN_HUMIDITY, &hum);
        if (ret) {
            LOG_ERR("sensor_channel_get failed ret %d", ret);
            return;
        }

        ret = sensor_channel_get(dev_bme680, SENSOR_CHAN_GAS_RES, &gas);
        if (ret) {
            LOG_ERR("sensor_channel_get failed ret %d", ret);
            return;
        }

        k_sleep(K_MSEC(250));
    }

}

/**
 * @brief Meassure the sensor data
 *
 * @param data Pointer to the data array
 *       [timestamp,                                            \
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
 * @return 0 if successful, negative error code otherwise.
 */
int sensor_measure(double *data)
{
	int ret;
	struct sensor_value accel0[3], gyr[3];
	struct sensor_value accel1[3];
	// struct sensor_value temp, press, hum, gas;
	// struct sensor_value mag[3]; // NOTE: The bmm350 device have no zephyr drivers yet

	//////////////////////BMI270//////////////////////
	LOG_DBG("BMI270");
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

	// Rotate the BMI270 data to match the orientation of the thingy
	ret = rotate_measurement(accel0, 180, 2);
	if (ret) {
		LOG_ERR("rotate_measurement failed ret %d", ret);
		return -1;
	}

	ret = rotate_measurement(gyr, 180, 2);
	if (ret) {
		LOG_ERR("rotate_measurement failed ret %d", ret);
		return -1;
	}

	//////////////////////ADXL367/////////////////////
	LOG_DBG("ADXL367");
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

	// //////////////////////BME680//////////////////////
	// LOG_DBG("BME680");
	// ret = sensor_sample_fetch(dev_bme680);
	// if (ret) {
	// 	LOG_ERR("sensor_sample_fetch failed ret %d", ret);
	// 	return -1;
	// }

	// ret = sensor_channel_get(dev_bme680, SENSOR_CHAN_AMBIENT_TEMP, &temp);
	// if (ret) {
	// 	LOG_ERR("sensor_channel_get failed ret %d", ret);
	// 	return -1;
	// }

	// ret = sensor_channel_get(dev_bme680, SENSOR_CHAN_PRESS, &press);
	// if (ret) {
	// 	LOG_ERR("sensor_channel_get failed ret %d", ret);
	// 	return -1;
	// }

	// ret = sensor_channel_get(dev_bme680, SENSOR_CHAN_HUMIDITY, &hum);
	// if (ret) {
	// 	LOG_ERR("sensor_channel_get failed ret %d", ret);
	// 	return -1;
	// }

	// ret = sensor_channel_get(dev_bme680, SENSOR_CHAN_GAS_RES, &gas);
	// if (ret) {
	// 	LOG_ERR("sensor_channel_get failed ret %d", ret);
	// 	return -1;
	// }

    // current_time = k_cycle_get_32();
    // diff = current_time - measure_start_time;
    // LOG_INF("BME680: %d us", k_cyc_to_us_floor32(diff));

	// NOTE: The bmm350 device have no zephyr drivers yet
	////////////////////BMM350//////////////////////
	// LOG_DBG("BMM350");
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

	LOG_DBG("Sensor data fetched");
	float32_t runtime = (float32_t)k_cycle_get_32() / (float32_t)sys_clock_hw_cycles_per_sec();

	// LOG_INF("Runtime: %f", runtime);

	//////////////////////Set output//////////////////////
	LOG_DBG("Set output");
	data[0] = runtime;
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
	data[14] = 0.0;
	data[15] = 0.0;
	data[16] = 0.0;

	return 0;
}

/**
 * @brief Get the sensor data as a JSON string
 *      The JSON string will look like this:
 *      { "timestamp": 0.0, "bmi270_ax": 0.0, "bmi270_ay": 0.0, "bmi270_az": 0.0, "bmi270_gx": 0.0,
 *      "bmi270_gy": 0.0, "bmi270_gz": 0.0, "adxl_ax": 0.0, "adxl_ay": 0.0, "adxl_az": 0.0,
 *      "bme680_temperature": 0.0, "bme680_pressure": 0.0, "bme680_humidity": 0.0, "bme680_gas": 0.0,
 *      "bmm350_magn_x": 0.0, "bmm350_magn_y": 0.0, "bmm350_magn_z": 0.0 }
 *
 * @param buf Pointer to the buffer
 * @param len Length of the buffer
 * @return int Length of the JSON string if successful, negative error code otherwise.
 */
int sensors_get_json(char *buf, size_t len)
{
	int ret;

	const char *sensors_json_template = "{"
					    "\"timestamp\":%.06f,"
					    "\"bmi270_ax\":%.06f,"
					    "\"bmi270_ay\":%.06f,"
					    "\"bmi270_az\":%.06f,"
					    "\"bmi270_gx\":%.06f,"
					    "\"bmi270_gy\":%.06f,"
					    "\"bmi270_gz\":%.06f,"
					    "\"adxl_ax\":%.06f,"
					    "\"adxl_ay\":%.06f,"
					    "\"adxl_az\":%.06f,"
					    "\"bme680_temperature\":%.03f,"
					    "\"bme680_pressure\":%.03f,"
					    "\"bme680_humidity\":%.03f,"
					    "\"bme680_gas\":%.03f,"
					    "\"bmm350_magn_x\":%.03f,"
					    "\"bmm350_magn_y\":%.03f,"
					    "\"bmm350_magn_z\":%.03f"
					    "}";

	LOG_DBG("Getting sensor data");

	double data[NUM_SENSOR_MEASUREMENTS];
	ret = sensor_measure(data);
	if (ret) {
		LOG_ERR("sensor_measure failed ret %d", ret);
		return ret;
	}

	LOG_DBG("Got sensor data");

	ret = snprintf(buf, len, sensors_json_template, data[0], data[1], data[2], data[3], data[4],
		       data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12],
		       data[13], data[14], data[15], data[16]);

	LOG_DBG("JSON-ified sensor data");
	// LOG_INF("JSON: %s", buf);

	if (ret >= len) {
		LOG_ERR("Sensor data does not fit in buffer");
		return -ENOSPC;
	}

	return ret;
}