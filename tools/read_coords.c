
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tiledb/tiledb.h>

// Dataset Dimensions matching your NetCDF configuration
#define DIM_DEPTH 210
#define DIM_LAT 1251
#define DIM_LON 1301

int main() {
    const char* tiledb_array_name = "muscal_tiledb";
    
    // Core TileDB infrastructure handles initialized to NULL
    tiledb_ctx_t* ctx = NULL;
    tiledb_array_t* array = NULL;

    // Buffer allocations for extracting the 1D tracking scales
    float* depth_metadata = NULL;
    float* lat_metadata = NULL;
    float* lon_metadata = NULL;

    // Allocate core application context
    tiledb_ctx_alloc(NULL, &ctx);
    
    // Allocate and open the targeted array in READ mode
    tiledb_array_alloc(ctx, tiledb_array_name, &array);
    if (tiledb_array_open(ctx, array, TILEDB_READ) != TILEDB_OK) {
        fprintf(stderr, "[ERROR] Unable to open TileDB array '%s' for reading.\n", tiledb_array_name);
        if (array) tiledb_array_free(&array);
        if (ctx) tiledb_ctx_free(&ctx);
        return -1;
    }

    printf("Successfully opened array '%s' in read mode.\n", tiledb_array_name);
    printf("Extracting geographical coordinate vectors from metadata structures...\n\n");

    // Allocate memory on the heap to receive the values
    depth_metadata = (float*)malloc(DIM_DEPTH * sizeof(float));
    lat_metadata   = (float*)malloc(DIM_LAT * sizeof(float));
    lon_metadata   = (float*)malloc(DIM_LON * sizeof(float));

    if (!depth_metadata || !lat_metadata || !lon_metadata) {
        fprintf(stderr, "[ERROR] Heap memory allocation failed for output arrays.\n");
        goto cleanup;
    }

    // Variables to hold the metadata value configuration checks
    tiledb_datatype_t value_type;
    uint32_t value_num;
    const void* value_ptr;

    // =========================================================
    // 1. EXTRACT & VERIFY DEPTH VALUES
    // =========================================================
    // Fetch a direct internal pointer to the metadata values
    tiledb_array_get_metadata(ctx, array, "coords_depth", &value_type, &value_num, &value_ptr);
    if (value_ptr != NULL) {
        // Copy the raw bytes securely into our typed array structure
        uint32_t elements_to_copy = (value_num > DIM_DEPTH) ? DIM_DEPTH : value_num;
        memcpy(depth_metadata, value_ptr, elements_to_copy * sizeof(float));
        
        printf("--- DEPTH METADATA (Total Elements Found: %u) ---\n", value_num);
        printf("First 5 values: ");
        for (int i = 0; i < (elements_to_copy < 5 ? elements_to_copy : 5); i++) {
            printf("%f ", depth_metadata[i]);
        }
        printf("\nLast value: %f\n\n", depth_metadata[elements_to_copy - 1]);
    } else {
        printf("[WARN] Metadata key 'coords_depth' was not found or is empty.\n\n");
    }

    // =========================================================
    // 2. EXTRACT & VERIFY LATITUDE VALUES
    // =========================================================
    tiledb_array_get_metadata(ctx, array, "coords_latitude", &value_type, &value_num, &value_ptr);
    if (value_ptr != NULL) {
        uint32_t elements_to_copy = (value_num > DIM_LAT) ? DIM_LAT : value_num;
        memcpy(lat_metadata, value_ptr, elements_to_copy * sizeof(float));
        
        printf("--- LATITUDE METADATA (Total Elements Found: %u) ---\n", value_num);
        printf("First 5 values: ");
        for (int i = 0; i < (elements_to_copy < 5 ? elements_to_copy : 5); i++) {
            printf("%f ", lat_metadata[i]);
        }
        printf("\nLast value: %f\n\n", lat_metadata[elements_to_copy - 1]);
    } else {
        printf("[WARN] Metadata key 'coords_latitude' was not found or is empty.\n\n");
    }

    // =========================================================
    // 3. EXTRACT & VERIFY LONGITUDE VALUES
    // =========================================================
    tiledb_array_get_metadata(ctx, array, "coords_longitude", &value_type, &value_num, &value_ptr);
    if (value_ptr != NULL) {
        uint32_t elements_to_copy = (value_num > DIM_LON) ? DIM_LON : value_num;
        memcpy(lon_metadata, value_ptr, elements_to_copy * sizeof(float));
        
        printf("--- LONGITUDE METADATA (Total Elements Found: %u) ---\n", value_num);
        printf("First 5 values: ");
        for (int i = 0; i < (elements_to_copy < 5 ? elements_to_copy : 5); i++) {
            printf("%f ", lon_metadata[i]);
        }
        printf("\nLast value: %f\n\n", lon_metadata[elements_to_copy - 1]);
    } else {
        printf("[WARN] Metadata key 'coords_longitude' was not found or is empty.\n\n");
    }

cleanup:
    // Safely unwind open descriptors and allocated blocks
    if (array) {
        tiledb_array_close(ctx, array);
        tiledb_array_free(&array);
    }
    if (ctx) tiledb_ctx_free(&ctx);

    free(depth_metadata);
    free(lat_metadata);
    free(lon_metadata);

    return 0;
}
