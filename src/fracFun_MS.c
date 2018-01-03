/***************************************************************************
 * Filename: fracFun_MS.c [testing]
 * Usage: mpirun [-np [0-9]] [-machinefile ./path/to/machine-file] ./bin/fracFun_MS
 * Author: Benjamin J Carrington
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "mpi.h"
#include "cmplx.h"
 
#define FULL_WIDTH 1024
#define CHUNK_WIDTH 32
#define MAX_ITER 1000

void plot(int* image_arr, FILE* img);
long iterator(Complex c, double im, double re);

int main(int argc, char* argv[])
{
    int *image_arr;
	int pixel_YX[3];
    int Y_start, X_start, CUR_CHUNK, disp = 0;
    int i, j;
    Complex c;
    FILE *img;
    int NUM_CHUNKS = (FULL_WIDTH / CHUNK_WIDTH) * (FULL_WIDTH / CHUNK_WIDTH);

    /** Timing variables */
    double start, stop;
    float elapsed_time;
     
    /** MPI specific variables */
    MPI_Status status, stat_recv;
    MPI_Request request;
    MPI_Datatype CHUNKxCHUNK, CHUNKxCHUNK_RE;
    int rankID, numProcs, numSlaves;
 
    /** Initialisation of MPI environment */
    MPI_Init(&argc, &argv); 
    MPI_Barrier(MPI_COMM_WORLD); 
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs); 
    MPI_Comm_rank(MPI_COMM_WORLD, &rankID); 
    MPI_Barrier(MPI_COMM_WORLD); 

    /** Variables for type creation */
    int full_sizes[2] = {FULL_WIDTH, FULL_WIDTH};
    int sub_sizes[2] = {CHUNK_WIDTH, CHUNK_WIDTH};
    int starting[2] = {0, 0};
    int sendcounts[numProcs];
    int displs[numProcs];

    /** Create CHUNK by CHUNK type */
    MPI_Type_create_subarray(
            2, 
            full_sizes, 
            sub_sizes, 
            starting, 
            MPI_ORDER_C, 
            MPI_INT, 
            &CHUNKxCHUNK
            );
    /** Specify offset */
    MPI_Type_create_resized(
            CHUNKxCHUNK,
            0, 
            CHUNK_WIDTH * sizeof(int), 
            &CHUNKxCHUNK_RE
            );
    /** Commit type to be used */
    MPI_Type_commit(&CHUNKxCHUNK_RE);

    /** # clients */
    numSlaves = numProcs - 1;
 
    /** Print basic information */
    if(rankID == 0)
        printf("Runtime Stats:\n\tNum Procs:\t%d\n\tNum Slaves:\t%d\n", numProcs, numSlaves);
    MPI_Barrier(MPI_COMM_WORLD);
 
    /** Hardcode constant */
    c.re = 0.285;
    c.im = 0.01;
 
    /** Master process portion of program */
    if(rankID == 0)
    {
        img = fopen("image_out.ppm","w");
        if(img == NULL)
        {
            printf("Could not open handle to image\n");
            return 1;
        }
        fprintf(img, "P6\n%d %d 255\n", FULL_WIDTH, FULL_WIDTH);
 
        image_arr = (int  *)malloc(FULL_WIDTH * FULL_WIDTH * sizeof(int));
 
        /** Start timer */
        start = MPI_Wtime();

        /** Calculate X and Y pixels and send to each client */
		for(pixel_YX[2] = 0; pixel_YX[2] < numSlaves; pixel_YX[2]++)
		{
            pixel_YX[0] = (pixel_YX[2] / (FULL_WIDTH / CHUNK_WIDTH)) * CHUNK_WIDTH; // Y
            pixel_YX[1] = (pixel_YX[2] % (FULL_WIDTH / CHUNK_WIDTH)) * CHUNK_WIDTH; // X

			MPI_Send(
                    pixel_YX, 
                    3, 
                    MPI_INT, 
                    pixel_YX[2] + 1, 
                    0, 
                    MPI_COMM_WORLD
                    );
		}

        /** Update current chunk and pixel for next send */
        pixel_YX[2] = numSlaves;
        pixel_YX[0] = (pixel_YX[2] / (FULL_WIDTH / CHUNK_WIDTH)) * CHUNK_WIDTH; // Y
        pixel_YX[1] = (pixel_YX[2] % (FULL_WIDTH / CHUNK_WIDTH)) * CHUNK_WIDTH; // X

        /** Recieve current chunk from X and send next chunk to X */
		while(pixel_YX[2] < NUM_CHUNKS)
		{
            /** Probe recieve buffer, calculate displacement within array, and receive */
			MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &stat_recv);
	
            disp = ((stat_recv.MPI_TAG * CHUNK_WIDTH) % FULL_WIDTH) + 
                (((stat_recv.MPI_TAG * CHUNK_WIDTH) / FULL_WIDTH) * CHUNK_WIDTH * FULL_WIDTH);

			MPI_Recv(
                    image_arr + disp,
                    1,
                    CHUNKxCHUNK_RE,
					stat_recv.MPI_SOURCE, 
					stat_recv.MPI_TAG, 
					MPI_COMM_WORLD,
					&status
                    );
			
#ifdef DEBUG
            printf("Proc: MA\tJob: Recieved [# %d]\n", stat_recv.MPI_TAG);
#endif

            MPI_Send(
                    pixel_YX,
                    3,
                    MPI_INT,
                    status.MPI_SOURCE,
                    0,
                    MPI_COMM_WORLD
                    );

            /** pixel_YX to next chunk values */
            pixel_YX[2]++;
            pixel_YX[0] = (pixel_YX[2] / (FULL_WIDTH / CHUNK_WIDTH)) * CHUNK_WIDTH; 
            pixel_YX[1] = (pixel_YX[2] % (FULL_WIDTH / CHUNK_WIDTH)) * CHUNK_WIDTH; 
		}

        /** Final recieves to match initial sends */
		for(i = 0; i < numSlaves; i++)
		{
            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &stat_recv);

            disp = ((stat_recv.MPI_TAG * CHUNK_WIDTH) % FULL_WIDTH) + (((stat_recv.MPI_TAG * CHUNK_WIDTH) / FULL_WIDTH) * CHUNK_WIDTH * FULL_WIDTH);

            MPI_Recv(
                    image_arr + disp, 
                    1, 
                    CHUNKxCHUNK_RE, 
                    stat_recv.MPI_SOURCE, 
                    stat_recv.MPI_TAG, 
                    MPI_COMM_WORLD,
                    &status
                    );
#ifdef DEBUG
            printf("Proc: MA\tJob: Recieved [# %d]\n", stat_recv.MPI_TAG);
#endif
        }
			
        /** Terminate clients */
		for(i = 0; i < numSlaves; i++)
			MPI_Send(
                    0, 
                    0, 
                    MPI_INT, 
                    i + 1, 
                    0xFFFF, 
                    MPI_COMM_WORLD
                    );

        /** Stop timer and calculate elapsed_time */
        stop = MPI_Wtime();
        elapsed_time = stop - start;
 
#ifdef DEBUG
		printf("Proc: Ma\tJob: Plotting image\n");
#endif
        plot(image_arr, img);
 
        printf("Algorithm completed for,\n\t%d * %d pixels\n\t%d maximum iterations\n\t\tin %f seconds.\n", \
                FULL_WIDTH, FULL_WIDTH, \
                MAX_ITER, \
                elapsed_time);
 
        fclose(img);
    }
    /** Client processes portion of program */
    else
    {
        /** Everybody allocate their portion of image_arr */
        image_arr = (int *)malloc(CHUNK_WIDTH * CHUNK_WIDTH * sizeof(int));
 
        /** Loop indefinitely until `break;` */
        while(1)
		{
			MPI_Recv(
                    pixel_YX, 
                    3, 
                    MPI_INT, 
                    0, 
                    MPI_ANY_TAG, 
                    MPI_COMM_WORLD, 
                    &status
                    );

            /** Check if being called to terminate */
			if(status.MPI_TAG == 0xFFFF)
			{
				printf("Proc: %d \tJob: Exiting\n", rankID);
				break;
			}

            CUR_CHUNK = pixel_YX[2];

#ifdef DEBUG
			printf("Proc: %d \tChunk %d \tJob: Algorithm\n", rankID, CUR_CHUNK);
#endif
 
			/** For each Y value */
            for(i = 0; i < CHUNK_WIDTH; i++)
            {
                for(j = 0; j < CHUNK_WIDTH; j++)
                {
                    image_arr[(i * CHUNK_WIDTH) + j] = iterator(
                            c,
                            -(((pixel_YX[0] + i) - (FULL_WIDTH / 2)) / (double) FULL_WIDTH) * 2,
                            (((pixel_YX[1] + j) - (FULL_WIDTH / 2)) / (double) FULL_WIDTH) * 2
                            );
                }
            }
 
#ifdef DEBUG
			printf("Proc: %d \tJob: Returning [# %d]\n", rankID, CUR_CHUNK);
#endif
 
			/** Send portion of calculated imaged to MASTER */
			MPI_Send(
                    image_arr, 
                    CHUNK_WIDTH * CHUNK_WIDTH,
                    MPI_INT, 
                    0, 
                    CUR_CHUNK, 
                    MPI_COMM_WORLD
                    );
		}
    }
 
    free(image_arr);
 
    /** Finalise MPI environment */
    MPI_Type_free(&CHUNKxCHUNK_RE);
    MPI_Finalize();
    fflush(stdout);
 
    return 0;
}
 
/**
Main iterating function of the program
*/
long iterator(Complex c, double im, double re)
{
    Complex z;
    long itCount = 0;
 
    z.re = re;
    z.im = im;
 
    for(; itCount < MAX_ITER; itCount++)
    {
        z = cmplx_add(cmplx_squared(z), c);
        if(cmplx_magnitude(z) > 4)
            break;
    }
 
    return itCount + 1;
}
 
/**
Function which calculates the pixel values from the square array
*/
void plot(int* image_arr, FILE* img)
{
    int i, j;
    unsigned char line[3 * FULL_WIDTH];
 
    for(i = 0; i < FULL_WIDTH; i++)
    {
        for(j = 0; j < FULL_WIDTH; j++)
        {
            if (*(image_arr + j + (i * FULL_WIDTH)) <= 63)
            {
                line[3 * j] = 255;
                line[3 * j + 1] = line[3 * j + 2] = 
                        255 - 4 * *(image_arr + j + (i * FULL_WIDTH));
            }
            else
            {
                line[3 * j] = 255;
                line[3 * j + 1] = *(image_arr + j + (i * FULL_WIDTH)) - 63;
                line[3 * j + 2] = 0;
            }
            if (*(image_arr + j + (i * FULL_WIDTH)) == 320) 
                line[3 * j] = line[3 * j + 1] = line[3 * j + 2] = 255;
        }
        fwrite(line, 1, 3 * FULL_WIDTH, img);
    }
}
