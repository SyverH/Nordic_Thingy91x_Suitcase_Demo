#ifndef ZSL_UTILS_H__
#define ZSL_UTILS_H__

#include <zsl/zsl.h>
#include <zsl/vectors.h>
#include <zsl/matrices.h>
#include <zsl/orientation/quaternions.h>

void zsl_vec_logging(struct zsl_vec *v, const char *name);
void zsl_mtx_logging(struct zsl_mtx *mtx, const char *name);
void zsl_quat_logging(struct zsl_quat *q, const char *name);

int zsl_vec_alloc(struct zsl_vec **v, size_t size);
int zsl_vec_free(struct zsl_vec *v);

int zsl_vec_set(struct zsl_vec *v, double *data);

int zsl_mtx_alloc(struct zsl_mtx **mtx, size_t rows, size_t cols, zsl_mtx_init_entry_fn_t entry_fn);
int zsl_mtx_free(struct zsl_mtx *mtx);

int zsl_quat_alloc(struct zsl_quat **q, enum zsl_quat_type type);
int zsl_quat_free(struct zsl_quat *q);

int zsl_quat_set(struct zsl_quat *q, double r, double i, double j, double k);
int zsl_quat_set_vec(struct zsl_quat *q, double *data);
int zsl_quat_copy(struct zsl_quat *q_dest, struct zsl_quat *r_src);

int zsl_vec_mtx_mult(struct zsl_vec *v, struct zsl_mtx *m, struct zsl_vec *r);
int zsl_mtx_vec_mult(struct zsl_mtx *m, struct zsl_vec *v, struct zsl_vec *r);
int zsl_mtx_unsymm_mult(struct zsl_mtx *m1, struct zsl_mtx *m2, struct zsl_mtx *r);

#endif // ZSL_UTILS_H__