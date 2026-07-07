#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <tiledb/tiledb.h>

// Helper function to find the closest index in a coordinate array
int find_nearest_index(const float* array, int length, float target) {
    int min_idx = 0;
    float min_diff = fabsf(array[0] - target);
    for (int i = 1; i < length; i++) {
        float diff = fabsf(array[i] - target);
        if (diff < min_diff) {
            min_diff = diff;
            min_idx = i;
        }
    }
    return min_idx;
}

int main() {
    const char* group_uri = "convert_tiledb_array";
    
    // Target coordinate values to search for
    float target_depth = 500.0f;
    float target_lat = 35.5f;
    float target_lon = -120.0f;

    // Initialize TileDB Context
    tiledb_ctx_t* ctx = NULL;
    tiledb_ctx_alloc(NULL, &ctx);

    // ==========================================
    // 1. READ COORDINATES FROM GROUP METADATA
    // ==========================================
    tiledb_group_t* group = NULL;
    tiledb_group_alloc(ctx, group_uri, &group);
    tiledb_group_open(ctx, group, TILEDB_READ);

    // Pointers to hold our coordinate arrays loaded from metadata
    float *depth_coords = NULL, *lat_coords = NULL, *lon_coords = NULL;
    tiledb_datatype_t type;
    uint32_t num_values;
    const void* value_ptr;

    // Load Depth coordinates
    tiledb_group_get_metadata(ctx, group, "depth_coords", &type, &num_values, &value_ptr);
    int depth_len = num_values;
    depth_coords = (float*)malloc(depth_len * sizeof(float));
    memcpy(depth_coords, value_ptr, depth_len * sizeof(float));

    // Load Latitude coordinates
    tiledb_group_get_metadata(ctx, group, "latitude_coords", &type, &num_values, &value_ptr);
    int lat_len = num_values;
    lat_coords = (float*)malloc(lat_len * sizeof(float));
    memcpy(lat_coords, value_ptr, lat_len * sizeof(float));

    // Load Longitude coordinates
    tiledb_group_get_metadata(ctx, group, "longitude_coords", &type, &num_values, &value_ptr);
    int lon_len = num_values;
    lon_coords = (float*)malloc(lon_len * sizeof(float));
    memcpy(lon_coords, value_ptr, lon_len * sizeof(float));

    tiledb_group_close(ctx, group);
    tiledb_group_free(&group);

    // ==========================================
    // 2. COMPUTE MULTI-DIMENSIONAL SUB-GRID RANGES
    // ==========================================
    int idx_d = find_nearest_index(depth_coords, depth_len, target_depth);
    int idx_lat = find_nearest_index(lat_coords, lat_len, target_lat);
    int idx_lon = find_nearest_index(lon_coords, lon_len, target_lon);

    // Safeguard index boundaries for a 2x2x2 neighborhood lookup
    if (idx_d >= depth_len - 1) idx_d = depth_len - 2;
    if (idx_lat >= lat_len - 1) idx_lat = lat_len - 2;
    if (idx_lon >= lon_len - 1) idx_lon = lon_len - 2;

    printf("Nearest Grid Origin: Depth Index=%d, Lat Index=%d, Lon Index=%d\n", idx_d, idx_lat, idx_lon);

    // Define the bounding range for our query (inclusive: [start, end])
    int32_t subarray_range[] = {
        idx_d,   idx_d + 1,    // depth range
        idx_lat, idx_lat + 1,  // latitude range
        idx_lon, idx_lon + 1   // longitude range
    };

    // ==========================================
    // 3. SLICE THE 2x2x2 BLOCK FROM AN ARRAY (e.g., vp)
    // ==========================================
    char array_uri[512];
    snprintf(array_uri, sizeof(array_uri), "%s/vp", group_uri);

    tiledb_array_t* array = NULL;
    tiledb_array_alloc(ctx, array_uri, &array);
    tiledb_array_open(ctx, array, TILEDB_READ);

    // Allocate buffer to hold the 8 resulting float values (2 * 2 * 2 = 8)
    float vp_data_buffer[8];
    uint64_t buffer_size = sizeof(vp_data_buffer);

    // Setup the Query
    tiledb_query_t* query = NULL;
    tiledb_query_alloc(ctx, array, TILEDB_READ, &query);
    tiledb_query_set_layout(ctx, query, TILEDB_ROW_MAJOR);
    tiledb_query_set_subarray(ctx, query, subarray_range);
    
    // Bind output buffer to the data attribute ("vp")
    tiledb_query_set_data_buffer(ctx, query, "vp", vp_data_buffer, &buffer_size);

    // Submit the query to execute the disk read
    tiledb_query_submit(ctx, query);

    // Close down resources cleanly
    tiledb_query_free(&query);
    tiledb_array_close(ctx, array);
    tiledb_array_free(&array);

    // ==========================================
    // 4. PRINT RESULTS FOR TRILINEAR INTERPOLATION
    // ==========================================
    printf("\n--- Retrieved 2x2x2 VP Matrix Data ---\n");
    int b = 0;
    for (int d = 0; d < 2; d++) {
        for (int lt = 0; lt < 2; lt++) {
            for (int ln = 0; ln < 2; ln++) {
                printf("vp[d+%d][lat+%d][lon+%d] = %.2f m/s\n", d, lt, ln, vp_data_buffer[b++]);
            }
        }
    }

    // Clean up memory leaks
    free(depth_coords);
    free(lat_coords);
    free(lon_coords);
    tiledb_ctx_free(&ctx);
    return 0;
}

