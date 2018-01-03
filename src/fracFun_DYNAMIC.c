/***************************************************************************
 * Filename: fracfun_DYNAMIC.c
 * Usage: ./bin/fracfun_DYNAMIC [max_iterations] [real_part] [imaginary_part] 
 *			[Optional: [X co-ord] [Y co-ord]]
 * Author: Benjamin J Carrington
 *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "cmplx.h"

/**
Takes the information of 'image', calculates colour intensity per pixel, and 
writes to 'img' handle
*/
void plot(int** image, FILE* img, int szX, int szY);

/**
Performs the equation of z = z^2 + c; returns number of iterations before 
point falls outside the circle
*/
int iterator(Complex c, int max_iterations, double im, double re);

/**
Main function
*/
int main(int argc, char* argv[])
{
	/** Double pointer to be used as 2d array for the image */
    int **image;
    /** X and Y co-ordinates with default values */
	int szX = 500, szY = 500;
    /** Maximum number of iterations for algorithm */
    int max_iterations;
    /** Counters */
	int i, j;
    /** FILE handle */
	FILE *img;
    /** Constant 'c' of type Complex (struct) */
	Complex c;
    /** Pointers to end characters of string conversions */
	char *itEnd_p, *imEnd_p, *reEnd_p, *szXEnd_p, *szYEnd_p;
    /** Clock variables for timing purposes */
	clock_t start, finish;
    float elapsed_time;

	/** Assure correct # of arguments */
	if(argc != 4 && argc != 6)
	{
		printf("Incorrect number of arguments\nUsage:\n\tfractal [max_iterations] [real_part] [imaginary_part]\n");
		return 1;
	}
	
	/** User-input section */
	max_iterations = strtol(argv[1], &itEnd_p, 10);
	c.re = strtod(argv[2], &reEnd_p);
	c.im = strtod(argv[3], &imEnd_p);

    /** If X and Y has been user-inputted, convert them to int and store */
	if(argc == 6) 
	{
		szX = strtol(argv[4], &szXEnd_p, 10);
		szY = strtol(argv[5], &szYEnd_p, 10);
	}

    /** If unexpected characters are caught by conversions, end the program */
	if(*itEnd_p || *imEnd_p || *reEnd_p) 
	{
		printf("Non-numeric characters in input\nProgram ending\n");
		return 1;
	}
	
    /** Allocate memory for 'image' */
	image = (int **)malloc(szX * sizeof(int *));
	for(i = 0; i < szX; i++)
		image[i] = (int *)malloc(szY * sizeof(int));

	/** Open 'img' handle as 'overwrite if exists' */
	img=fopen("image_out.ppm","w");
    if(img == NULL)
    {
        printf("Could not open output file. Exiting\n");
        return 1;
    }

    /** Print file signature to handle */
	fprintf(img,"P6\n%d %d 255\n", szX, szY);
    if(img == NULL)
    {
        printf("Could not open image file\n");
        return 1;
    }

	/** Begin the clock */
	start = clock();

    /** For all values of szX */
	for(i = 0; i < szY; i++)
	{
        /** For all values of szY */
		for(j = 0; j < szX; j++)
		{
            /** Call iterator function with each pixel (mapped between -1 and 1) */
            image[j][i] = iterator(c, max_iterations, \
					-(((i - (szY / 2)) / (double) szY) * 2), 
					((j - (szX / 2)) / (double) szX) * 2);			
		}	
	}

	/** End the clock */
	finish = clock() - start;

    /** Convert clocks/second to seconds (as float) */
	elapsed_time = ((float) finish) / ((float) CLOCKS_PER_SEC);
	
    /** Print information regarding algorithm and run time */
	printf("Algorithm completed for,\n\t%d * %d pixels\n\t%d maximum iterations\n\t\tin %f seconds.\n", \
			szX, szY, \
			max_iterations, \
			elapsed_time);
	
	/** Plot the image */
	plot(image, img, szX, szY);

	/** Close handle to 'img' */
	fclose(img);

    /** Free memory for 'image' */
	for(i = 0; i < szX; i++)
		free(image[i]);
	free(image);

    /** Successful return */
	return 0;
}

/**
Iterating function
*/
int iterator(Complex c, int max_iterations, double re, double im)
{
    /** Variable 'z' of type Complex structure */
	Complex z;
    /** Counter of iterations */
	int itCount;

    /** Setting 'z' equal to the incoming grid co-ordinates */
	z.re = re;
	z.im = im;

    /** Main loop of the algorithm which is ran 'max_iterations' number of times */
	for(itCount = 0; itCount < max_iterations; itCount++)
	{
        /** Perform z = z^2 + c */
		z = cmplx_add(cmplx_squared(z), c);

        /** If components of z squared are > 4, break */
		if(cmplx_magnitude(z) > 4)
			break;
	}

    /** Return the number of iterations */
	return itCount;
}

/**
Plotting function
*/
void plot(int** image, FILE* img, int szX, int szY)
{
    /** Counters */
	int i, j;
    /** Array of byte values of the width of X dimension multiplied by three to accomodate
    one byte each for R, G, and B */
	unsigned char line[3 * szX];

    /** For the length of Y */
	for(i = 0; i < szY; i++)
	{
        /** For the length of X */
		for(j = 0; j < szX; j++)
		{
			if (image[j][i] <= 63)
			{
				line[3 * j] = 255;
				line[3 * j + 1] = line[ 3 * j + 2 ] = 255 - 4 * image[j][i];
			}
			else
			{
				line[3 * j] = 255;
				line[3 * j + 1] = image[j][i] - 63;
				line[3 * j + 2] = 0;
			}
			if (image[j][i] == 320)	
                line[3 * j] = line[3 * j + 1] = line[3 * j + 2] = 255;
		}
        /** Write 'line' array to 'img' handle */
		fwrite(line, 1, 3 * szX, img);
	}
}
