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
const char *ARRAY_URI = "/var/www/html/UCVM_rel/model/muscal2/tools/muscal_tiledb";

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

#endif
