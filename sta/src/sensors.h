#pragma once

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <math.h>

#include <zsl/zsl.h>
#include <zsl/matrices.h>
#include <zsl/vectors.h>
#include <zsl/orientation/orientation.h>
#include <zsl/orientation/quaternions.h>
#include <zsl/orientation/euler.h>

#include "fqa_orientation.h"

// Magnetometer calibration values

// K matrix
//
//      | k0 k1 k2 |
//      | k3 k4 k5 |
//      | k6 k7 k8 |

#define DEFAULT_MAG_CAL_K0 1.4418112
#define DEFAULT_MAG_CAL_K1 0.0059029
#define DEFAULT_MAG_CAL_K2 0.0214013
#define DEFAULT_MAG_CAL_K3 0.000000
#define DEFAULT_MAG_CAL_K4 1.52038
#define DEFAULT_MAG_CAL_K5 0.1017505
#define DEFAULT_MAG_CAL_K6 0.000000
#define DEFAULT_MAG_CAL_K7 0.000000
#define DEFAULT_MAG_CAL_K8 1.5569043

// b vector
//
//      | bx by bz |

#define DEFAULT_MAG_CAL_BX       -19.9203904
#define DEFAULT_MAG_CAL_BY       7.1179814
#define DEFAULT_MAG_CAL_BZ       -7.6478235

// #include "zsl_utils.h"
// #include <zsl/vectors.h>
// #include <zsl/matrices.h>
// #include <zsl/orientation/quaternions.h>
// #include <zsl/orientation/euler.h>

// #include "fqa_orientation.h"

#define GRAVITY 9.80665
#define PI      3.14159265359

int sensors_init(void);
int sensors_measure(void);
int sensors_get_json(char *buf, size_t len);
int sensor_rotate_measurement(struct sensor_value *data, int x, int y, int z);