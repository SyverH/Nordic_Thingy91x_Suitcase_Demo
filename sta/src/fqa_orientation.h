#pragma once

#include <zsl/zsl.h>
#include <zsl/matrices.h>
#include <zsl/vectors.h>
#include "zsl_utils.h"

int fqa_orientation(struct zsl_vec *accel, struct zsl_vec *magn, struct zsl_vec *magn_ref,
		    struct zsl_quat *q);