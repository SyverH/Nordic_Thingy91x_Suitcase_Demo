/**
 *@file zsl_utils.c
 *@author Trondfc
 *@brief Utility functions for vectors, matrices and quaternions, expansion of the zscilib library.
 *@version 0.1
 *        - 2024-10-31: Initial implementation. Logging functions for vectors and matrices.
 *        - 2024-11-01: Added memory allocation functions for vectors and matrices.
 *        - 2024-11-04: Added matrix-vector multiplication functions.
 *        - 2024-11-21: Added quaternion utility functions.
 *        - 2025-01-13: Added file header.
 *@date 2024-10-31
 */
#include "zsl_utils.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(zsl_utils, CONFIG_ZSL_UTILS_LOG_LEVEL);

#include <zsl/zsl.h>
#include <zsl/vectors.h>
#include <zsl/matrices.h>
#include <zsl/orientation/quaternions.h>

#include <stdlib.h>

/**
 *@brief Log the data of a vector. same as zsl_vec_print but with logging backend.
 *
 *@param v vector to log.
 *@param name name of the vector.
 */
void zsl_vec_logging(struct zsl_vec *v, const char *name)
{
	if (v == NULL) {
		LOG_ERR("Vector %s is NULL", name);
		return;
	}

	if (v->data == NULL) {
		LOG_ERR("Data in vector %s is NULL", name);
		return;
	}

	if (name == NULL) {
		LOG_WRN("Name is NULL");
		name = "NULL";
	}

	int malloc_size = v->sz * sizeof(char) * 7 +
			  128; // 7 characters per number + 128 for the rest of the string
	char *data_str = malloc(malloc_size);
	memset(data_str, 0, malloc_size);
	int str_len = 0;

	snprintf(data_str, malloc_size, "\r\n\t%s: \r\n\t", name);
	str_len = strlen(data_str);

	for (int i = 0; i < v->sz; i++) {
		snprintf(data_str + str_len, malloc_size - str_len, "%.05f ", v->data[i]);
		str_len = strlen(data_str);
	}
	LOG_INF("%s", data_str);
	free(data_str);
}

/**
 *@brief Log the data of a matrix. same as zsl_mtx_print but with logging backend.
 *
 *@param mtx matrix to log.
 *@param name name of the matrix.
 */
void zsl_mtx_logging(struct zsl_mtx *mtx, const char *name)
{
	// LOG_INF("Logging matrix %s", name);
	if (mtx == NULL) {
		LOG_ERR("Matrix %s is NULL", name);
		return;
	}

	if (mtx->data == NULL) {
		LOG_ERR("Data in matrix %s is NULL", name);
		return;
	}

	if (name == NULL) {
		LOG_WRN("Name is NULL");
		name = "NULL";
	}

	// LOG_INF("allocating memory for data_str");

	int malloc_size = mtx->sz_rows * mtx->sz_cols * sizeof(char) * 8 +
			  128; // 7 characters per number + 128 for the rest of the string
	char *data_str = malloc(malloc_size);
	memset(data_str, 0, malloc_size);
	int str_len = 0;

	// LOG_INF("Assembeling data_str");

	snprintf(data_str, malloc_size, "\r\n\t%s: \r\n\t", name);
	str_len = strlen(data_str);

	for (int i = 0; i < mtx->sz_rows; i++) {
		for (int j = 0; j < mtx->sz_cols; j++) {
			snprintf(data_str + str_len, malloc_size - str_len, "%.05f ",
				 mtx->data[i * mtx->sz_cols + j]);
			str_len = strlen(data_str);
		}
		snprintf(data_str + str_len, malloc_size - str_len, "\r\n\t");
		str_len = strlen(data_str);
	}
	// k_sleep(K_MSEC(1000));
	LOG_INF("%s", data_str);
	// LOG_INF("Data str size: %d", str_len);
	free(data_str);
	// LOG_INF("Memory freed");
}

/**
 *@brief Log the data of a quaternion. same as zsl_quat_print but with logging backend.
 *
 *@param q quaternion to log.
 *@param name name of the quaternion.
 */
void zsl_quat_logging(struct zsl_quat *q, const char *name)
{
	if (q == NULL) {
		LOG_ERR("Quaternion %s is NULL", name);
		return;
	}

	if (name == NULL) {
		LOG_WRN("Name is NULL");
		name = "NULL";
	}

	int malloc_size = strlen(name) + 4 * 7 +
			  32; // 7 characters per number + 32 for the rest of the string
	char *data_str = malloc(malloc_size);
	memset(data_str, 0, malloc_size);
	int str_len = 0;

	snprintf(data_str, malloc_size, "\r\n\t%s: \r\n\t", name);
	str_len = strlen(data_str);

	snprintf(data_str + str_len, malloc_size - str_len,
		 "r: %.05f, i: %.05f, j: %.05f, k: %.05f", q->r, q->i, q->j, q->k);

	LOG_INF("%s", data_str);
	free(data_str);
}

/**
 *@brief Allocate memory for a vector and its data. same function as ZSL_VECTOR_DEF but with memory
 *allocation instead of stack allocation.
 *
 *@param v vector to allocate.
 *@param sz size of the vector.
 *@return int 0 if successful, error code otherwise.
 */
int zsl_vec_alloc(struct zsl_vec **v, size_t sz)
{
	*v = malloc(sizeof(struct zsl_vec));
	if (*v == NULL) {
		LOG_ERR("Memory allocation for vector failed");
		return -1;
	}
	(*v)->sz = sz;
	(*v)->data = malloc(sz * sizeof(double));
	if ((*v)->data == NULL) {
		LOG_ERR("Memory allocation for vector data failed");
		free(*v); // Free the allocated vector structure
		*v = NULL;
		return -1;
	}
	zsl_vec_init(*v);

	return 0;
}

/**
 *@brief Free the memory allocated for a vector.
 *
 *@param v vector to free.
 *@return int 0 if successful, error code otherwise.
 */
int zsl_vec_free(struct zsl_vec *v)
{
	if (v == NULL) {
		LOG_ERR("Vector is NULL");
		return -1;
	}

	if (v->data != NULL) {
		free(v->data);
	}
	free(v);
	return 0;
}

/**
 *@brief Set the data of a vector.
 *
 *@param v pointer to the vector.
 *@param data data to set.
 *@return int 0 if successful, error code otherwise.
 */
int zsl_vec_set(struct zsl_vec *v, double *data)
{
	if (v == NULL || data == NULL) {
		LOG_ERR("Vector or data is NULL");
		return -1;
	}

	for (size_t i = 0; i < v->sz; i++) {
		v->data[i] = data[i];
	}

	return 0;
}

/**
 *@brief Allocate memory for a matrix and its data. same function as ZSL_MATRIX_DEF but with memory
 *allocation instead of stack allocation.
 *
 *@param mtx matrix to allocate.
 *@param rows number of rows.
 *@param cols number of columns.
 *@param entry_fn zs_mtx_init_entry_fn_t function to initialize the matrix entries.
 *@return int 0 if successful, error code otherwise.
 */
int zsl_mtx_alloc(struct zsl_mtx **mtx, size_t rows, size_t cols, zsl_mtx_init_entry_fn_t entry_fn)
{
	*mtx = malloc(sizeof(struct zsl_mtx));
	if (*mtx == NULL) {
		LOG_ERR("Memory allocation for matrix failed");
		return -1;
	}
	(*mtx)->sz_rows = rows;
	(*mtx)->sz_cols = cols;
	(*mtx)->data = malloc(rows * cols * sizeof(double));
	if ((*mtx)->data == NULL) {
		LOG_ERR("Memory allocation for matrix data failed");
		free(*mtx); // Free the allocated matrix structure
		*mtx = NULL;
		return -1;
	}
	zsl_mtx_init(*mtx, entry_fn);

	return 0;
}

/**
 *@brief free the memory allocated for a matrix.
 *
 *@param mtx matrix to free.
 *@return int 0 if successful, error code otherwise.
 */
int zsl_mtx_free(struct zsl_mtx *mtx)
{
	if (mtx == NULL) {
		LOG_ERR("Matrix is NULL");
		return -1;
	}

	if (mtx->data != NULL) {
		free(mtx->data);
	}
	free(mtx);
	return 0;
}

/**
 *@brief Allocate memory for a quaternion.
 *
 *@param q quaternion to allocate.
 *@param type zsl_quat_type type to initialize the quaternion with.
 *@return int 0 if successful, error code otherwise.
 */
int zsl_quat_alloc(struct zsl_quat **q, enum zsl_quat_type type)
{
	*q = malloc(sizeof(struct zsl_quat));
	if (*q == NULL) {
		LOG_ERR("Memory allocation for quaternion failed");
		return -1;
	}
	zsl_quat_init(*q, type);

	return 0;
}

int zsl_quat_free(struct zsl_quat *q)
{
	if (q == NULL) {
		LOG_ERR("Quaternion is NULL");
		return -1;
	}

	free(q);
	return 0;
}

/**
 *@brief Set the data of a quaternion.
 *
 *@param q pointer to the quaternion.
 *@param r quaternion lement value.
 *@param i quaternion lement value.
 *@param j quaternion lement value.
 *@param k quaternion lement value.
 *@return int 0 if successful, error code otherwise.
 */
int zsl_quat_set(struct zsl_quat *q, double r, double i, double j, double k)
{
	if (q == NULL) {
		LOG_ERR("Quaternion is NULL");
		return -1;
	}

	q->r = r;
	q->i = i;
	q->j = j;
	q->k = k;

	return 0;
}

/**
 *@brief Set the data of a quaternion from a vector.
 *
 *@param q pointer to the quaternion.
 *@param data pointer to the vector data.
 *@return int 0 if successful, error code otherwise.
 */
int zsl_quat_set_vec(struct zsl_quat *q, double *data)
{
	if (q == NULL || data == NULL) {
		LOG_ERR("Quaternion or data is NULL");
		return -1;
	}

	q->r = data[0];
	q->i = data[1];
	q->j = data[2];
	q->k = data[3];

	return 0;
}

/**
 *@brief Copy the data of a quaternion to another quaternion.
 *
 *@param q_dest destination quaternion.
 *@param q_src source quaternion.
 *@return int 0 if successful, error code otherwise.
 */
int zsl_quat_copy(struct zsl_quat *q_dest, struct zsl_quat *q_src)
{
	if (q_dest == NULL || q_src == NULL) {
		LOG_ERR("Quaternion is NULL");
		return -1;
	}

	q_dest->r = q_src->r;
	q_dest->i = q_src->i;
	q_dest->j = q_src->j;
	q_dest->k = q_src->k;

	return 0;
}

/**
 *@brief Multiply a vector by a matrix.
 *
 *@param v vector to multiply.
 *@param m matrix to multiply.
 *@param r result vector.
 *@return int 0 if successful, error code otherwise.
 */
int zsl_vec_mtx_mult(struct zsl_vec *v, struct zsl_mtx *m, struct zsl_vec *r)
{
	if (v == NULL || m == NULL || r == NULL) {
		LOG_ERR("Vector or matrix is NULL");
		return -1;
	}

	if (v->sz != m->sz_cols) {
		LOG_ERR("Vector size does not match matrix column size");
		return -1;
	}

	if (r->sz != m->sz_rows) {
		LOG_ERR("Result vector size does not match matrix row size\r\n  \
                Result vector size should be %d",
			m->sz_rows);
		return -1;
	}

	// Initialize the result vector
	for (size_t i = 0; i < r->sz; i++) {
		r->data[i] = 0.0;
	}

	// Perform the multiplication
	for (size_t i = 0; i < m->sz_rows; i++) {
		for (size_t j = 0; j < m->sz_cols; j++) {
			r->data[i] += m->data[i * m->sz_cols + j] * v->data[j];
		}
	}

	return 0;
}

/**
 *@brief Multiply a matrix by a vector.
 *
 *@param m matrix to multiply.
 *@param v vector to multiply.
 *@param r result vector.
 *@return int 0 if successful, error code otherwise.
 */
int zsl_mtx_vec_mult(struct zsl_mtx *m, struct zsl_vec *v, struct zsl_vec *r)
{
	if (m == NULL || v == NULL || r == NULL) {
		LOG_ERR("Matrix or vector is NULL");
		return -1;
	}

	if (m->sz_cols != v->sz) {
		LOG_ERR("Matrix column size does not match vector size");
		return -1;
	}

	if (r->sz != m->sz_rows) {
		LOG_ERR("Result vector size does not match matrix row size\r\n  \
                Result vector size should be %d",
			m->sz_rows);
		return -1;
	}

	// Initialize the result vector
	for (size_t i = 0; i < r->sz; i++) {
		r->data[i] = 0.0;
	}

	// Perform the multiplication
	for (size_t i = 0; i < m->sz_rows; i++) {
		for (size_t j = 0; j < m->sz_cols; j++) {
			r->data[i] += m->data[i * m->sz_cols + j] * v->data[j];
		}
	}

	return 0;
}

/**
 *@brief multiply two matrices of different sizes.
 *
 *@param m1 pointer to the first matrix.
 *@param m2 pointer to the second matrix.
 *@param r result matrix, expected size is m1->sz_rows x m2->sz_cols.
 *@return int 0 if successful, error code otherwise.
 */
int zsl_mtx_unsymm_mult(struct zsl_mtx *m1, struct zsl_mtx *m2, struct zsl_mtx *r)
{
	if (m1 == NULL || m2 == NULL || r == NULL) {
		LOG_ERR("Matrices are NULL");
		return -1;
	}

	if (m1->sz_cols != m2->sz_rows) {
		LOG_ERR("Matrix sizes do not match");
		return -1;
	}

	if (r->sz_rows != m1->sz_rows || r->sz_cols != m2->sz_cols) {
		LOG_ERR("Result matrix size does not match the matrix multiplication result\r\n  \
                Result matrix size should be %d x %d",
			m1->sz_rows, m2->sz_cols);

		return -1;
	}

	for (int i = 0; i < m1->sz_rows; i++) {
		for (int j = 0; j < m2->sz_cols; j++) {
			r->data[i * r->sz_cols + j] = 0;
			for (int k = 0; k < m1->sz_cols; k++) {
				r->data[i * r->sz_cols + j] += m1->data[i * m1->sz_cols + k] *
							       m2->data[k * m2->sz_cols + j];
			}
		}
	}
	return 0;
}