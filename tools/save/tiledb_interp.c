/**
   tiledb_interp.c

To perform bilinear or trilinear interpolation on a dense dataset, 
you first need to map your physical coordinates (e.g., Depth in meters,
Lat/Lon in degrees) to the continuous floating-point index space of 
your TileDB array.

Once you have the fractional index, you grab the 8 surrounding integer 
corner points (for 3D trilinear) or 4 surrounding points (for 2D bilinear)
to compute the weighted average.

Here is the complete C code to query the exact neighboring cells from 
TileDB and perform trilinear interpolation.

----

Crucial Implementation Notes
Row-Major Memory Mapping: When reading a 2×2×2 bounding box via TileDB using 
TILEDB_ROW_MAJOR, the returned 1D array indices correspond perfectly to these
spatial locations:
cube_data[0] = Top-Left-Front Neighbor
cube_data[7] = Bottom-Right-Back Neighbor

Edge Cases (Out of Bounds): If your target physical coordinate sits exactly
on the maximum border of your dataset, d1, lt1, or ln1 will exceed the array 
bounds. In a production pipeline, wrap your indices in a clamping function:

if (d1 >= 210) d1 = 209; // Keep inside array bounds

Performance Optimization: Instead of hitting the disk using a separate TileDB 
query for every single coordinate evaluation, you should load a larger target 
slice profile into RAM once, and then run this arithmetic mapping logic inside 
your loop directly over the local pointer matrix.

**/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <tiledb/tiledb.h>

// Dataset grid specifications (from your NetCDF metadata)
#define ORIGIN_DEPTH 0.0          // Example starting values
#define ORIGIN_LAT   34.0
#define ORIGIN_LON   -118.0

#define STEP_DEPTH   5.0          // Assume variable or fixed dz
#define STEP_LAT     0.01         // From your filename: dll0.01
#define STEP_LON     0.01

// Linear interpolation helper
float lerp(float v0, float v1, float t) {
    return (1.0f - t) * v0 + t * v1;
}

// 3D Trilinear Interpolation helper
float trilinear_interpolate(float c000, float c100, float c010, float c110,
                              float c001, float c101, float c011, float c111,
                              float x, float y, float z) {
    float c00 = lerp(c000, c100, x);
    float c10 = lerp(c010, c110, x);
    float c01 = lerp(c001, c101, x);
    float c11 = lerp(c011, c111, x);

    float c0 = lerp(c00, c10, y);
    float c1 = lerp(c01, c11, y);

    return lerp(c0, c1, z);
}

int main() {
    // --- TARGET COORDINATES ---
    double target_depth = 12.3;     // Falls between index 3 and 4
    double target_lat   = 34.054;   // Falls between index 6 and 7
    double target_lon   = -117.932; // Falls between index 7 and 8

    // 1. Convert Physical Coordinates to Continuous Array Space (0-indexed)
    double f_depth = (target_depth - ORIGIN_DEPTH) / STEP_DEPTH;
    double f_lat   = (target_lat - ORIGIN_LAT) / STEP_LAT;
    double f_lon   = (target_lon - ORIGIN_LON) / STEP_LON;

    // 2. Find the bounding corner indices
    int d0 = (int)floor(f_depth); int d1 = d0 + 1;
    int lt0 = (int)floor(f_lat);   int lt1 = lt0 + 1;
    int ln0 = (int)floor(f_lon);   int ln1 = ln0 + 1;

    // Calculate Interpolation Weights (fractional distances)
    float wd = f_depth - d0;
    float wlt = f_lat - lt0;
    float wln = f_lon - ln0;

    // 3. Map to TileDB 1-based indexing structure
    int32_t subarray[] = {
        d0 + 1, d1 + 1,     // Depth bounding box
        lt0 + 1, lt1 + 1,   // Latitude bounding box
        ln0 + 1, ln1 + 1    // Longitude bounding box
    };

    // --- TILEDB ACCESSS ---
    tiledb_ctx_t* ctx;
    tiledb_ctx_alloc(NULL, &ctx);
    tiledb_array_t* array;
    tiledb_array_alloc(ctx, "muscal_canvas_tiledb_array", &array);
    tiledb_array_open(ctx, array, TILEDB_READ);

    // Subarray is a 2x2x2 cube = 8 points total
    size_t buffer_size = 8 * sizeof(float);
    float* cube_data = (float*)malloc(buffer_size);

    tiledb_query_t* query;
    tiledb_query_alloc(ctx, array, TILEDB_READ, &query);
    tiledb_query_set_layout(ctx, query, TILEDB_ROW_MAJOR);
    tiledb_query_set_subarray(ctx, query, subarray);
    tiledb_query_set_data_buffer(ctx, query, "rho", cube_data, &buffer_size);

    tiledb_query_submit(ctx, query);

    // --- 4. EXTRACT NEIGHBORS & INTERPOLATE ---
    // Because TileDB returns data in Row-Major order (Depth -> Lat -> Lon),
    // the 8 elements in cube_data map directly to binary corner offsets:
    float c000 = cube_data[0]; // [d0, lt0, ln0]
    float c001 = cube_data[1]; // [d0, lt0, ln1]
    float c010 = cube_data[2]; // [d0, lt1, ln0]
    float c011 = cube_data[3]; // [d0, lt1, ln1]
    float c100 = cube_data[4]; // [d1, lt0, ln0]
    float c101 = cube_data[5]; // [d1, lt0, ln1]
    float c110 = cube_data[6]; // [d1, lt1, ln0]
    float c111 = cube_data[7]; // [d1, lt1, ln1]

    // Compute Trilinear Interpolation
    float interpolated_rho = trilinear_interpolate(
        c000, c100, c010, c110,
        c001, c101, c011, c111,
        wd, wlt, wln
    );

    // Default No-Interpolation (Nearest Neighbor fallback)
    float nearest_rho = cube_data[(wd > 0.5 ? 4 : 0) + (wlt > 0.5 ? 2 : 0) + (wln > 0.5 ? 1 : 0)];

    // Print Results
    printf("Target Location: Depth=%.2f, Lat=%.3f, Lon=%.3f\n", target_depth, target_lat, target_lon);
    printf("Nearest Neighbor Value (No Interpolation): %f kg/m^3\n", nearest_rho);
    printf("Trilinear Interpolated Value             : %f kg/m^3\n", interpolated_rho);

    // Clean up
    free(cube_data);
    tiledb_query_free(&query);
    tiledb_array_close(ctx, array);
    tiledb_array_free(&array);
    tiledb_ctx_free(&ctx);

    return 0;
}

