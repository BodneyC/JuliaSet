#include <math.h>
#include "cmplx.h"

/**
Complex magnitude function
*/
double cmplx_magnitude(Complex c)
{
    return (c.re * c.re) + (c.im * c.im);
}

/**
Complex add function
*/
Complex cmplx_add(Complex c_1, Complex c_2)
{
	Complex retComp;

	retComp.re = c_1.re + c_2.re;
	retComp.im = c_1.im + c_2.im;
	
	return(retComp);
}

/**
Complex squaring function
*/
Complex cmplx_squared(Complex c) 
{
	Complex retComp;

	retComp.re = pow(c.re, 2) - pow(c.im, 2);
	retComp.im = (c.im * c.re) * 2;

	return(retComp);
}

