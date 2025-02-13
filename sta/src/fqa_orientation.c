/**
 *@file fqa_orientation.c
 *@author Trondfc
 *@brief Implementation of the Fast Quaternion Algorithm for orientation estimation.
 *           Based on math in https://ahrs.readthedocs.io/en/latest/filters/fqa.html
 *           and code in  https://github.com/Mayitzin/ahrs/blob/master/ahrs/filters/fqa.py
 *@version 0.3
 *        - 2024-11-17: Initial implementation.
 *        - 2025-01-07: Fixed mistake in the calculation of the roll quaternion.
 *        - 2025-01-13: Small cleanup of the code. Added file header.
 *@date 2024-11-17
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fqa, CONFIG_FQA_LOG_LEVEL);

#include "fqa_orientation.h"
#include "zsl_utils.h"

#define SIGN(x) ((x) == 0 ? 0 : (x) > 0 ? 1 : -1)

/**
 *@brief Calculate the norm of a vector.
 *
 *@param v vector to calculate the norm of.
 *@return double norm of the vector (sqrt of the sum of the squares of the elements).
 */
double vector_norm(struct zsl_vec *v)
{
	double norm = 0.0;
	for (size_t i = 0; i < v->sz; i++) {
		norm += v->data[i] * v->data[i];
	}
	return sqrt(norm);
}

/**
 *@brief Calculate the orientation of the device using the Fast Quaternion Algorithm.
 *
 *@param accel pointer to 3D vector with the accelerometer data.
 *@param magn pointer to 3D vector with the magnetometer data.
 *@param magn_ref pointer to 3D vector with the reference magnetometer data (either the initial
 *magnetometer data or a non-rotating magnetometer data).
 *@param q pointer to the quaternion to store the orientation.
 *@return int 0 if successful, error code otherwise.
 */
int fqa_orientation(struct zsl_vec *accel, struct zsl_vec *magn, struct zsl_vec *magn_ref,
		    struct zsl_quat *q)
{
	int ret = 0;

	// Normalize the accelerometer and magnetometer data.
	struct zsl_vec *a;
	struct zsl_vec *m;
	struct zsl_vec *m_ref;

	zsl_vec_alloc(&a, 3);
	zsl_vec_alloc(&m, 3);
	zsl_vec_alloc(&m_ref, 3);

	zsl_vec_copy(a, accel);
	zsl_vec_copy(m, magn);
	zsl_vec_copy(m_ref, magn_ref);

	// Normalize the accelerometer vector
	ret = zsl_vec_to_unit(a);

	// Elevation quaternion
	double s_theta = MIN((MAX(a->data[0], -1.0)), 1.0);
	double c_theta = sqrt(1 - s_theta * s_theta);
	double s_theta_2 = SIGN(s_theta) * sqrt((1.0 - c_theta) / 2.0);
	double c_theta_2 = sqrt((1.0 + c_theta) / 2.0);
	struct zsl_quat *q_elev;
	zsl_quat_alloc(&q_elev, ZSL_QUAT_TYPE_EMPTY);
	zsl_quat_set(q_elev, c_theta_2, 0.0, s_theta_2, 0.0);
	zsl_quat_to_unit_d(q_elev);

	// Roll quaternion
	bool is_singular = (c_theta == 0.0);
	double s_phi = 0.0;
	double c_phi = 0.0;
	if (!is_singular) {
		s_phi = (-1 * a->data[1] / c_theta);
		c_phi = (-1 * a->data[2] / c_theta);
	}
	c_phi = MIN((MAX(c_phi, -1.0)), 1.0);
	int sign_s_phi = SIGN(s_phi);
	if (c_phi == -1.0 && s_phi == 0.0) {
		sign_s_phi = 1.0;
	}
	double s_phi_2 = sign_s_phi * sqrt((1.0 - c_phi) / 2.0);
	double c_phi_2 = sqrt((1.0 + c_phi) / 2.0);
	struct zsl_quat *q_roll;
	zsl_quat_alloc(&q_roll, ZSL_QUAT_TYPE_EMPTY);
	zsl_quat_set(q_roll, c_phi_2, s_phi_2, 0.0, 0.0);
	zsl_quat_to_unit_d(q_roll);

	struct zsl_quat *q_elev_roll;
	zsl_quat_alloc(&q_elev_roll, ZSL_QUAT_TYPE_EMPTY);
	zsl_quat_mult(q_elev, q_roll, q_elev_roll);

	// Azimuth quaternion
	double m_norm = vector_norm(m);
	if (m_norm <= 0.0) {
		LOG_WRN("Magnetometer vector norm is zero");
		ret = zsl_quat_copy(q, q_elev_roll);
	}

	ret = zsl_vec_to_unit(m);
	if (ret != 0) {
		LOG_ERR("Failed to normalize magnetometer vector");
		return ret;
	}
	struct zsl_quat *q_bm;
	zsl_quat_alloc(&q_bm, ZSL_QUAT_TYPE_EMPTY);
	zsl_quat_set(q_bm, 0.0, m->data[0], m->data[1], m->data[2]);

	struct zsl_quat *q_em;
	zsl_quat_alloc(&q_em, ZSL_QUAT_TYPE_EMPTY);
	struct zsl_quat *q_temp1;
	zsl_quat_alloc(&q_temp1, ZSL_QUAT_TYPE_EMPTY);
	struct zsl_quat *q_temp2;
	zsl_quat_alloc(&q_temp2, ZSL_QUAT_TYPE_EMPTY);
	struct zsl_quat *q_temp3;
	zsl_quat_alloc(&q_temp3, ZSL_QUAT_TYPE_EMPTY);

	zsl_quat_conj(q_roll, q_temp1);           // q_conj(q_r)
	zsl_quat_conj(q_elev, q_temp2);           // q_conj(q_e)
	zsl_quat_mult(q_temp1, q_temp2, q_temp3); // q_prod(q_conj(q_r), q_conj(q_e))
	zsl_quat_mult(q_bm, q_temp3, q_temp1);    // q_prod(bm, q_prod(q_conj(q_r), q_conj(q_e)))
	zsl_quat_mult(q_roll, q_temp1,
		      q_temp2); // q_prod(q_r, q_prod(bm, q_prod(q_conj(q_r), q_conj(q_e))))
	zsl_quat_mult(
		q_elev, q_temp2,
		q_em); // q_prod(q_e, q_prod(q_r, q_prod(bm, q_prod(q_conj(q_r), q_conj(q_e)))))

	double nx = m_ref->data[0];
	double ny = m_ref->data[1];
	struct zsl_vec *N_vec;
	zsl_vec_alloc(&N_vec, 2);
	N_vec->data[0] = nx;
	N_vec->data[1] = ny;
	zsl_vec_to_unit(N_vec);
	double Mx = q_em->i / sqrt(q_em->i * q_em->i + q_em->j * q_em->j);
	double My = q_em->j / sqrt(q_em->i * q_em->i + q_em->j * q_em->j);
	double c_psi = N_vec->data[0] * Mx + N_vec->data[1] * My;
	double s_psi = N_vec->data[0] * My * -1 + N_vec->data[1] * Mx;
	c_psi = MIN((MAX(c_psi, -1.0)), 1.0);
	double s_psi_2 = SIGN(s_psi) * sqrt((1.0 - c_psi) / 2.0);
	double c_psi_2 = sqrt((1.0 + c_psi) / 2.0);
	struct zsl_quat *q_azim;
	zsl_quat_alloc(&q_azim, ZSL_QUAT_TYPE_EMPTY);
	zsl_quat_set(q_azim, c_psi_2, 0.0, 0.0, s_psi_2);
	zsl_quat_to_unit_d(q_azim);

	// Final quaternion
	zsl_quat_mult(q_azim, q_elev_roll, q);
	zsl_quat_to_unit_d(q);

	// enshure positive scalar part
	if (q->r < 0) {
		q->r = -q->r;
		q->i = -q->i;
		q->j = -q->j;
		q->k = -q->k;
	}

	// Free memory
	zsl_quat_free(q_elev);
	zsl_quat_free(q_roll);
	zsl_quat_free(q_elev_roll);
	zsl_quat_free(q_bm);
	zsl_quat_free(q_em);
	zsl_quat_free(q_temp1);
	zsl_quat_free(q_temp2);
	zsl_quat_free(q_temp3);
	zsl_quat_free(q_azim);
	zsl_vec_free(a);
	zsl_vec_free(m);
	zsl_vec_free(m_ref);
	zsl_vec_free(N_vec);

	return 0;
}