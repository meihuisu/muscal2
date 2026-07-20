
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tiledb/tiledb.h>

int main() {
    const char* tiledb_array_name = "muscal_canvas.tdb";

    // --- Target index coordinates ---
    int32_t target_dep_idx = 2;
    int32_t target_lat_idx = 613;
    int32_t target_lon_idx = 906;

    tiledb_ctx_t* ctx = NULL;
    tiledb_array_t* array = NULL;
    tiledb_query_t* query = NULL;
    tiledb_subarray_t* subarray = NULL;

    // Allocate application context
    tiledb_ctx_alloc(NULL, &ctx);

    // Open the array in READ mode
    tiledb_array_alloc(ctx, tiledb_array_name, &array);
    if (tiledb_array_open(ctx, array, TILEDB_READ) != TILEDB_OK) {
        fprintf(stderr, "[ERROR] Unable to open TileDB array '%s' for reading.\n", tiledb_array_name);
        if (array) tiledb_array_free(&array);
        if (ctx) tiledb_ctx_free(&ctx);
        return -1;
    }

    // =========================================================
    // STEP 1: CONVERT INDICES TO REAL-WORLD METADATA COORDINATES
    // =========================================================
    float real_depth = 0.0f;
    float real_lat   = 0.0f;
    float real_lon   = 0.0f;

    tiledb_datatype_t val_type;
    uint32_t val_num;
    const void* val_ptr = NULL;

    // Fetch Depth Metadata Component
    tiledb_array_get_metadata(ctx, array, "coords_depth", &val_type, &val_num, &val_ptr);
    if (val_ptr && (uint32_t)target_dep_idx < val_num) {
        real_depth = ((float*)val_ptr)[target_dep_idx];
    } else {
        fprintf(stderr, "[WARN] Depth index out of bounds or metadata missing.\n");
    }

    // Fetch Latitude Metadata Component
    tiledb_array_get_metadata(ctx, array, "coords_latitude", &val_type, &val_num, &val_ptr);
    if (val_ptr && (uint32_t)target_lat_idx < val_num) {
        real_lat = ((float*)val_ptr)[target_lat_idx];
    } else {
        fprintf(stderr, "[WARN] Latitude index out of bounds or metadata missing.\n");
    }

    // Fetch Longitude Metadata Component
    tiledb_array_get_metadata(ctx, array, "coords_longitude", &val_type, &val_num, &val_ptr);
    if (val_ptr && (uint32_t)target_lon_idx < val_num) {
        real_lon = ((float*)val_ptr)[target_lon_idx];
    } else {
        fprintf(stderr, "[WARN] Longitude index out of bounds or metadata missing.\n");
    }

    // =========================================================
    // STEP 2: RENDER CELL VALUE EXTRACTION SLICE
    // =========================================================
    float val_vp = 0.0f, val_vs = 0.0f, val_rho = 0.0f;
    uint64_t bytes_vp = sizeof(float), bytes_vs = sizeof(float), bytes_rho = sizeof(float);

    tiledb_query_alloc(ctx, array, TILEDB_READ, &query);
    tiledb_query_set_layout(ctx, query, TILEDB_ROW_MAJOR);
    tiledb_subarray_alloc(ctx, array, &subarray);

    int32_t depth_range[] = {target_dep_idx, target_dep_idx};
    int32_t lat_range[]   = {target_lat_idx, target_lat_idx};
    int32_t lon_range[]   = {target_lon_idx, target_lon_idx};

    tiledb_subarray_add_range(ctx, subarray, 0, &depth_range[0], &depth_range[1], NULL);
    tiledb_subarray_add_range(ctx, subarray, 1, &lat_range[0],  &lat_range[1],  NULL);
    tiledb_subarray_add_range(ctx, subarray, 2, &lon_range[0],  &lon_range[1],  NULL);
    tiledb_query_set_subarray_t(ctx, query, subarray);

    tiledb_query_set_data_buffer(ctx, query, "vp", &val_vp, &bytes_vp);
    tiledb_query_set_data_buffer(ctx, query, "vs", &val_vs, &bytes_vs);
    tiledb_query_set_data_buffer(ctx, query, "rho", &val_rho, &bytes_rho);

    if (tiledb_query_submit(ctx, query) != TILEDB_OK) {
        tiledb_error_t* err = NULL;
        tiledb_ctx_get_last_error(ctx, &err);
        const char* msg = NULL;
        tiledb_error_message(err, &msg);
        fprintf(stderr, "[ERROR] TileDB query read failed: %s\n", msg);
        tiledb_error_free(&err);
        goto cleanup;
    }

    tiledb_query_status_t status;
    tiledb_query_get_status(ctx, query, &status);
    if (status == TILEDB_COMPLETED) {
        printf("===========================================\n");
        printf("TARGET LOCATION SUMMARY\n");
        printf("===========================================\n");
        printf("Array Indices    -> Depth Idx: %d | Lat Idx: %d | Lon Idx: %d\n", 
               target_dep_idx, target_lat_idx, target_lon_idx);
        printf("Physical Coords  -> Depth: %.2f | Latitude: %.4f | Longitude: %.4f\n", 
               real_depth, real_lat, real_lon);
        printf("-------------------------------------------\n");
        printf("Material Values  -> VP  : %f m/s\n", val_vp);
        printf("                    VS  : %f m/s\n", val_vs);
        printf("                    RHO : %f kg/m^3\n", val_rho);
        printf("===========================================\n");
    } else {
        fprintf(stderr, "[ERROR] Query finished with unexpected status flag: %d\n", status);
    }

cleanup:
    if (subarray) tiledb_subarray_free(&subarray);
    if (query) tiledb_query_free(&query);
    tiledb_array_close(ctx, array);
    tiledb_array_free(&array);
    tiledb_ctx_free(&ctx);

    return 0;
}

