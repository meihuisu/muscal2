/**
 * convert_nc_to_tiledb.c
 * * Complete Streamed Conversion Tool for TileDB v2.30.0+
 * Reduces memory footprint by streaming 3D datasets using depth-chunked hyperslabs,
 * while fully preserving NetCDF Variable Attributes (_FillValue and units).
 * * Compile: gcc -std=c11 convert_nc_to_tiledb.c -o convert_nc_to_tiledb -lnetcdf -ltiledb
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netcdf.h>
#include <tiledb/tiledb.h>

#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(1);}

int main() {
    // File and Array Names
    const char* nc_filename = "model_MUSCAL_CANVAS_dll0.01_vardz_float32_cmpd.nc";
    const char* tiledb_array_name = "muscal_tiledb_array";

    // Global Dataset Dimensions
    const size_t depth_len = 210;
    const size_t lat_len = 1251;
    const size_t lon_len = 1301;
    
    // Chunk Configuration (Matches TileDB Extents)
    const size_t chunk_depth_step = 30; 
    const size_t chunk_elements = chunk_depth_step * lat_len * lon_len;
    const size_t max_chunk_bytes = chunk_elements * sizeof(float);

    printf("Initializing streamed migration with metadata preservation...\n");
    printf("Global dimensions: Depth=%zu, Lat=%zu, Lon=%zu\n", depth_len, lat_len, lon_len);

    // --- 1. OPEN NETCDF FIRST TO EXTRACT VARIABLE METADATA ---
    int ncid, retval;
    int varid_vp, varid_vs, varid_rho;
    if ((retval = nc_open(nc_filename, NC_NOWRITE, &ncid))) ERR(retval);
    if ((retval = nc_inq_varid(ncid, "vp", &varid_vp))) ERR(retval);
    if ((retval = nc_inq_varid(ncid, "vs", &varid_vs))) ERR(retval);
    if ((retval = nc_inq_varid(ncid, "rho", &varid_rho))) ERR(retval);

    // Dynamic extraction buffers for 3D metadata attributes
    float vp_fill, vs_fill, rho_fill;
    char vp_units[64] = {0}, vs_units[64] = {0}, rho_units[64] = {0};
    size_t att_len;

    nc_get_att_float(ncid, varid_vp, "_FillValue", &vp_fill);
    nc_inq_attlen(ncid, varid_vp, "units", &att_len);
    nc_get_att_text(ncid, varid_vp, "units", vp_units);
    vp_units[att_len] = '\0';

    nc_get_att_float(ncid, varid_vs, "_FillValue", &vs_fill);
    nc_inq_attlen(ncid, varid_vs, "units", &att_len);
    nc_get_att_text(ncid, varid_vs, "units", vs_units);
    vs_units[att_len] = '\0';

    nc_get_att_float(ncid, varid_rho, "_FillValue", &rho_fill);
    nc_inq_attlen(ncid, varid_rho, "units", &att_len);
    nc_get_att_text(ncid, varid_rho, "units", rho_units);
    rho_units[att_len] = '\0';

    // Allocate host memory buffers for streaming 3D variables
    float* host_vp = (float*)malloc(max_chunk_bytes);
    float* host_vs = (float*)malloc(max_chunk_bytes);
    float* host_rho = (float*)malloc(max_chunk_bytes);

    if (host_vp == NULL || host_vs == NULL || host_rho == NULL) {
        fprintf(stderr, "Fatal Error: Failed to allocate workspace RAM.\n");
        return 1;
    }

    // --- 2. INITIALIZE 3D TILEDB CONTEXT & SCHEMA ---
    printf("Creating Core 3D TileDB Schema with explicit Fill Values...\n");
    tiledb_ctx_t* ctx;
    tiledb_ctx_alloc(NULL, &ctx);

    int32_t dim_bounds[] = {1, 210, 1, 1251, 1, 1301};
    int32_t tile_extents[] = {30, 50, 50}; 

    tiledb_dimension_t *d1, *d2, *d3;
    tiledb_dimension_alloc(ctx, "depth", TILEDB_INT32, &dim_bounds[0], &tile_extents[0], &d1);
    tiledb_dimension_alloc(ctx, "latitude", TILEDB_INT32, &dim_bounds[2], &tile_extents[1], &d2);
    tiledb_dimension_alloc(ctx, "longitude", TILEDB_INT32, &dim_bounds[4], &tile_extents[2], &d3);

    tiledb_domain_t* domain;
    tiledb_domain_alloc(ctx, &domain);
    tiledb_domain_add_dimension(ctx, domain, d1);
    tiledb_domain_add_dimension(ctx, domain, d2);
    tiledb_domain_add_dimension(ctx, domain, d3);

    tiledb_attribute_t *a1, *a2, *a3;
    tiledb_attribute_alloc(ctx, "vp", TILEDB_FLOAT32, &a1);
    tiledb_attribute_set_fill_value(ctx, a1, &vp_fill, sizeof(float)); // Map NaNf Fill Value

    tiledb_attribute_alloc(ctx, "vs", TILEDB_FLOAT32, &a2);
    tiledb_attribute_set_fill_value(ctx, a2, &vs_fill, sizeof(float)); // Map NaNf Fill Value

    tiledb_attribute_alloc(ctx, "rho", TILEDB_FLOAT32, &a3);
    tiledb_attribute_set_fill_value(ctx, a3, &rho_fill, sizeof(float)); // Map NaNf Fill Value

    tiledb_array_schema_t* array_schema;
    tiledb_array_schema_alloc(ctx, TILEDB_DENSE, &array_schema);
    tiledb_array_schema_set_cell_order(ctx, array_schema, TILEDB_ROW_MAJOR);
    tiledb_array_schema_set_tile_order(ctx, array_schema, TILEDB_ROW_MAJOR);
    tiledb_array_schema_set_domain(ctx, array_schema, domain);
    tiledb_array_schema_add_attribute(ctx, array_schema, a1);
    tiledb_array_schema_add_attribute(ctx, array_schema, a2);
    tiledb_array_schema_add_attribute(ctx, array_schema, a3);

    tiledb_array_create(ctx, tiledb_array_name, array_schema);
    printf("TileDB storage target initialized at './%s'\n", tiledb_array_name);

    // --- 3. OPEN TARGETS & WRITE 3D METADATA ---
    tiledb_array_t* array;
    tiledb_array_alloc(ctx, tiledb_array_name, &array);
    tiledb_array_open(ctx, array, TILEDB_WRITE);

    // Ingest the missing Units into the main TileDB array metadata block
    tiledb_array_put_metadata(ctx, array, "vp_units", TILEDB_CHAR, (uint32_t)strlen(vp_units), vp_units);
    tiledb_array_put_metadata(ctx, array, "vs_units", TILEDB_CHAR, (uint32_t)strlen(vs_units), vs_units);
    tiledb_array_put_metadata(ctx, array, "rho_units", TILEDB_CHAR, (uint32_t)strlen(rho_units), rho_units);

    tiledb_query_t* query;
    tiledb_query_alloc(ctx, array, TILEDB_WRITE, &query);
    tiledb_query_set_layout(ctx, query, TILEDB_ROW_MAJOR);

    // --- 4. LOOP 3D CHUNKS THROUGH THE PIPELINE ---
    printf("Beginning piped layout ingestion...\n");
    for (size_t d_start = 0; d_start < depth_len; d_start += chunk_depth_step) {
        size_t current_chunk_depth = (d_start + chunk_depth_step > depth_len) ? 
                                     (depth_len - d_start) : chunk_depth_step;
        
        size_t current_elements = current_chunk_depth * lat_len * lon_len;
        size_t current_data_bytes = current_elements * sizeof(float);

        printf(" -> Processing depths %zu to %zu/%zu...\n", 
                d_start + 1, d_start + current_chunk_depth, depth_len);

        size_t nc_start[] = {d_start, 0, 0};
        size_t nc_count[] = {current_chunk_depth, lat_len, lon_len};
        
        if ((retval = nc_get_vara_float(ncid, varid_vp, nc_start, nc_count, host_vp))) ERR(retval);
        if ((retval = nc_get_vara_float(ncid, varid_vs, nc_start, nc_count, host_vs))) ERR(retval);
        if ((retval = nc_get_vara_float(ncid, varid_rho, nc_start, nc_count, host_rho))) ERR(retval);

        int32_t tiledb_subarray_bounds[] = {
            (int32_t)(d_start + 1), (int32_t)(d_start + current_chunk_depth),
            1, 1251,
            1, 1301
        };
        
        tiledb_subarray_t* subarray;
        tiledb_subarray_alloc(ctx, array, &subarray);
        tiledb_subarray_set_subarray(ctx, subarray, tiledb_subarray_bounds);
        tiledb_query_set_subarray_t(ctx, query, subarray);

        size_t bytes_vp = current_data_bytes;
        size_t bytes_vs = current_data_bytes;
        size_t bytes_rho = current_data_bytes;

        tiledb_query_set_data_buffer(ctx, query, "vp", host_vp, &bytes_vp);
        tiledb_query_set_data_buffer(ctx, query, "vs", host_vs, &bytes_vs);
        tiledb_query_set_data_buffer(ctx, query, "rho", host_rho, &bytes_rho);

        tiledb_query_submit(ctx, query);
        tiledb_subarray_free(&subarray);
    }

    tiledb_query_free(&query);
    tiledb_array_close(ctx, array);
    tiledb_array_free(&array);


    // =========================================================================
    // --- 5. INGEST 1D COORDINATE ARRAYS WITH METADATA ---
    // =========================================================================
    printf("\nProcessing 1D coordinate dimension tracking scales...\n");

    const char* coord_names[] = {"depth", "latitude", "longitude"};
    size_t coord_lens[] = {depth_len, lat_len, lon_len};

    for (int c = 0; c < 3; c++) {
        const char* name = coord_names[c];
        size_t len = coord_lens[c];
        
        char target_array_name[256];
        snprintf(target_array_name, sizeof(target_array_name), "%s_%s", tiledb_array_name, name);

        // Extract coordinate variables and their attributes
        int varid_coord;
        float coord_fill = 0.0f;
        char coord_units[64] = {0};

        if ((retval = nc_inq_varid(ncid, name, &varid_coord))) ERR(retval);
        nc_get_att_float(ncid, varid_coord, "_FillValue", &coord_fill);
        nc_inq_attlen(ncid, varid_coord, "units", &att_len);
        nc_get_att_text(ncid, varid_coord, "units", coord_units);
        coord_units[att_len] = '\0';

        printf(" -> Extracting variable: %s (Units: %s) -> './%s'\n", name, coord_units, target_array_name);

        float* coord_buf = (float*)malloc(len * sizeof(float));
        if ((retval = nc_get_var_float(ncid, varid_coord, coord_buf))) ERR(retval);

        // Build 1D Dense Schema
        int32_t c_bounds[] = {1, (int32_t)len};
        int32_t c_extent = (int32_t)len;

        tiledb_dimension_t* cd1;
        tiledb_dimension_alloc(ctx, "index", TILEDB_INT32, &c_bounds[0], &c_extent, &cd1);

        tiledb_domain_t* c_domain;
        tiledb_domain_alloc(ctx, &c_domain);
        tiledb_domain_add_dimension(ctx, c_domain, cd1);

        tiledb_attribute_t* c_attr;
        tiledb_attribute_alloc(ctx, "coords", TILEDB_FLOAT32, &c_attr);
        tiledb_attribute_set_fill_value(ctx, c_attr, &coord_fill, sizeof(float)); // Set 1D NaNf Fill Value

        tiledb_array_schema_t* c_schema;
        tiledb_array_schema_alloc(ctx, TILEDB_DENSE, &c_schema);
        tiledb_array_schema_set_cell_order(ctx, c_schema, TILEDB_ROW_MAJOR);
        tiledb_array_schema_set_tile_order(ctx, c_schema, TILEDB_ROW_MAJOR);
        tiledb_array_schema_set_domain(ctx, c_schema, c_domain);
        tiledb_array_schema_add_attribute(ctx, c_schema, c_attr);

        tiledb_array_create(ctx, target_array_name, c_schema);

        // Open, Write Metadata, Write Elements
        tiledb_array_t* c_array;
        tiledb_array_alloc(ctx, target_array_name, &c_array);
        tiledb_array_open(ctx, c_array, TILEDB_WRITE);

        // Inject 1D Units into companion array metadata metadata
        tiledb_array_put_metadata(ctx, c_array, "units", TILEDB_CHAR, (uint32_t)strlen(coord_units), coord_units);

        tiledb_query_t* c_query;
        tiledb_query_alloc(ctx, c_array, TILEDB_WRITE, &c_query);
        tiledb_query_set_layout(ctx, c_query, TILEDB_ROW_MAJOR);

        tiledb_subarray_t* c_sub;
        tiledb_subarray_alloc(ctx, c_array, &c_sub);
        tiledb_subarray_set_subarray(ctx, c_sub, c_bounds);
        tiledb_query_set_subarray_t(ctx, c_query, c_sub);

        size_t c_bytes = len * sizeof(float);
        tiledb_query_set_data_buffer(ctx, c_query, "coords", coord_buf, &c_bytes);

        tiledb_query_submit(ctx, c_query);

        // Free Structures
        tiledb_subarray_free(&c_sub);
        tiledb_query_free(&c_query);
        tiledb_array_close(ctx, c_array);
        tiledb_array_free(&c_array);
        tiledb_dimension_free(&cd1);
        tiledb_domain_free(&c_domain);
        tiledb_attribute_free(&c_attr);
        tiledb_array_schema_free(&c_schema);
        free(coord_buf);
    }

    // --- 6. FINAL SHUTDOWN & CLEANUP ---
    printf("\nFinalizing file structures and releasing global contexts...\n");
    nc_close(ncid);

    printf("Success! Full migration finished without losing variable attributes.\n");

    free(host_vp); free(host_vs); free(host_rho);
    tiledb_dimension_free(&d1); tiledb_dimension_free(&d2); tiledb_dimension_free(&d3);
    tiledb_attribute_free(&a1); tiledb_attribute_free(&a2); tiledb_attribute_free(&a3);
    tiledb_domain_free(&domain);
    tiledb_array_schema_free(&array_schema);
    tiledb_ctx_free(&ctx);

    return 0;
}
