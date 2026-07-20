/*
 * muscal2_util.c
 * nc=netcdf
*/

#include "muscal2_util.h"

int debug=0;

/* open a binary file and extract total number of data out of it  */
float *get_binary_float_buffer(const char *datadir, char *datafile, int total) {

    int elem_size = sizeof(float);
    float *buffer = (float *) malloc(total * elem_size);
    if (!buffer) { fprintf(stderr, "malloc failed\n"); return NULL; }

    char filepath[256];
    sprintf(filepath, "%s/%s", datadir, datafile);
    if(debug) fprintf(stderr," data file ..%s\n", filepath);

    FILE *fp=fopen(filepath,"rb");
    size_t read_count = fread(buffer, sizeof(float), total, fp);
    if (read_count != total) {
        printf("Warning: read %zu elements (expected %d)\n", read_count, total);
    }
    fclose(fp);
    return buffer;
}

// find nearest buffer idx even if it is over it
int find_nearest_buffer_idx(float *buffer, size_t nelems, float target) {
    size_t lo = 0, hi = nelems; // search in [lo, hi)
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        float v = buffer[mid];
        // Treat NaNs as +infinity to keep ordering sane; skip if needed
        if (isnan(v)) { // move right past NaNs
            lo = mid + 1;
            continue;
        }
        if (v < target) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    // lo is the first index with a[lo] >= target (lower_bound)
    if (lo == 0) return 0;
    if (lo == nelems) return (int)(nelems - 1);

    // a[lo-1] or a[lo]
    float left  = buffer[lo - 1];
    float right = buffer[lo];

    int idx=(fabsf(left - target) <= fabsf(right - target)) ? (int)(lo - 1) : (int)lo;
    return idx;
}

// find  lowest array value that is lower than target
int find_buffer_idx(float *buffer, size_t nelems, float target) {
    if (nelems < 2) return -1;

    if (target < buffer[0] || target > buffer[nelems - 1]) return -1;

    int lo = 0, hi = nelems - 2;
    while (lo < hi) {
        int mid = (lo + hi + 1) / 2;
        if (buffer[mid] <= target) 
          lo = mid;
        else
          hi = mid - 1;
    }
    return lo;
}


/* Find bracketing cell index i such that grid[i] <= x <= grid[i+1].
   If x is outside, returns nearest edge cell index instead of erroring. */
int find_buffer_idx_clamped(float *buffer, size_t nelems, float target) {
    if (nelems < 2) return -1;

    if (target <= buffer[0]) {
        return 0;
    }
    if (target >= buffer[nelems - 1]) {
        return nelems - 2;
    }

    int lo = 0, hi = nelems - 1;
    while (hi - lo > 1) {
        int mid = lo + (hi - lo) / 2;
        if (buffer[mid] <= target) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    return lo;
}

// find the percent of target [0-1] within the cell
float find_cell_percent(float *buffer, float target, int idx) {

    float percent=(target - buffer[idx])/(buffer[idx+1]-buffer[idx]);
    if(percent < 0.0) percent=0;
    if(percent > 1.0) percent=1.0;
    return percent;
}


// should be 8 values 
void dump_corners(FILE *fp, float *corners) {
  fprintf(fp,"Corners(8): %lf %lf %lf %lf %lf %lf %lf %lf\n",
    corners[0], corners[1], corners[2], corners[3], corners[4], corners[5], corners[6], corners[7]);
}


// create a float array from a string of "{ 10.1, 2.5 ...}",
// remember to free it
float *float_array_it(const char *str, size_t *n){

    const char *p = str;
    char *end;
    size_t capacity = 8;
    size_t count = 0;

    float *arr = (float  *)malloc(capacity * sizeof(float));
    if (!arr) return NULL;

    // Skip whitespace
    while (isspace(*p)) p++;

    // Expect '{'
    if (*p != '{') {
        free(arr);
        return NULL;
    }
    p++;  // skip '{'

    while (1) {
        // Skip whitespace
        while (isspace(*p)) p++;

        // Check for end
        if (*p == '}') {
            p++;
            break;
        }

        // Parse float
        float val = strtof(p, &end);
        if (p == end) {
            free(arr);
            return NULL;  // parse error
        }

        // Append to array
        if (count == capacity) {
            capacity *= 2;
            float *tmp = realloc(arr, capacity * sizeof(float));
            if (!tmp) {
                free(arr);
                return NULL;
            }
            arr = tmp;
        }

        arr[count++] = val;
        p = end;

        // Skip whitespace
        while (isspace(*p)) p++;

        // Expect ',' or '}'
        if (*p == ',') {
            p++;
        } else if (*p == '}') {
            p++;
            break;
        } else {
            free(arr);
            return NULL;  // invalid delimiter
        }
    }

    *n = count;
    return arr;
}
