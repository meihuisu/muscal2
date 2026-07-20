#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <tiledb/tiledb.h>

typedef struct {
    int32_t nearest_idx;
    float fraction;
} LookupResult;

// Core mathematical mapping function (Direction-Agnostic)
LookupResult find_nearest_and_fraction(const float* coord_array, uint32_t total_elements, float target_value) {
    LookupResult res = {0, 0.0f};
    
    if (total_elements <= 1) {
        res.nearest_idx = 0;
        res.fraction = 0.0f;
        return res;
    }
    
    int ascending = (coord_array[total_elements - 1] > coord_array[0]);
    float min_val = ascending ? coord_array[0] : coord_array[total_elements - 1];
    float max_val = ascending ? coord_array[total_elements - 1] : coord_array[0];

    if (target_value <= min_val) {
        res.nearest_idx = ascending ? 0 : (int32_t)(total_elements - 1);
        res.fraction = 0.0f;
        return res;
    }
    if (target_value >= max_val) {
        res.nearest_idx = ascending ? (int32_t)(total_elements - 1) : 0;
        res.fraction = 0.0f;
        return res;
    }

    for (uint32_t i = 0; i < total_elements - 1; i++) {
        float v0 = coord_array[i];
        float v1 = coord_array[i + 1];
        
        if ((target_value >= v0 && target_value <= v1) || (target_value <= v0 && target_value >= v1)) {
            float cell_width = v1 - v0;
            float raw_fraction = (target_value - v0) / cell_width;
            
            if (raw_fraction <= 0.5f) {
                res.nearest_idx = (int32_t)i;
                res.fraction = raw_fraction;
            } else {
                res.nearest_idx = (int32_t)(i + 1);
                res.fraction = raw_fraction - 1.0f; 
            }
            return res;
        }
    }
    return res;
}

int main() {
    const char* tiledb_array_name = "muscal_canvas.tdb";

    // --- Target Inputs ---
    float input_depth = 45.2f;   // in meters
    float input_lat   = 34.0025f; // in degrees
    float input_lon   = -118.241f; // in degrees

    tiledb_ctx_t* ctx = NULL;
    tiledb_array_t* array = NULL;

    tiledb_ctx_alloc(NULL, &ctx);
    tiledb_array_alloc(ctx, tiledb_array_name, &array);
    if (tiledb_array_open(ctx, array, TILEDB_READ) != TILEDB_OK) {
        fprintf(stderr, "[ERROR] Unable to open TileDB array for reading.\n");
        if (array) tiledb_array_free(&array);
        if (ctx) tiledb_ctx_free(&ctx);
        return -1;
    }

    // =========================================================
    // STEP 1: CONVERT PHYSICAL COORDS TO ARRAY INDICES
    // =========================================================
    tiledb_datatype_t val_type;
    uint32_t dep_num = 0, lat_num = 0, lon_num = 0;
    const void *dep_ptr = NULL, *lat_ptr = NULL, *lon_ptr = NULL;

    tiledb_array_get_metadata(ctx, array, "coords_depth", &val_type, &dep_num, &dep_ptr);
    tiledb_array_get_metadata(ctx, array, "coords_latitude", &val_type, &lat_num, &lat_ptr);
    tiledb_array_get_metadata(ctx, array, "coords_longitude", &val_type, &lon_num, &lon_ptr);

    if (!dep_ptr || !lat_ptr || !lon_ptr) {
        fprintf(stderr, "[ERROR] Coordinate metadata missing from TileDB array.\n");
        goto cleanup_array;
    }

    LookupResult dep_res = find_nearest_and_fraction((const float*)dep_ptr, dep_num, input_depth);
    LookupResult lat_res = find_nearest_and_fraction((const float*)lat_ptr, lat_num, input_lat);
    LookupResult lon_res = find_nearest_and_fraction((const float*)lon_ptr, lon_num, input_lon);

    // =========================================================
    // STEP 2: QUERY TILEDB FOR VP, VS, RHO AT NEAREST INDEX
    // =========================================================
    tiledb_query_t* query = NULL;
    tiledb_subarray_t* subarray = NULL;

    float val_vp = 0.0f, val_vs = 0.0f, val_rho = 0.0f;
    uint64_t bytes_vp = sizeof(float), bytes_vs = sizeof(float), bytes_rho = sizeof(float);

    tiledb_query_alloc(ctx, array, TILEDB_READ, &query);
    tiledb_subarray_alloc(ctx, array, &subarray);

    // Set the range for each dimension to exactly 1 point [start, end]
    int32_t dep_range[] = { dep_res.nearest_idx, dep_res.nearest_idx };
    int32_t lat_range[] = { lat_res.nearest_idx, lat_res.nearest_idx };
    int32_t lon_range[] = { lon_res.nearest_idx, lon_res.nearest_idx };

    // Assuming dimension 0 = depth, 1 = latitude, 2 = longitude
    tiledb_subarray_add_range(ctx, subarray, 0, &dep_range[0], &dep_range[1], NULL);
    tiledb_subarray_add_range(ctx, subarray, 1, &lat_range[0], &lat_range[1], NULL);
    tiledb_subarray_add_range(ctx, subarray, 2, &lon_range[0], &lon_range[1], NULL);
    
    tiledb_query_set_subarray_t(ctx, query, subarray);
    
    // Bind memory buffers to capture the output variables
    tiledb_query_set_data_buffer(ctx, query, "vp", &val_vp, &bytes_vp);
    tiledb_query_set_data_buffer(ctx, query, "vs", &val_vs, &bytes_vs);
    tiledb_query_set_data_buffer(ctx, query, "rho", &val_rho, &bytes_rho);

    // Execute the query
    if (tiledb_query_submit(ctx, query) != TILEDB_OK) {
        fprintf(stderr, "[ERROR] TileDB query failed.\n");
        goto cleanup_query;
    }

    // =========================================================
    // STEP 3: PRINT RESULTS
    // =========================================================
    printf("===================================================\n");
    printf("TILEDB GEOGRAPHIC MAPPING & DATA EXTRACTION\n");
    printf("===================================================\n");
    printf("Input Values     -> Depth: %.2f m | Lat: %.4f° | Lon: %.4f°\n", 
           input_depth, input_lat, input_lon);
    printf("---------------------------------------------------\n");
    printf("Mapped Indexes   -> Depth Idx: %5d | Lat Idx: %5d | Lon Idx: %5d\n", 
           dep_res.nearest_idx, lat_res.nearest_idx, lon_res.nearest_idx);
    printf("Cell Fractions   -> Depth Frac:%+.4f | Lat Frac:%+.4f | Lon Frac:%+.4f\n", 
           dep_res.fraction, lat_res.fraction, lon_res.fraction);
    printf("---------------------------------------------------\n");
    printf("Actual Node Pos  -> Depth: %.2f m | Lat: %.4f° | Lon: %.4f°\n",
           ((const float*)dep_ptr)[dep_res.nearest_idx], 
           ((const float*)lat_ptr)[lat_res.nearest_idx], 
           ((const float*)lon_ptr)[lon_res.nearest_idx]);
    printf("---------------------------------------------------\n");
    printf("Material Values  -> VP  : %.2f m/s\n", val_vp);
    printf("                    VS  : %.2f m/s\n", val_vs);
    printf("                    RHO : %.2f kg/m^3\n", val_rho);
    printf("===================================================\n");

cleanup_query:
    if (subarray) tiledb_subarray_free(&subarray);
    if (query) tiledb_query_free(&query);

cleanup_array:
    tiledb_array_close(ctx, array);
    tiledb_array_free(&array);
    tiledb_ctx_free(&ctx);

    return 0;
}
