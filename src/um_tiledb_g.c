/* 
   um_tiledb_g.c
*/

#include "muscal2_util.h"
#include "um_tiledb_g.h"

int create_tiledb_dense_3d_array(tiledb_ctx_t *ctx, const char *array_uri) {
  int rc;
  tiledb_attribute_t *a1 = NULL, *a2 = NULL, *a3 = NULL;
  tiledb_dimension_t *d1 = NULL, *d2 = NULL, *d3 = NULL;
  tiledb_domain_t *domain = NULL;
  tiledb_array_schema_t *schema = NULL;

  tiledb_object_t obj_type;
  rc = tiledb_object_type(ctx, array_uri, &obj_type);
  if (rc == TILEDB_OK && obj_type == TILEDB_ARRAY) {
    printf("Array already exists: %s\n", array_uri);
    return 0;
  }

  /* 3 Attributes matching NetCDF variables */
  rc = tiledb_attribute_alloc(ctx, "vp", TILEDB_FLOAT32, &a1);
  rc |= tiledb_attribute_alloc(ctx, "vs", TILEDB_FLOAT32, &a2);
  rc |= tiledb_attribute_alloc(ctx, "rho", TILEDB_FLOAT32, &a3);
  if (rc != TILEDB_OK) { fprintf(stderr, "Error allocating attributes\n"); return -1; }

  /* 1-based index boundaries matching convert_nc_to_tiledb schema configuration */
  int32_t dim_bounds[] = {1, DEPTH_MAX, 1, LAT_MAX, 1, LON_MAX};
  int32_t tile_extents[] = {30, 50, 50}; 

  rc = tiledb_dimension_alloc(ctx, "depth", TILEDB_INT32, &dim_bounds[0], &tile_extents[0], &d1);
  rc |= tiledb_dimension_alloc(ctx, "latitude", TILEDB_INT32, &dim_bounds[2], &tile_extents[1], &d2);
  rc |= tiledb_dimension_alloc(ctx, "longitude", TILEDB_INT32, &dim_bounds[4], &tile_extents[2], &d3);
  if (rc != TILEDB_OK) { fprintf(stderr, "Error allocating dimensions\n"); goto cleanup; }

  rc = tiledb_domain_alloc(ctx, &domain);
  if (rc != TILEDB_OK) goto cleanup;
  rc = tiledb_domain_add_dimension(ctx, domain, d1);
  rc |= tiledb_domain_add_dimension(ctx, domain, d2);
  rc |= tiledb_domain_add_dimension(ctx, domain, d3);

  rc = tiledb_array_schema_alloc(ctx, TILEDB_DENSE, &schema);
  if (rc != TILEDB_OK) goto cleanup;

  rc = tiledb_array_schema_set_cell_order(ctx, schema, TILEDB_ROW_MAJOR);
  rc |= tiledb_array_schema_set_tile_order(ctx, schema, TILEDB_ROW_MAJOR);
  rc |= tiledb_array_schema_set_domain(ctx, schema, domain);
  rc |= tiledb_array_schema_add_attribute(ctx, schema, a1);
  rc |= tiledb_array_schema_add_attribute(ctx, schema, a2);
  rc |= tiledb_array_schema_add_attribute(ctx, schema, a3);
  if (rc != TILEDB_OK) goto cleanup;

  rc = tiledb_array_create(ctx, array_uri, schema);
  if (rc == TILEDB_OK) printf("Initialized mock array: %s\n", array_uri);

cleanup:
  if (a1) tiledb_attribute_free(&a1); if (a2) tiledb_attribute_free(&a2); if (a3) tiledb_attribute_free(&a3);
  if (d1) tiledb_dimension_free(&d1); if (d2) tiledb_dimension_free(&d2); if (d3) tiledb_dimension_free(&d3);
  if (domain) tiledb_domain_free(&domain);
  if (schema) tiledb_array_schema_free(&schema);
  return rc == TILEDB_OK ? 0 : -1;
}

int read_tiledb_voxel_corner_values(tiledb_ctx_t *ctx, const char *array_uri,
                             int32_t idep, int32_t ilat, int32_t ilon,
                             float vp_out[8], float vs_out[8], float rho_out[8]) {
  int rc;
  tiledb_array_t *array = NULL;
  tiledb_query_t *query = NULL;
  tiledb_subarray_t *subarray_obj = NULL;

  rc = tiledb_array_alloc(ctx, array_uri, &array);
  if (rc != TILEDB_OK) return -1;

  rc = tiledb_array_open(ctx, array, TILEDB_READ);
  if (rc != TILEDB_OK) { tiledb_array_free(&array); return -1; }

  /* Subarray selection bounds matching (depth, latitude, longitude) dimensions */
  int32_t subarray_bounds[6];
  subarray_bounds[0] = idep; subarray_bounds[1] = idep + 1;
  subarray_bounds[2] = ilat; subarray_bounds[3] = ilat + 1;
  subarray_bounds[4] = ilon; subarray_bounds[5] = ilon + 1;

  fprintf(stderr,".. subarray_bounds ..\n");
  fprintf(stderr,"   0: %d \n", idep);
  fprintf(stderr,"   2: %d \n", ilat);
  fprintf(stderr,"   4: %d \n", ilon);

  uint64_t size_vp = 8 * sizeof(float);
  uint64_t size_vs = 8 * sizeof(float);
  uint64_t size_rho = 8 * sizeof(float);

  rc = tiledb_query_alloc(ctx, array, TILEDB_READ, &query);
  rc |= tiledb_subarray_alloc(ctx, array, &subarray_obj);
  if (rc != TILEDB_OK) goto cleanup;

  rc = tiledb_subarray_set_subarray(ctx, subarray_obj, subarray_bounds);
  rc |= tiledb_query_set_subarray_t(ctx, query, subarray_obj);
  rc |= tiledb_query_set_layout(ctx, query, TILEDB_ROW_MAJOR);
  if (rc != TILEDB_OK) goto cleanup;

  /* Bind space buffers for all three geological attributes */
  rc = tiledb_query_set_data_buffer(ctx, query, "vp", vp_out, &size_vp);
  rc |= tiledb_query_set_data_buffer(ctx, query, "vs", vs_out, &size_vs);
  rc |= tiledb_query_set_data_buffer(ctx, query, "rho", rho_out, &size_rho);
  if (rc != TILEDB_OK) goto cleanup;


if (tiledb_query_submit(ctx, query) != TILEDB_OK) {
    tiledb_error_t* err = NULL;
    tiledb_ctx_get_last_error(ctx, &err);
    const char* msg = NULL;
    tiledb_error_message(err, &msg);
    fprintf(stderr, "TileDB Query Failure: %s\n", msg);
    tiledb_error_free(&err);
}


cleanup:
  if (subarray_obj) tiledb_subarray_free(&subarray_obj);
  if (query) tiledb_query_free(&query);
  if (array) {
    tiledb_array_close(ctx, array);
    tiledb_array_free(&array);
  }
  return rc == TILEDB_OK ? 0 : -1;
}

float trilinear_interp_tiledb(const float c[8], float fdep, float flat, float flon) {
  /* Layout Mapping positions inside row-major continuous data:
     c[0] = (d,   lat,   lon)       c[1] = (d,   lat,   lon+1)
     c[2] = (d,   lat+1, lon)       c[3] = (d,   lat+1, lon+1)
     c[4] = (d+1, lat,   lon)       c[5] = (d+1, lat,   lon+1)
     c[6] = (d+1, lat+1, lon)       c[7] = (d+1, lat+1, lon+1) */

  /* Interpolate along fastest changing axis (longitude) */
  float c00 = c[0] * (1.0f - flon) + c[1] * flon; /* (depth, lat) */
  float c10 = c[2] * (1.0f - flon) + c[3] * flon; /* (depth, lat+1) */
  float c01 = c[4] * (1.0f - flon) + c[5] * flon; /* (depth+1, lat) */
  float c11 = c[6] * (1.0f - flon) + c[7] * flon; /* (depth+1, lat+1) */

  /* Interpolate along intermediate axis (latitude) */
  float c0 = c00 * (1.0f - flat) + c10 * flat; /* depth floor slice plane */
  float c1 = c01 * (1.0f - flat) + c11 * flat; /* depth ceiling slice plane */

  /* Interpolate along slowest axis (depth) */
  return c0 * (1.0f - fdep) + c1 * fdep;
}

//XXX

int main(int argc, char *const argv[]) {
  int rc;
  tiledb_ctx_t *ctx = NULL;

  rc = tiledb_ctx_alloc(NULL, &ctx);
  if (rc != TILEDB_OK) { fprintf(stderr, "Error: tiledb_ctx_alloc\n"); return 1; }

  /* Target floating-point coordinates inside real spatial domains
     Bounds: 1.0 <= depth <= 210.0, 1.0 <= lat <= 1251.0, 1.0 <= lon <= 1301.0 */

  char line[1024]; 
  double r_depth = 0.0; 
  double r_lat   = 0.0; 
  double r_lon   = 0.0;  

  fprintf(stderr,"\n INPUT: lon_idx lat_idx dep_idx (start at 1)\n");

  while (fgets(line, 1024, stdin) != NULL) {

    if(line[0]=='#') continue;  // a comment line
    if (sscanf(line,"%lf %lf %lf", &r_lon, &r_lat, &r_depth) == 3) {
    
      /* Compute lower integer base structures */
      int32_t idep   = (int32_t)floor(r_depth);
      int32_t ilat = (int32_t)floor(r_lat);
      int32_t ilon = (int32_t)floor(r_lon);
    
      /* Boundaries protection validator */
      if (idep < 1 || ilat < 1 || ilon < 1 || idep + 1 > DEPTH_MAX || ilat + 1 > LAT_MAX || ilon + 1 > LON_MAX) {
        fprintf(stderr, "Fatal Error: Coordinates fall out of matrix interpolation grid boundaries.\n");
        tiledb_ctx_free(&ctx);
        return 1;
      }
    
      /* Compute precise unit cell fractional spaces scales */
      float fdep = (float)(r_depth - (double)idep);
      float flat = (float)(r_lat - (double)ilat);
      float flon = (float)(r_lon - (double)ilon);
    
      /* Storage vectors for voxel corners extraction */
      float corners_vp[8], corners_vs[8], corners_rho[8];
    
      printf("\nQuerying 2x2x2 voxel cube starting at Base Index Node: idep,ilat,ilon (%d, %d, %d)...\n", idep, ilat, ilon);
      idep++;
      ilat++;
      ilon++;
      if (read_voxel_corner_values(ctx, ARRAY_URI, idep, ilat, ilon, corners_vp, corners_vs, corners_rho) != 0) {
        fprintf(stderr, "Query execution extraction failure. Make sure array data exists.\n");
        tiledb_ctx_free(&ctx);
        return 1;
      }

      dump_corners(corners_vp);
    
      /* Run mathematical trilinear interpolation equations over extracted corners */
      float interp_vp  = trilinear_interp(corners_vp, fdep, flat, flon);
      float interp_vs  = trilinear_interp(corners_vs, fdep, flat, flon);
      float interp_rho = trilinear_interp(corners_rho, fdep, flat, flon);
    
      printf("--- Interp Profile Results at Coordinate rdep,rlat,rlon(%.3f, %.3f, %.3f) ---\n", r_depth, r_lat, r_lon);
      printf(" -> vp  (P-Wave Velocity) : %.4f m/s\n", interp_vp);
      printf(" -> vs  (S-Wave Velocity) : %.4f m/s\n", interp_vs);
      printf(" -> rho (Mass Density)    : %.4f kg/m^3\n", interp_rho);
    }
  }
  
  tiledb_ctx_free(&ctx);
  return 0;
}


