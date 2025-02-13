#include "sensors.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(SENSORS, CONFIG_SENSORS_LOG_LEVEL);

const struct device *dev_bmi270 = DEVICE_DT_GET(DT_ALIAS(accel0));
const struct device *dev_adxl367 = DEVICE_DT_GET(DT_ALIAS(accel1));
const struct device *dev_bme680 = DEVICE_DT_GET(DT_ALIAS(env0));
const struct device *dev_bmm350 = DEVICE_DT_GET(DT_ALIAS(mag0)); // NOTE: The bmm350 device have
// no zephyr drivers yet

uint64_t sample_count = 0; // Counter for the measurements

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

int sensors_correct_magnetometer(struct sensor_value *data)
{

    zsl_real_t data_vec[3] = {sensor_value_to_double(&data[0]),
                               sensor_value_to_double(&data[1]),
                               sensor_value_to_double(&data[2])};
    zsl_real_t b_data[3] = {DEFAULT_MAG_CAL_BX, DEFAULT_MAG_CAL_BY, DEFAULT_MAG_CAL_BZ};
    zsl_real_t K_data[9] = {DEFAULT_MAG_CAL_K0, DEFAULT_MAG_CAL_K1, DEFAULT_MAG_CAL_K2,
                             DEFAULT_MAG_CAL_K3, DEFAULT_MAG_CAL_K4, DEFAULT_MAG_CAL_K5,
                             DEFAULT_MAG_CAL_K6, DEFAULT_MAG_CAL_K7, DEFAULT_MAG_CAL_K8};

    ZSL_VECTOR_DEF(d, 3);
    ZSL_VECTOR_DEF(b, 3);
    ZSL_MATRIX_DEF(K, 3, 3);
    ZSL_VECTOR_DEF(d_out, 3);

    zsl_vec_from_arr(&d, data_vec);
    zsl_vec_from_arr(&b, b_data);
    zsl_mtx_from_arr(&K, K_data);

    // zsl_mtx_logging(&K, "K");

    zsl_fus_cal_corr_vec(&d, &K, &b, &d_out);

    // LOG_INF("Mag: %f %f %f -> %f %f %f", 
    //     data_vec[0], data_vec[1], data_vec[2],
    //     d_out.data[0], d_out.data[1], d_out.data[2]);

    sensor_value_from_double(&data[0], d_out.data[0]);
    sensor_value_from_double(&data[1], d_out.data[1]);
    sensor_value_from_double(&data[2], d_out.data[2]);

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
	if(!device_is_ready(dev_bmm350)) {
	    LOG_ERR("Device %s is not ready", dev_bmm350->name);
	    return -1;
	}
	LOG_INF("Device %s is ready", dev_bmm350->name);

	return 0;
}

double mag_ref[3] = {0.0, 0.0, 0.0};

void quaternion_to_euler(struct zsl_quat *q, struct zsl_euler *e)
{
    zsl_real_t roll;
    zsl_real_t pitch;
    zsl_real_t yaw;

    // Roll (x-axis rotation)
    double sinr_cosp = 2.0 * (q->r * q->i + q->j * q->k);
    double cosr_cosp = 1.0 - 2.0 * (q->i * q->i + q->j * q->j);
    roll = atan2(sinr_cosp, cosr_cosp);

    // Pitch (y-axis rotation)
    double sinp = 2.0 * (q->r * q->j - q->k * q->i);
    if (fabs(sinp) >= 1.0)
        pitch = copysign(PI / 2.0, sinp); // Use 90 degrees if out of range
    else
        pitch = asin(sinp);

    // Yaw (z-axis rotation)
    double siny_cosp = 2.0 * (q->r * q->k + q->i * q->j);
    double cosy_cosp = 1.0 - 2.0 * (q->j * q->j + q->k * q->k);
    yaw = atan2(siny_cosp, cosy_cosp);

    e->x = roll;
    e->y = pitch;
    e->z = yaw;
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
	struct sensor_value mag[3]; // NOTE: The bmm350 device have no zephyr drivers yet

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

    // Rotate the accelerometer 180 degrees around the x-axis and 90 degrees around the z-axis
	ret = rotate_measurement(accel0, 180, 0);
    if (ret) {
        LOG_ERR("rotate_measurement failed ret %d", ret);
        return -1;
    }

    ret = rotate_measurement(accel0, 90, 2);
    if (ret) {
        LOG_ERR("rotate_measurement failed ret %d", ret);
        return -1;
    }

    // Rotate the gyroscope 180 degrees around the x-axis and 90 degrees around the z-axis
    ret = rotate_measurement(gyr, 180, 0);
    if (ret) {
        LOG_ERR("rotate_measurement failed ret %d", ret);
        return -1;
    }

    ret = rotate_measurement(gyr, 90, 2);
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
	ret = sensor_sample_fetch(dev_bmm350);
	if(ret) {
	    LOG_ERR("sensor_sample_fetch failed ret %d", ret);
	    return -1;
	}

	ret = sensor_channel_get(dev_bmm350, SENSOR_CHAN_MAGN_XYZ, mag);
	if(ret) {
	    LOG_ERR("sensor_channel_get failed ret %d", ret);
	    return -1;
	}

    //////////////////////Correct magnetometer//////////////////////
    ret = sensors_correct_magnetometer(mag);
    if (ret) {
        LOG_ERR("sensors_correct_magnetometer failed ret %d", ret);
        return -1;
    }

	sample_count++;

    //////////////////FQA test//////////////////////
    // if (sample_count == 1) {
    if(mag_ref[0] == 0.0 && mag_ref[1] == 0.0 && mag_ref[2] == 0.0) {
        mag_ref[0] = sensor_value_to_double(&mag[0]);
        mag_ref[1] = sensor_value_to_double(&mag[1]);
        mag_ref[2] = sensor_value_to_double(&mag[2]);
    }

    struct zsl_vec *acc_vec;
    struct zsl_vec *mag_vec;
    struct zsl_vec *mag_ref_vec;
    struct zsl_quat *q;

    zsl_vec_alloc(&acc_vec, 3);
    zsl_vec_alloc(&mag_vec, 3);
    zsl_vec_alloc(&mag_ref_vec, 3);
    zsl_quat_alloc(&q, ZSL_QUAT_TYPE_IDENTITY);

    double acc_vec_data[3] = {sensor_value_to_double(&accel0[0]), sensor_value_to_double(&accel0[1]), sensor_value_to_double(&accel0[2])};
    double mag_vec_data[3] = {sensor_value_to_double(&mag[0]), sensor_value_to_double(&mag[1]), sensor_value_to_double(&mag[2])};

    zsl_vec_set(acc_vec, acc_vec_data);
    zsl_vec_set(mag_vec, mag_vec_data);
    zsl_vec_set(mag_ref_vec, mag_ref);

    ret = fqa_orientation(acc_vec, mag_vec, mag_ref_vec, q);
    if (ret != 0) {
        LOG_ERR("Failed to calculate orientation");
        return -1;
    }

    // LOG_INF("Orientation: %f, %f, %f, %f ", q->r, q->i, q->j, q->k); // NOTE: Logg fqa quaternion
    LOG_INF("Acc: %f, %f, %f \t Mag: %f, %f, %f", acc_vec_data[0], acc_vec_data[1], acc_vec_data[2], mag_vec_data[0], mag_vec_data[1], mag_vec_data[2]);

    struct zsl_euler euler;
    // zsl_quat_to_euler(q, &euler);
    quaternion_to_euler(q, &euler);

    // LOG_INF("Euler: %f, %f, %f", euler.x, euler.y, euler.z);

    zsl_vec_free(acc_vec);
    zsl_vec_free(mag_vec);
    zsl_vec_free(mag_ref_vec);
    zsl_quat_free(q);

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
	data[14] = sensor_value_to_double(&mag[0]);
	data[15] = sensor_value_to_double(&mag[1]);
	data[16] = sensor_value_to_double(&mag[2]);
    data[17] = euler.x * 180 / PI;
    data[18] = euler.y * 180 / PI;
    data[19] = euler.z * 180 / PI;
    // data[17] = 0;
    // data[18] = 0;
    // data[19] = 0;

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
					    "\"bmm350_magn_z\":%.03f,"
                        "\"euler_x\":%.03f,"
                        "\"euler_y\":%.03f,"
                        "\"euler_z\":%.03f"
					    "}";

	double data[20];
	ret = sensor_measure(data);
	if (ret) {
		LOG_ERR("sensor_measure failed ret %d", ret);
		return ret;
	}

	ret = snprintf(buf, len, sensors_json_template, (int)data[0], data[1], data[2], data[3],
		       data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11],
		       data[12], data[13], data[14], data[15], data[16], data[17], data[18], data[19]);

	// LOG_INF("JSON: %s", buf);

	if (ret >= len) {
		LOG_ERR("Sensor data does not fit in buffer");
		return -ENOSPC;
	}

	return ret;
}