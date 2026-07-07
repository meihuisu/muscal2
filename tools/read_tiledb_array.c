/**
 * read_tiledb_array.c
 * Updated for TileDB v2.30.0+ Compliant API
 * * Compile: gcc -std=c11 read_tiledb_array.c -o read_tiledb_array -ltiledb -lm
 **/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include <tiledb/tiledb.h>

int main(int argc, char  *const argv[]) {

//    const char* tiledb_array_name = "new-muscal_canvas_tiledb_array";
    const char* tiledb_array_name = "muscal_tiledb_array";

    // Initialize TileDB Context
    tiledb_ctx_t* ctx = NULL;
    if (tiledb_ctx_alloc(NULL, &ctx) != TILEDB_OK) {
        fprintf(stderr, "Error: Failed to allocate TileDB context.\n");
        return 1;
    }

    // Open the array cleanly for reading
    tiledb_array_t* array = NULL;
    tiledb_array_alloc(ctx, tiledb_array_name, &array);
    if (tiledb_array_open(ctx, array, TILEDB_READ) != TILEDB_OK) {
        fprintf(stderr, "Error: Could not open array %s for reading.\n", tiledb_array_name);
        tiledb_array_free(&array);
        tiledb_ctx_free(&ctx);
        return 1;
    }

char line[1024];
int rdepth = 0.0; 
int rlat   = 0.0; 
int rlon   = 0.0;  

fprintf(stderr,"\n INPUT: dep_idx lat_idx lon_idx... \n");

while (fgets(line, 1024, stdin) != NULL) {
 if(line[0]=='#') continue;  // a comment line
 if (sscanf(line,"%d %d %d", &rdepth, &rlat, &rlon) == 3) {

    fprintf(stderr,"\nUSING %d %d %d \n", rdepth,rlat,rlon);

    rdepth++;
    rlat++;
    rlon++;

    // =========================================================================
    // SECTION 0: Read a localized 3D Voxel Location (Example)
    // =========================================================================

    printf("\n--- Reading a loc ---\n");
    int32_t loc_subarray[] = { rdepth, rdepth, rlat, rlat, rlon, rlon };
    printf("  location: %d %d / %d %d / %d %d\n\n", loc_subarray[0], loc_subarray[1], 
                  loc_subarray[2], loc_subarray[3], loc_subarray[4], loc_subarray[5]);

    float vp_loc[1];
    uint64_t loc_buffer_size = sizeof(vp_loc);

    tiledb_query_t* query_loc = NULL;
    tiledb_subarray_t* sub_obj_loc = NULL;

    tiledb_query_alloc(ctx, array, TILEDB_READ, &query_loc);
    tiledb_query_set_layout(ctx, query_loc, TILEDB_ROW_MAJOR);

    // Modern Subarray API Pipeline for 1D Profile Read
    tiledb_subarray_alloc(ctx, array, &sub_obj_loc);
    tiledb_subarray_set_subarray(ctx, sub_obj_loc, loc_subarray);
    tiledb_query_set_subarray_t(ctx, query_loc, sub_obj_loc);
    
    tiledb_query_set_data_buffer(ctx, query_loc, "vp", vp_loc, &loc_buffer_size);

    if (tiledb_query_submit(ctx, query_loc) != TILEDB_OK) {
        tiledb_error_t* err = NULL;
        tiledb_ctx_get_last_error(ctx, &err);
        const char* msg = NULL;
        tiledb_error_message(err, &msg);
        fprintf(stderr, "TileDB Query Failure: %s\n", msg);
        tiledb_error_free(&err);
    } else {
        printf("Retrieved Vp block elements successfully. Base value: %.4f m/s\n", vp_loc[0]);
    }
    tiledb_subarray_free(&sub_obj_loc);
    tiledb_query_free(&query_loc);

    // =========================================================================
    // SECTION 1: Read a localized 3D Voxel Block (Example)
    // =========================================================================

    printf("--- Reading a localized 2x2x2 Voxel Block ---\n");

    int32_t block_subarray[] = { rdepth, rdepth+1, rlat, rlat+1, rlon, rlon+1 };
    printf("  block: %d %d / %d %d / %d %d\n", block_subarray[0], block_subarray[1], 
           block_subarray[2], block_subarray[3], block_subarray[4], block_subarray[5]);

    float vp_block[8];
    uint64_t vp_buffer_size = sizeof(vp_block);

    tiledb_query_t* query_block = NULL;
    tiledb_subarray_t* sub_obj_block = NULL;

    tiledb_query_alloc(ctx, array, TILEDB_READ, &query_block);
    tiledb_query_set_layout(ctx, query_block, TILEDB_ROW_MAJOR);

    // Modern Subarray API Pipeline for Block Read
    tiledb_subarray_alloc(ctx, array, &sub_obj_block);
    tiledb_subarray_set_subarray(ctx, sub_obj_block, block_subarray);
    tiledb_query_set_subarray_t(ctx, query_block, sub_obj_block);
    tiledb_query_set_data_buffer(ctx, query_block, "vp", vp_block, &vp_buffer_size);

    if (tiledb_query_submit(ctx, query_block) != TILEDB_OK) {
        tiledb_error_t* err = NULL;
        tiledb_ctx_get_last_error(ctx, &err);
        const char* msg = NULL;
        tiledb_error_message(err, &msg);
        fprintf(stderr, "TileDB Query Failure: %s\n", msg);
        tiledb_error_free(&err);
    } else {
        printf("Retrieved Vp block elements successfully. Base value: %.4f m/s\n", vp_block[0]);
        printf("Retrieved Vp block elements successfully. Base value: %.4f m/s\n", vp_block[1]);
        printf("Retrieved Vp block elements successfully. Base value: %.4f m/s\n", vp_block[2]);
        printf("Retrieved Vp block elements successfully. Base value: %.4f m/s\n", vp_block[3]);
        printf("Retrieved Vp block elements successfully. Base value: %.4f m/s\n", vp_block[4]);
        printf("Retrieved Vp block elements successfully. Base value: %.4f m/s\n", vp_block[5]);
        printf("Retrieved Vp block elements successfully. Base value: %.4f m/s\n", vp_block[6]);
        printf("Retrieved Vp block elements successfully. Base value: %.4f m/s\n", vp_block[7]);
    }

    // Clean up block query resources
    tiledb_subarray_free(&sub_obj_block);
    tiledb_query_free(&query_block);


    // =========================================================================
    // SECTION 2: Reading a 1D slice along the Latitude axis
    // =========================================================================
    printf("--- Reading a 1D slice along the Latitude axis ---\n");

    // To read a profile across all latitudes, we fix depth and longitude to 1 cell,
    // and span the subarray across all 1251 latitude points.

//int32_t lat_subarray[] = { 2, 2, 1, 1251, 906, 906 };
int32_t lat_subarray[] = { rdepth, rdepth, 1, 1251, rlon, rlon };
    printf("  profile: %d %d / %d %d / %d %d\n", lat_subarray[0], lat_subarray[1], 
                  lat_subarray[2], lat_subarray[3], lat_subarray[4], lat_subarray[5]);

    uint64_t lat_elements = 1251;
    uint64_t lat_buffer_size = lat_elements * sizeof(float);
    float* vp_lat_profile = (float*)malloc(lat_buffer_size);

    tiledb_query_t* query_lat = NULL;
    tiledb_subarray_t* sub_obj_lat = NULL;

    tiledb_query_alloc(ctx, array, TILEDB_READ, &query_lat);
    tiledb_query_set_layout(ctx, query_lat, TILEDB_ROW_MAJOR);

    // Modern Subarray API Pipeline for 1D Profile Read
    tiledb_subarray_alloc(ctx, array, &sub_obj_lat);
    tiledb_subarray_set_subarray(ctx, sub_obj_lat, lat_subarray);
    tiledb_query_set_subarray_t(ctx, query_lat, sub_obj_lat);
    
    // Pulling from the 'vp' attribute along a 1D geographic vector
    tiledb_query_set_data_buffer(ctx, query_lat, "vp", vp_lat_profile, &lat_buffer_size);

    if (tiledb_query_submit(ctx, query_lat) != TILEDB_OK) {
        tiledb_error_t* err = NULL;
        tiledb_ctx_get_last_error(ctx, &err);
        const char* msg = NULL;
        tiledb_error_message(err, &msg);
        fprintf(stderr, "TileDB Query Failure: %s\n", msg);
        tiledb_error_free(&err);

    // Clean up to prevent memory leaks before exiting
        free(vp_lat_profile);
        tiledb_subarray_free(&sub_obj_lat);
        tiledb_query_free(&query_lat);
        tiledb_array_close(ctx, array);
        tiledb_array_free(&array);
        tiledb_ctx_free(&ctx) ;
        return 1;
    }

    printf("Retrieved %" PRIu64 " latitude points for vp profile. First N values:\n", lat_elements);

    int valid_count = 0;
    int first_valid_index = -1;
    
    for (int i = 0; i < lat_elements; i++) {
            // isnan() comes from <math.h>
            if (!isnan(vp_lat_profile[i])) {
                valid_count++;
                if (first_valid_index == -1) {
                    first_valid_index = i + 1; // 1-based index for display
                }
                // Print only the first 5 real data points found
    if (i==610 || i==611 || i==612 || i==613 || i==614)
    //if(valid_count < 5) 
                {
                    printf("  [Real Data] lat_index[%d]: %.4f m/s\n", i + 1, vp_lat_profile[i]);
                }
            }
    }
    printf(" valid %d out of %d\n", valid_count, lat_elements);
    free(vp_lat_profile);
    tiledb_query_free(&query_lat);
}
}

    // =========================================================================
    // CLEANUP & DEALLOCATION
    // =========================================================================
    tiledb_array_close(ctx, array);
    tiledb_array_free(&array);
    tiledb_ctx_free(&ctx);

    return 0;
}
