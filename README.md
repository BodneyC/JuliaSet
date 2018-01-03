Julia Set - C
=============

### Description

This comes from a university project. This is a short C program which iterates over the classic z=z^2+c equation to compute a fractal set and writes out the 2d array to a .ppm image.

### Compilation

Clone the [repository](https://github.com/BodneyC/JuliaSet.git), create an `obj` and `src` directories, and compile the program with `make`.

```bash
git clone https://github.com/BodneyC/JuliaSet.git
cd JuliaSet/
mkdir -p src,obj
make
```

## Usage

There are three versions of this program, a serial version and two differently load balanced parallelised versions; in the parallel versions, the width of the image and the width of the chunk (inversely proportional to granularity) are hard-coded in `#define` statements at the top of the sources.

`fracFun_DYNAMIC.c` is the serial version of this program and its usage is:

    ./bin/fracfun_DYNAMIC [max_iterations] [real_part] [imaginary_part] [Optional: [X co-ord] [Y co-ord]]

`fracfun_CM` is the cyclically mapped version of the parallelised versions:

    mpirun [-np [0-9]] [-machinefile ./path/to/machine-file] ./bin/fracFun_CM

`fracfun_MS` is the client/server version of the parallelised versions:

    mpirun [-np [0-9]] [-machinefile ./path/to/machine-file] ./bin/fracFun_MS

