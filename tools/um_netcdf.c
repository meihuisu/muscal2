/*
 * um_netcdf.c
 * nc=netcdf
*/

#include "um_netcdf.h"

/* Open file (read-only) */
int open_nc(const char* path) {
    int ncid = -1;
    NC_CHECK(nc_open(path, NC_NOWRITE, &ncid));
    return ncid;
}
 
/* Get variable ID by name */
int get_nc_varid(int ncid, const char* varname, const char *path) { 
    int varid = -1;
    int status = nc_inq_varid(ncid, varname, &varid);
    if (status != NC_NOERR) {
        fprintf(stderr, "Variable '%s' not found in %s: %s\n",
                varname, path, nc_strerror(status));
        nc_close(ncid);
        return EXIT_FAILURE;
    }
    return varid;
}


/* Inquire variable metadata */
int get_nc_var(int ncid, int varid, nc_type *vtype, int *ndims, int **dimids, size_t **dimlens) { 
    int nndims = 0;
    int natts = 0;
    NC_CHECK(nc_inq_var(ncid, varid, NULL, vtype, &nndims, NULL, &natts));
    if (nndims <= 0) {
        fprintf(stderr, "Variable has no dimensions (scalar). Reading scalar...\n");
    }
    /* Get dimension sizes */
    int *ndimids = (int *)malloc(sizeof(int) * (nndims > 0 ? nndims : 1));
    if (!ndimids) {
        fprintf(stderr, "Out of memory allocating dimids\n");
        nc_close(ncid);
        return EXIT_FAILURE;
    }

    if (nndims > 0) {
        NC_CHECK(nc_inq_var(ncid, varid, NULL, NULL, NULL, ndimids, NULL));
    }
//fprintf(stderr," ndimids is .. %d %d %d\n", ndimids[0], ndimids[1], ndimids[2]);

    size_t *ndimlens = (size_t *)malloc(sizeof(size_t) * (nndims > 0 ? nndims : 1));
    if (!*ndimlens) {
        fprintf(stderr, "Out of memory allocating *dimlens\n");
        free(ndimids);
        nc_close(ncid);
        return EXIT_FAILURE;
    }

    size_t nelems = 1;
    for (int i = 0; i < nndims; ++i) {
        size_t len;
        NC_CHECK(nc_inq_dimlen(ncid, ndimids[i], &len));
        ndimlens[i] = len;
        nelems *= len;
    }

    *dimlens = ndimlens;
    *dimids = ndimids;
    *ndims = nndims;
    return nelems;
}


// offset= (dep_idx)*(lat_cnt * lon_cnt)+(lat_idx)*(lon_cnt)+lon_idx
int print_nc_buffer_offset(nc_type vtype, int offset, void *buffer) {

    switch (vtype) {
        case NC_BYTE:
        case NC_UBYTE:
            printf("%u ", ((unsigned char*)buffer)[offset]);
            break;
        case NC_CHAR:
            /* Print as characters; for strings, format may vary */
            printf("%c", ((char*)buffer)[offset]);
            break;
        case NC_SHORT:
            printf("%d ", ((short*)buffer)[offset]);
            break;
        case NC_USHORT:
            printf("%u ", ((unsigned short*)buffer)[offset]);
            break;
        case NC_INT:
            printf("%d ", ((int*)buffer)[offset]);
            break;
        case NC_UINT:
            printf("%u ", ((unsigned int*)buffer)[offset]);
            break;
        case NC_INT64:
            printf("%lld ", ((long long*)buffer)[offset]);
            break;
        case NC_UINT64:
            printf("%llu ", ((unsigned long long*)buffer)[offset]);
            break;
        case NC_FLOAT:
            printf("%g ", ((float*)buffer)[offset]);
            break;
        case NC_DOUBLE:
            printf("%g ", ((double*)buffer)[offset]);
            break;
        default:
            /* already handled earlier */
            break;
    }
}

// e_dimlens,  expected dimlens
void *get_nc_buffer(int ncid, char *varname, const char *path, nc_type *vtype, size_t *nelems, int e_dimlens) { 
    int varid=-1;
    int ndims = 0;
    int natts = 0;
    size_t nnelems = 1;
    int *dimids=0;
    size_t *dimlens=0;
    void *buffer = NULL;
    size_t elem_size = 0;
    nc_type nvtype;

    varid=get_nc_varid(ncid,varname,path);
    nnelems =get_nc_var(ncid, varid, &nvtype, &ndims, &dimids, &dimlens);
    // ndims should be 1 or 3
    if(ndims != e_dimlens) {
        fprintf(stderr," Fail to extract %s data\n",varname);
        goto cleanup;
    }
    /* grab the list */
    switch ( nvtype ) {
        case NC_BYTE:
        case NC_UBYTE:
            //fprintf(stderr," buffer of NC_BYTE or NC_UBYTE \n");
            elem_size = sizeof(unsigned char);
            buffer = malloc(nnelems * elem_size);
            if (!buffer) { fprintf(stderr, "malloc failed\n"); goto cleanup; }
            NC_CHECK(nc_get_var_uchar(ncid, varid, (unsigned char*)buffer));
            break;

        case NC_CHAR:
            /* NC_CHAR often represents character arrays / strings */
            //fprintf(stderr," buffer of NC_CHAR \n");
            elem_size = sizeof(char);
            buffer = malloc(nnelems * elem_size);
            if (!buffer) { fprintf(stderr, "malloc failed\n"); goto cleanup; }
            NC_CHECK(nc_get_var_text(ncid, varid, (char*)buffer));
            break;

        case NC_SHORT:
            //fprintf(stderr," buffer of NC_SHORT \n");
            elem_size = sizeof(short);
            buffer = malloc(nnelems * elem_size);
            if (!buffer) { fprintf(stderr, "malloc failed\n"); goto cleanup; }
            NC_CHECK(nc_get_var_short(ncid, varid, (short*)buffer));
            break;

        case NC_USHORT:
            //fprintf(stderr," buffer of NC_USHORT \n");
            elem_size = sizeof(unsigned short);
            buffer = malloc(nnelems * elem_size);
            if (!buffer) { fprintf(stderr, "malloc failed\n"); goto cleanup; }
            NC_CHECK(nc_get_var_ushort(ncid, varid, (unsigned short*)buffer));
            break;

        case NC_INT:
            //fprintf(stderr," buffer of NC_INT \n");
            elem_size = sizeof(int);
            buffer = malloc(nnelems * elem_size);
            if (!buffer) { fprintf(stderr, "malloc failed\n"); goto cleanup; }
            NC_CHECK(nc_get_var_int(ncid, varid, (int*)buffer));
            break;

        case NC_UINT:
            //fprintf(stderr," buffer of NC_UINT \n");
            elem_size = sizeof(unsigned int);
            buffer = malloc(nnelems * elem_size);
            if (!buffer) { fprintf(stderr, "malloc failed\n"); goto cleanup; }
            NC_CHECK(nc_get_var_uint(ncid, varid, (unsigned int*)buffer));
            break;

        case NC_INT64:
            //fprintf(stderr," buffer of NC_INT64 \n");
            elem_size = sizeof(long long);
            buffer = malloc(nnelems * elem_size);
            if (!buffer) { fprintf(stderr, "malloc failed\n"); goto cleanup; }
            NC_CHECK(nc_get_var_longlong(ncid, varid, (long long*)buffer));
            break;

        case NC_UINT64:
            //fprintf(stderr," buffer of NC_UINT64 \n");
            elem_size = sizeof(unsigned long long);
            buffer = malloc(nnelems * elem_size);
            if (!buffer) { fprintf(stderr, "malloc failed\n"); goto cleanup; }
            NC_CHECK(nc_get_var_ulonglong(ncid, varid, (unsigned long long*)buffer));
            break;

        case NC_FLOAT:
            //fprintf(stderr,"   buffer of NC_FLOAT \n");
            elem_size = sizeof(float);
            buffer = malloc(nnelems * elem_size);
            //fprintf(stderr,"   needs %d \n",nnelems * elem_size);
            if (!buffer) { fprintf(stderr, "malloc failed\n"); goto cleanup; }
            NC_CHECK(nc_get_var_float(ncid, varid, (float*)buffer));
            break;

        case NC_DOUBLE:
            //fprintf(stderr," buffer of NC_DOUBLE \n");
            elem_size = sizeof(double);
            buffer = malloc(nnelems * elem_size);
            if (!buffer) { fprintf(stderr, "malloc failed\n"); goto cleanup; }
            NC_CHECK(nc_get_var_double(ncid, varid, (double*)buffer));
            break;

        default:
            fprintf(stderr, "Unsupported variable type (type id=%d)\n", nvtype);
            goto cleanup;
    }

    /* Print some information and sample values */
    if(debug) {
        printf("\nFile: %s\n", path);
        printf("  Var name: %s\n", varname);
        printf("  Type: %d\n", (int)nvtype);
        printf("  Dimensions: %d\n", ndims);
        for (int i = 0; i < ndims; ++i) {
            char dname[NC_MAX_NAME + 1];
            NC_CHECK(nc_inq_dimname(ncid, dimids[i], dname));
            printf("     dim[%d] name=%s len=%zu\n", i, dname, dimlens[i]);
        }
        printf("  Total elements: %zu\n", nnelems);
    }

cleanup: 
    if(dimids) free(dimids);
    if(dimlens) free(dimlens);
    * vtype = nvtype;
    * nelems = nnelems;
    return buffer;
}

// e_dimlens,  expected dimlens
float *get_nc_float_buffer(int ncid, char *varname, const char *path, nc_type *vtype, size_t *nelems, int e_dimlens) { 
    int varid=-1;
    int ndims = 0;
    int natts = 0;
    size_t nnelems = 1;
    int *dimids=0;
    size_t *dimlens=0;
    float *buffer = NULL;
    size_t elem_size = 0;
    nc_type nvtype;

    varid=get_nc_varid(ncid,varname,path);
    nnelems =get_nc_var(ncid, varid, &nvtype, &ndims, &dimids, &dimlens);
    // ndims should be 1 or 3
    if(ndims != e_dimlens) {
        fprintf(stderr," Fail to extract %s data\n",varname);
        goto cleanup;
    }


    /* get list from external file */
    switch ( nvtype ) {
        case NC_BYTE:
        case NC_UBYTE:
            if(debug) printf("\nBuffer of NC_BYTE or NC_UBYTE");
            break;

        case NC_CHAR:
            if(debug) printf("\nBuffer of NC_CHAR");
            break;

        case NC_SHORT:
            if(debug) printf("\nBuffer of NC_SHORT");
            break;

        case NC_USHORT:
            if(debug) printf("\nBuffer of NC_USHORT");
            break;

        case NC_INT:
            if(debug) printf("\nBuffer of NC_INT");
            break;

        case NC_UINT:
            if(debug) printf("\nBuffer of NC_UINT");
            break;

        case NC_INT64:
            if(debug) printf("\nBuffer of NC_INT64 => NC_FLOAT");
            break;

        case NC_UINT64:
            if(debug) printf("\nBuffer of NC_UINT64");
            break;

        case NC_FLOAT:
            if(debug) printf("\nBuffer of NC_FLOAT");
            break;

        case NC_DOUBLE:
            if(debug) printf("\nBuffer of NC_DOUBLE => NC_FLOAT");
            break;

        default:
            fprintf(stderr, "Unsupported variable type (type id=%d)\n", nvtype);
            goto cleanup;
    }

    elem_size = sizeof(float);
    buffer = malloc(nnelems * elem_size);
    if (!buffer) { fprintf(stderr, "malloc failed\n"); goto cleanup; }
    NC_CHECK(nc_get_var_float(ncid, varid, (float*)buffer));

    /* Print some information and sample values */
    if(debug) {
        printf("\n  File: %s\n", path);
        printf("  Var name: %s\n", varname);
        printf("  Original Type: %d\n", (int)nvtype);
        printf("  Dimensions: %d\n", ndims);
        for (int i = 0; i < ndims; ++i) {
            char dname[NC_MAX_NAME + 1];
            NC_CHECK(nc_inq_dimname(ncid, dimids[i], dname));
            printf("     dim[%d] name=%s len=%zu\n", i, dname, dimlens[i]);
        }
        printf("  Total elements: %zu\n", nnelems);
    }

cleanup: 
    if(dimids) free(dimids);
    if(dimlens) free(dimlens);
    * vtype = nvtype;
    * nelems = nnelems;
    return buffer;
}


float get_nc_vara_float(int ncid, int varid, int dep_idx, int lat_idx, int lon_idx) {
// depth, lat, lon
  size_t start[] = {dep_idx, lat_idx, lon_idx};
  size_t count[] = {1, 1, 1};

  float val;
  int status = nc_get_vara_float(ncid, varid, start, count, &val);
  if (status != NC_NOERR) { fprintf(stderr, "netCDF error: %s\n", nc_strerror(status)); }

return val;
}


// improve access speed
// vertical = same lat-idx, same lon-idx,  z varies
// horizontal= all lat-idx, all-lon-idx,  same z
// dep profile = one lat-idx, one lon-idx,  z varies
//
int cache_depth_col_float(int ncid, int varid, 
    size_t ndepth, size_t lat_idx, size_t lon_idx,
    float *col /* size >= ndepth */) {

    size_t start[3] = {0, lat_idx, lon_idx};
    size_t count[3] = {ndepth, 1, 1};

    int status = nc_get_vara_float(ncid, varid, start, count, col);
    if (status != NC_NOERR) {
        fprintf(stderr, "netCDF error (cache_depth_col_float): %s\n",
                nc_strerror(status));
        return status;
    }
    return NC_NOERR;
}

// ny = n(logitude), ny = n(lattidue)
int cache_latlon_layer_float(int ncid, int varid,
                size_t dep_idx, size_t ny, size_t nx,
                float *layer /* size >= ny*nx */)
{
 if(debug) {fprintf(stderr," layer, calling layer_float %d %d (%d) \n", ny, nx, (ny * nx)); }
 if(debug) {fprintf(stderr," layer, using dep_idx %d\n", dep_idx); }

    size_t start[] = {dep_idx, 0, 0};
    size_t count[] = {1, ny, nx};
    int status = nc_get_vara_float(ncid, varid, start, count, layer);

    if (status != NC_NOERR) {
        fprintf(stderr, "netCDF error (cache_latlon_layer_float): %s\n", nc_strerror(status));
        return status;
    }
    return NC_NOERR;
}

