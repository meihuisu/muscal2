/* 
   um_tiledb_g.h

   Direct TileDB Trilinear Interpolation Tool

   Updated for TileDB v2.30.0+ Compliant API
   Fully mapped to: model_MUSCAL_CANVAS_dll0.01_vardz_float32_cmpd Schema

   Compile:
     gcc10-gcc -g -I${UCVM_INSTALL_PATH}/lib/netcdf/include -I/home/mei/muscal-git/TileDB/opt/tiledb/include
        -o trilinear_tiledb trilinear_tiledb.c -L${UCVM_INSTALL_PATH}/lib/netcdf/lib -lnetcdf 
        -L/home/mei/muscal-git/TileDB/opt/tiledb/lib64 -ltiledb 
        -Wl,-rpath,/home/mei/muscal-git/TileDB/opt/tiledb/lib64 -lm
*/


#ifndef UM_TILEDB_G_H
#define UM_TILEDB_G_H

#include <inttypes.h>
#include "tiledb/tiledb.h"

/* Match target location created by conversion pipeline */
const char *ARRAY_URI = "/var/www/html/UCVM_rel/model/muscal2/tools/muscal_tiledb"

/* Global dataset matrix bounds (use 1-indexed matching conversion pipeline) */
#define DEPTH_MAX 210
#define LAT_MAX   1251
#define LON_MAX   1301

/* ------------------------------------------------------------------
 * Optional Utility: Create a dense 3D array matching the NC layout 
 * (Useful if running mock or testing logic independently)
 * ------------------------------------------------------------------ */
int create_tiledb_dense_3d_array(tiledb_ctx_t *ctx, const char *array_uri);

/* ------------------------------------------------------------------
 * Reads an intertwined 2x2x2 sub-bounding block for all attributes.
 * Expects 1-based int32_t indices matching dataset coordinates.
 * ------------------------------------------------------------------ */
int read_tiledb_voxel_corner_values(tiledb_ctx_t *ctx, const char *array_uri,
                             int32_t idep, int32_t ilat, int32_t ilon,
                             float vp_out[8], float vs_out[8], float rho_out[8]);

/* ------------------------------------------------------------------
 * Trilinear Interp execution mapped natively to TILEDB_ROW_MAJOR order:
 * Dimension speed layout: depth (slowest), latitude, longitude (fastest).
 * ------------------------------------------------------------------ */
float trilinear_interp_tiledb(const float c[8], float fdep, float flat, float flon);

/* ------------------------------------------------------------------
 * Main Execution Block
 * ------------------------------------------------------------------ */

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


#endif
