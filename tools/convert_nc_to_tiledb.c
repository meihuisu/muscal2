#include <stdio.h>
#include <stdlib.h>
#include <netcdf.h>
#include <tiledb/tiledb.h>

// Dataset Dimensions from ncdump
#define DIM_DEPTH 210
#define DIM_LAT 1251
#define DIM_LON 1301

// Streaming Pipeline Depth-Chunk Setting
#define DEPTH_CHUNK_SIZE 10

#define ERR_CHCK(nc_stat) { if (nc_stat != NC_NOERR) { printf("NetCDF Error: %s\n", nc_strerror(nc_stat)); return -1; } }

void create_tiledb_array_v230(const char* array_name) {
    tiledb_ctx_t* ctx;
    tiledb_ctx_alloc(NULL, &ctx);

    // 1. Setup Bounded Dimensions (v2.30 compatible structures)
    tiledb_dimension_t *d1, *d2, *d3;
    int32_t depth_bounds[] = {0, DIM_DEPTH - 1};
    int32_t depth_extent = DEPTH_CHUNK_SIZE; 
    tiledb_dimension_alloc(ctx, "depth", TILEDB_INT32, depth_bounds, &depth_extent, &d1);

    int32_t lat_bounds[] = {0, DIM_LAT - 1};
    int32_t lat_extent = DIM_LAT; 
    tiledb_dimension_alloc(ctx, "latitude", TILEDB_INT32, lat_bounds, &lat_extent, &d2);

    int32_t lon_bounds[] = {0, DIM_LON - 1};
    int32_t lon_extent = DIM_LON;
    tiledb_dimension_alloc(ctx, "longitude", TILEDB_INT32, lon_bounds, &lon_extent, &d3);

    // 2. Setup Spatial Domain Container
    tiledb_domain_t* domain;
    tiledb_domain_alloc(ctx, &domain);
    tiledb_domain_add_dimension(ctx, domain, d1);
    tiledb_domain_add_dimension(ctx, domain, d2);
    tiledb_domain_add_dimension(ctx, domain, d3);

    // 3. Define Main Attributes (vp, vs, rho)
    tiledb_attribute_t *a1, *a2, *a3;
    tiledb_attribute_alloc(ctx, "vp", TILEDB_FLOAT32, &a1);
    tiledb_attribute_alloc(ctx, "vs", TILEDB_FLOAT32, &a2);
    tiledb_attribute_alloc(ctx, "rho", TILEDB_FLOAT32, &a3);

    // 4. Assemble Array Schema for Dense Dataset
    tiledb_array_schema_t* array_schema;
    tiledb_array_schema_alloc(ctx, TILEDB_DENSE, &array_schema);
    tiledb_array_schema_set_cell_order(ctx, array_schema, TILEDB_ROW_MAJOR);
    tiledb_array_schema_set_tile_order(ctx, array_schema, TILEDB_ROW_MAJOR);
    tiledb_array_schema_set_domain(ctx, array_schema, domain);
    tiledb_array_schema_add_attribute(ctx, array_schema, a1);
    tiledb_array_schema_add_attribute(ctx, array_schema, a2);
    tiledb_array_schema_add_attribute(ctx, array_schema, a3);

    // 5. Build Array Dir on VFS
    tiledb_array_create(ctx, array_name, array_schema);

    // Release Layout schemas from stack memory
    tiledb_attribute_free(&a1); tiledb_attribute_free(&a2); tiledb_attribute_free(&a3);
    tiledb_dimension_free(&d1); tiledb_dimension_free(&d2); tiledb_dimension_free(&d3);
    tiledb_domain_free(&domain);
    tiledb_array_schema_free(&array_schema);
    tiledb_ctx_free(&ctx);
}

int main() {
    const char* nc_filename = "/var/www/html/CVM_DATASET_DIRECTORY/model/muscal/model_MUSCAL_CANVAS_dll0.01_vardz_float32_cmpd.nc";
    const char* tiledb_array_name = "model_MUSCAL_CANVAS_dll0.01_vardz_float32_cmpd.tiledb";
    int ncid = -1, vp_id, vs_id, rho_id;
    int depth_var_id, lat_var_id, lon_var_id;

    // Pointers initialized to NULL for safety in error_cleanup jumps
    tiledb_ctx_t* ctx = NULL;
    tiledb_array_t* array = NULL;
    float* coords_depth = NULL;
    float* coords_lat = NULL;
    float* coords_lon = NULL;
    float* buffer_vp = NULL;
    float* buffer_vs = NULL;
    float* buffer_rho = NULL;

    printf("Opening target NetCDF storage component: %s...\n", nc_filename);
    ERR_CHCK(nc_open(nc_filename, NC_NOWRITE, &ncid));

    // Inspect variable IDs
    ERR_CHCK(nc_inq_varid(ncid, "vp", &vp_id));
    ERR_CHCK(nc_inq_varid(ncid, "vs", &vs_id));
    ERR_CHCK(nc_inq_varid(ncid, "rho", &rho_id));
    ERR_CHCK(nc_inq_varid(ncid, "depth", &depth_var_id));
    ERR_CHCK(nc_inq_varid(ncid, "latitude", &lat_var_id));
    ERR_CHCK(nc_inq_varid(ncid, "longitude", &lon_var_id));

    printf("Constructing TileDB Array directory schema...\n");
    create_tiledb_array_v230(tiledb_array_name);

    // Establish Core Write Session via Context Handles
    tiledb_ctx_alloc(NULL, &ctx);
    tiledb_array_alloc(ctx, tiledb_array_name, &array);
    if (tiledb_array_open(ctx, array, TILEDB_WRITE) != TILEDB_OK) {
        fprintf(stderr, "TileDB Error: Unable to open array file for writing.\n");
        return -1;
    }

    // =========================================================
    // STEP 1: READ & ATTACH PHYSICAL COORDINATE ARRAYS (METADATA)
    // =========================================================
    printf("Extracting 1D real-world location tracking vectors...\n");
    coords_depth = (float*)malloc(DIM_DEPTH * sizeof(float));
    coords_lat   = (float*)malloc(DIM_LAT * sizeof(float));
    coords_lon   = (float*)malloc(DIM_LON * sizeof(float));

    if (!coords_depth || !coords_lat || !coords_lon) {
        fprintf(stderr, "Fatal error: Out of heap space for coordinate mapping vectors.\n");
        goto error_cleanup;
    }

    ERR_CHCK(nc_get_var_float(ncid, depth_var_id, coords_depth));
    ERR_CHCK(nc_get_var_float(ncid, lat_var_id, coords_lat));
    ERR_CHCK(nc_get_var_float(ncid, lon_var_id, coords_lon));

    // Map 1D floating coordinates using standard v2.30 key-value storage signatures
    tiledb_array_put_metadata(ctx, array, "coords_depth", TILEDB_FLOAT32, DIM_DEPTH, coords_depth);
    tiledb_array_put_metadata(ctx, array, "coords_latitude", TILEDB_FLOAT32, DIM_LAT, coords_lat);
    tiledb_array_put_metadata(ctx, array, "coords_longitude", TILEDB_FLOAT32, DIM_LON, coords_lon);

    free(coords_depth); coords_depth = NULL;
    free(coords_lat);   coords_lat = NULL;
    free(coords_lon);   coords_lon = NULL;
    printf(" -> Coordinate vectors correctly bound to schema metadata headers.\n");

    // =========================================================
    // STEP 2: STREAMING HYPERSLAB DATA CONVERSION
    // =========================================================
    size_t max_elements_per_slab = (size_t)DEPTH_CHUNK_SIZE * DIM_LAT * DIM_LON;
    size_t slab_bytes = max_elements_per_slab * sizeof(float);

    printf("Allocating stream window buffers (~%.2f MB allocated memory footprint per channel)...\n", (double)slab_bytes / (1024.0 * 1024.0));
    buffer_vp = (float*)malloc(slab_bytes);
    buffer_vs = (float*)malloc(slab_bytes);
    buffer_rho = (float*)malloc(slab_bytes);

    if (!buffer_vp || !buffer_vs || !buffer_rho) {
        fprintf(stderr, "Fatal error: Streaming slice allocation failure.\n");
        goto error_cleanup;
    }

    printf("Executing streaming pass across hyperslab iterations...\n");
    for (int start_depth = 0; start_depth < DIM_DEPTH; start_depth += DEPTH_CHUNK_SIZE) {
        int current_chunk_size = (start_depth + DEPTH_CHUNK_SIZE > DIM_DEPTH) 
                                 ? (DIM_DEPTH - start_depth) 
                                 : DEPTH_CHUNK_SIZE;
        
        // Compute element count and the shared buffer size in BYTES
        uint64_t current_elements = (uint64_t)current_chunk_size * DIM_LAT * DIM_LON;
        uint64_t current_element_bytes = current_elements * sizeof(float);

        printf(" -> Processing slice section for depth indices: [%d to %d]\n", start_depth, start_depth + current_chunk_size - 1);

        // Fetch target 3D bounding range from NetCDF variables
        size_t nc_start[] = {start_depth, 0, 0};
        size_t nc_count[] = {current_chunk_size, DIM_LAT, DIM_LON};

        ERR_CHCK(nc_get_vara_float(ncid, vp_id, nc_start, nc_count, buffer_vp));
        ERR_CHCK(nc_get_vara_float(ncid, vs_id, nc_start, nc_count, buffer_vs));
        ERR_CHCK(nc_get_vara_float(ncid, rho_id, nc_start, nc_count, buffer_rho));

        // Create standard query context
        tiledb_query_t* query;
        tiledb_query_alloc(ctx, array, TILEDB_WRITE, &query);
        tiledb_query_set_layout(ctx, query, TILEDB_ROW_MAJOR);

        // --- TileDB v2.30 Subarray Object Allocation ---
        tiledb_subarray_t* subarray;
        tiledb_subarray_alloc(ctx, array, &subarray);

        // Define bounds for each dimension range pointer
        int32_t depth_range[] = {start_depth, start_depth + current_chunk_size - 1};
        int32_t lat_range[]   = {0, DIM_LAT - 1};
        int32_t lon_range[]   = {0, DIM_LON - 1};

        // Add explicit coordinate ranges index by index (0=depth, 1=latitude, 2=longitude)
        tiledb_subarray_add_range(ctx, subarray, 0, &depth_range[0], &depth_range[1], NULL);
        tiledb_subarray_add_range(ctx, subarray, 1, &lat_range[0],  &lat_range[1],  NULL);
        tiledb_subarray_add_range(ctx, subarray, 2, &lon_range[0],  &lon_range[1],  NULL);

        // Bind the completed subarray tracking object to the current query context
        tiledb_query_set_subarray_t(ctx, query, subarray);

        // Bind attributes using the shared byte-count tracking reference.
        // (Note: TileDB safely reads the value at query submit time, but we pass the address 
        // separately for each call because TileDB may write back to it upon completion)
        uint64_t bytes_vp = current_element_bytes;
        uint64_t bytes_vs = current_element_bytes;
        uint64_t bytes_rho = current_element_bytes;

        tiledb_query_set_data_buffer(ctx, query, "vp", buffer_vp, &bytes_vp);
        tiledb_query_set_data_buffer(ctx, query, "vs", buffer_vs, &bytes_vs);
        tiledb_query_set_data_buffer(ctx, query, "rho", buffer_rho, &bytes_rho);

        // 1. Submit the write query and check immediate API return value
        if (tiledb_query_submit(ctx, query) != TILEDB_OK) {
            tiledb_error_t* err = NULL;
            tiledb_ctx_get_last_error(ctx, &err);
            const char* msg = NULL;
            tiledb_error_message(err, &msg);
            fprintf(stderr, "\n[ERROR] TileDB query submission failed at depth %d: %s\n", start_depth, msg);
            tiledb_error_free(&err);
            
            tiledb_subarray_free(&subarray);
            tiledb_query_free(&query);
            goto error_cleanup;
        }

        // 2. Double-check the query execution status lifecycle
        tiledb_query_status_t status;
        tiledb_query_get_status(ctx, query, &status);
        if (status != TILEDB_COMPLETED) {
            fprintf(stderr, "\n[ERROR] TileDB query finished with unexpected status %d at depth %d\n", status, start_depth);
            tiledb_subarray_free(&subarray);
            tiledb_query_free(&query);
            goto error_cleanup;
        }

        // Free loop specific iteration structures
        tiledb_subarray_free(&subarray);
        tiledb_query_free(&query);
    }

    // --- PIPELINE CLEANUP (NORMAL COMPLETION) ---
    nc_close(ncid);
    tiledb_array_close(ctx, array);
    tiledb_array_free(&array);
    tiledb_ctx_free(&ctx);

    free(buffer_vp);
    free(buffer_vs);
    free(buffer_rho);

    printf("\nUnified streaming pipeline completed successfully!\n");
    return 0;

// --- ERROR CLEANUP TARGET ---
error_cleanup:
    if (ncid >= 0) nc_close(ncid);
    if (array) {
        tiledb_array_close(ctx, array);
        tiledb_array_free(&array);
    }
    if (ctx) tiledb_ctx_free(&ctx);
    
    free(coords_depth);
    free(coords_lat);
    free(coords_lon);
    free(buffer_vp);
    free(buffer_vs);
    free(buffer_rho);
    
    fprintf(stderr, "\nPipeline aborted due to runtime errors.\n");
    return -1;
}
