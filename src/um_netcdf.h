#ifndef UM_NETCDF_H
#define UM_NETCDF_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <math.h>
#include <netcdf.h>

extern int debug;

/* Simple error-handling macro */
#define NC_CHECK(status)                                        \
    do {                                                        \
        if ((status) != NC_NOERR) {                             \
            fprintf(stderr, "NetCDF error: %s\n",               \
                    nc_strerror(status));                       \
            exit(EXIT_FAILURE);                                 \
        }                                                       \
    } while (0)


int open_nc(const char* path);
int get_nc_varid(int ncid, const char* varname, const char* path);
int get_nc_var(int ncid, int varid, nc_type *vtype, int *ndims, int **dimids, size_t **dimlens);

int print_nc_buffer_offset(nc_type vtype, int offset, void *buffer);
void *get_nc_buffer(int ncid, char *varname, const char *path, nc_type *vtype, size_t *nelems, int e_dimlens);
float *get_nc_float_buffer(int ncid, char *varname, const char *path, nc_type *vtype, size_t *nelems, int e_dimlens);
float get_nc_vara_float(int ncid, int varid, int dep_idx, int lat_idx, int lon_idx);

int cache_depth_col_float(int ncid, int varid,
                size_t ndepth, size_t lat_idx, size_t lon_idx,
                float *col /* size >= ndepth */);

int cache_latlon_layer_float(int ncid, int varid,
                size_t dep_idx, size_t ny, size_t nx,
                float *layer /* size >= ny*nx */);

#endif



