#ifndef CMPLX_HEAD
#define CMPLX_HEAD

/**
Structure for storing two doubles to simulate a complex number
*/
typedef struct Complex
{
	double re, im;
} Complex;

/**
Complex abdolute function
*/
double cmplx_magnitude(Complex c);

/**
Complex add function
*/
Complex cmplx_add(Complex c_1, Complex c_2);

/**
Complex squaring function
*/
Complex cmplx_squared(Complex c);

#endif
