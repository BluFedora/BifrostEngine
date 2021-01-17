// TODO(SR): Delete Me!

#ifndef BIFROST_MATH2_H
#define BIFROST_MATH2_H
#include <float.h> /* DBL_MAX, FLT_MAX, DBL_EPSILON, FLT_EPSILON */
#include <math.h>  /* pow, powf, fabs, fabsf, sqrt, sqrtf        */
#define BIFROST_PHYSICS_DBL 0
#define BIFROST_PHYSICS_FLT 1
#if defined(BIFROST_USE_DOUBLE) && BIFROST_USE_DOUBLE
#define BIFROST_PHYSICS_PRECISION BIFROST_PHYSICS_DBL
#else
#define BIFROST_PHYSICS_PRECISION BIFROST_PHYSICS_FLT
#endif

#if BIFROST_PHYSICS_PRECISION == BIFROST_PHYSICS_DBL
#define max_real DBL_MAX
#define pow_real pow
#define abs_real fabs
#define sqrt_real sqrt
#define epsilon_real DBL_EPSILON

typedef double bf_real;
#elif BIFROST_PHYSICS_PRECISION == BIFROST_PHYSICS_FLT
#define max_real FLT_MAX
#define pow_real powf
#define abs_real fabsf
#define sqrt_real sqrtf
#define epsilon_real FLT_EPSILON

typedef float bf_real;
#else
#error "A floating point precision must be specified."
#endif
#endif
