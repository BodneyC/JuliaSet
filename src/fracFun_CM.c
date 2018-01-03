/***************************************************************************
 * Filename: fracFun_CM.c [testing]
 * Usage: mpirun [-np [0-9]] [-machinefile ./path/to/machine-file] ./bin/fracFun_CM
 * Author: Benjamin J Carrington
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpi.h"
#include "cmplx.h"

#define FULL_WIDTH 16384
#define CHUNK_WIDTH 2
#define MAX_ITER 1000

void plot(int* full_arr, FILE* img);
long iterator(Complex c, double im, double re);

int main(int argc, char* argv[])
{
    int *send_arr, *full_arr;
    int Y_start, X_start, CUR_CHUNK, CHUNK_SQUARED;
    int i, j, k, LOOPCOUNT;
    int NUM_CHUNKS = (FULL_WIDTH / CHUNK_WIDTH) * (FULL_WIDTH / CHUNK_WIDTH);
    int NUM_CHUNKS_REMAINING = 0;
    int disp = 0;
    FILE* img;
    Complex c;
    int pixel_YX[2];
    /** Timing variables */
    double start, stop;
    float elapsed_time;

    /** MPI specific variables */
    MPI_Status status;
    MPI_Request request;
    int rankID, numProcs, numSlaves;

    /** Initialisation of MPI environment */
    MPI_Init(&argc, &argv);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rankID);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Datatype CHUNKxCHUNK, CHUNKxCHUNK_RE;

    /** Variables for type creation */
    int full_sizes[2] = {FULL_WIDTH, FULL_WIDTH};
    int sub_sizes[2] = {CHUNK_WIDTH, CHUNK_WIDTH};
    int starting[2] = {0, 0};
    int sendcounts[numProcs];
    int displs[numProcs];

    /** Create CHUNK by CHUNK type */
    MPI_Type_create_subarray(
        2, // Dimensions
        full_sizes, // Full size
        sub_sizes, // Interior sizes
        starting, // Begining offset
        MPI_ORDER_C, // Ordering in mem
        MPI_INT, // Data type
        &CHUNKxCHUNK
    );
    /** Specify offset */
    MPI_Type_create_resized(
        CHUNKxCHUNK,
        0, // Lower-boumd
        CHUNK_WIDTH * sizeof(int), // Upper-bound (starting offset of next block)
        &CHUNKxCHUNK_RE
    );
    /** Commit type to be used */
    MPI_Type_commit(&CHUNKxCHUNK_RE);

    /** Hardcode constant */
    c.re = -.4;
    c.im = .6;

    send_arr = (int  *)malloc(CHUNK_WIDTH * CHUNK_WIDTH * sizeof(int));

    /** Master process create full array and initial file IO */
    if(rankID == 0) {
        if(rankID == 0)
            printf("Runtime Stats:\n\tNum Procs:\t%d\n\n", numProcs);

        full_arr = (int  *)malloc(FULL_WIDTH * FULL_WIDTH * sizeof(int));

        for(i = 0; i < FULL_WIDTH * FULL_WIDTH; i++)
            full_arr[i] = 5; // RANDOM VALUE

        img = fopen("image_out.ppm", "w");

        if(img == NULL) {
            printf("Could not open handle to image\n");
            return 1;
        }

        fprintf(img, "P6\n%d %d 255\n", FULL_WIDTH, FULL_WIDTH);
    }

    /** Start MPI timer */
    start = MPI_Wtime();

    /** Grid-stride loop */
    for(LOOPCOUNT = 0; LOOPCOUNT < NUM_CHUNKS; LOOPCOUNT += numProcs) {
        /** Work out chunk info */
        CUR_CHUNK = LOOPCOUNT + rankID;
        CHUNK_SQUARED = CHUNK_WIDTH * CHUNK_WIDTH;

        if(CUR_CHUNK > NUM_CHUNKS)
            CHUNK_SQUARED = 0;

#ifdef DEBUG
        printf("Proc %d\tJob: Process [# %d]\n", rankID, CUR_CHUNK, CHUNK_SQUARED);
#endif

        /** Pixel co-ord based on chunk number */
        pixel_YX[0] = (CUR_CHUNK / (FULL_WIDTH / CHUNK_WIDTH)) * CHUNK_WIDTH;
        pixel_YX[1] = (CUR_CHUNK % (FULL_WIDTH / CHUNK_WIDTH)) * CHUNK_WIDTH;

        /** Iterate over equation for each pixel in chunk */
        for(i = 0; i < CHUNK_WIDTH; i++) {
            for(j = 0; j < CHUNK_WIDTH; j++) {
                send_arr[(i * CHUNK_WIDTH) + j] = iterator(
                                                      c,
                                                      -(((pixel_YX[0] + i) - (FULL_WIDTH / 2)) / (double) FULL_WIDTH) * 2,
                                                      (((pixel_YX[1] + j) - (FULL_WIDTH / 2)) / (double) FULL_WIDTH) * 2
                                                  );
            }
        }

        /** Calculate displacements */
        if(rankID == 0) {
            for(i = 0; i < numProcs; i++) {
                if((disp + i) % (FULL_WIDTH / CHUNK_WIDTH) == 0 && disp + i != 0)
                    disp += (FULL_WIDTH / CHUNK_WIDTH) * (CHUNK_WIDTH - 1);

                displs[i] = disp + i;
                sendcounts[i] = CUR_CHUNK + i > NUM_CHUNKS ? 0 : CHUNK_WIDTH * CHUNK_WIDTH;
            }

            disp += numProcs;
        }

        /** Group comms */
        MPI_Gatherv(
            send_arr,
            CHUNK_SQUARED,
            MPI_INT,
            full_arr,
            sendcounts,
            displs,
            CHUNKxCHUNK_RE,
            0,
            MPI_COMM_WORLD
        );
#ifdef DEBUG

        if(rankID == 0)
            printf("Proc: MA\tJob: Gather [# %d]\n", CUR_CHUNK / numProcs);

#endif
    }

    /** End elapsed time */
    stop = MPI_Wtime();
    elapsed_time = stop - start;

    /** Master process plot the image */
    if(rankID == 0) {
        printf("Algorithm completed for,\n\t%d * %d pixels\n\t%d maximum iterations\n\t\tin %f seconds.\n",
               FULL_WIDTH, FULL_WIDTH,
               MAX_ITER,
               elapsed_time);

        plot(full_arr, img);

        fclose(img);
        free(full_arr);
    }

    free(send_arr);

    /** MPI clean-up */
    MPI_Type_free(&CHUNKxCHUNK_RE);
    MPI_Finalize();
    fflush(stdout);

    /** Successful return */
    return 0;
}

/**
Iterating function
*/
long iterator(Complex c, double im, double re)
{
    Complex z;
    long itCount = 0;

    z.re = re;
    z.im = im;

    for(; itCount < MAX_ITER; itCount++) {
        z = cmplx_add(cmplx_squared(z), c);

        if(cmplx_magnitude(z) > 4)
            break;
    }

    return itCount + 1;
}

/**
Function which calculates the pixel values from the square array
*/
void plot(int* full_arr, FILE* img)
{
    int i, j;
    unsigned char line[3 * FULL_WIDTH];

    for(i = 0; i < FULL_WIDTH; i++) {
        for(j = 0; j < FULL_WIDTH; j++) {
            if (*(full_arr + j + (i * FULL_WIDTH)) <= 63) {
                line[3 * j] = 255;
                line[3 * j + 1] = line[3 * j + 2] =
                                      255 - 4 * *(full_arr + j + (i * FULL_WIDTH));
            } else {
                line[3 * j] = 255;
                line[3 * j + 1] = *(full_arr + j + (i * FULL_WIDTH)) - 63;
                line[3 * j + 2] = 0;
            }

            if (*(full_arr + j + (i * FULL_WIDTH)) == 320)
                line[3 * j] = line[3 * j + 1] = line[3 * j + 2] = 255;
        }

        fwrite(line, 1, 3 * FULL_WIDTH, img);
    }
}

