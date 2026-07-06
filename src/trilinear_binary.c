/* 
   trilinear_binary.c

   Direct Binary Trilinear Interpolation Tool
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define VP_FILE "/var/www/html/CVM_DATASET_DIRECTORY/model/muscal/vp.dat"
#define VS_FILE "/var/www/html/CVM_DATASET_DIRECTORY/model/muscal/vs.dat"
#define RHO_FILE "/var/www/html/CVM_DATASET_DIRECTORY/model/muscal/rho.dat"

/* NetCDF Matrix Dimensions (0-indexed max boundaries) */
#define DEPTH_MAX 210
#define LAT_MAX   1251
#define LON_MAX   1301

/* Macro for clean error handling */
#define ERR(e) {printf("Binary Error: %s\n", nc_strerror(e)); exit(1);}

/* ------------------------------------------------------------------
 * Extract a 2x2x2 Hyperslab for a specific variable
 * ------------------------------------------------------------------ */
int _offset(int lon_i, int lat_i, int dep_i) {
    int nx=LON_MAX;
    int ny=LAT_MAX;
    int offset= (dep_i)*(ny * nx)+(lat_i)*(nx)+lon_i;
    return offset;
}

void read_voxel_corner_values_binary(float *buffer, 
                                 int32_t dep_idx, int32_t lat_idx, int32_t lon_idx, 
                                 float outbuf[8]) {
    int retval;

    outbuf[0]= buffer[_offset(lon_idx,lat_idx,dep_idx)];      // x,    y, z
    outbuf[1]= buffer[_offset(lon_idx+1,lat_idx,dep_idx)];    // x+1,  y, z 
    outbuf[2]= buffer[_offset(lon_idx,lat_idx+1,dep_idx)];    // x,  y+1, z 
    outbuf[3]= buffer[_offset(lon_idx+1,lat_idx+1,dep_idx)];  // x+1,y+1, z
    outbuf[4]= buffer[_offset(lon_idx,lat_idx,dep_idx+1)];    // x,    y, z+1
    outbuf[5]= buffer[_offset(lon_idx+1,lat_idx,dep_idx+1)];  // x+1,  y, z+1
    outbuf[6]= buffer[_offset(lon_idx,lat_idx+1,dep_idx+1)];  // x,  y+1, z+1
    outbuf[7]= buffer[_offset(lon_idx+1,lat_idx+1,dep_idx+1)];// x+1,y+1, z+1
}

/* ------------------------------------------------------------------
 * Trilinear Interp execution mapped natively to Row-Major order:
 * Dimension speed layout: depth (slowest), latitude, longitude (fastest).
 * ------------------------------------------------------------------ */
float trilinear_interp(const float c[8], float fd, float flat, float flon) {
    /* NetCDF populates the 8-element array in standard C row-major layout:
       c[0] = (d,   lat,   lon)       c[1] = (d,   lat,   lon+1)
       c[2] = (d,   lat+1, lon)       c[3] = (d,   lat+1, lon+1)
       c[4] = (d+1, lat,   lon)       c[5] = (d+1, lat,   lon+1)
       c[6] = (d+1, lat+1, lon)       c[7] = (d+1, lat+1, lon+1) */

    /* Interpolate along fastest changing axis (longitude) */
    float c00 = c[0] * (1.0f - flon) + c[1] * flon; 
    float c10 = c[2] * (1.0f - flon) + c[3] * flon; 
    float c01 = c[4] * (1.0f - flon) + c[5] * flon; 
    float c11 = c[6] * (1.0f - flon) + c[7] * flon; 

    /* Interpolate along intermediate axis (latitude) */
    float c0 = c00 * (1.0f - flat) + c10 * flat; 
    float c1 = c01 * (1.0f - flat) + c11 * flat; 

    /* Interpolate along slowest axis (depth) */
    return c0 * (1.0f - fd) + c1 * fd;
}

/* ------------------------------------------------------------------
 * Main Execution Block
 * ------------------------------------------------------------------ */
void dump_corners(float *corners) {
  // should be 8 values 
  fprintf(stderr,"%lf %lf %lf %lf %lf %lf %lf %lf\n",
    corners[0], corners[1], corners[2], corners[3], corners[4], corners[5], corners[6], corners[7]);
}

int main(int argc, char *const argv[]) {
    int retval;
    size_t read_count;
    int total= LON_MAX * LAT_MAX * DEPTH_MAX;

    float *buffer_vp= (float *) malloc( total * sizeof(float));
    float *buffer_vs= (float *) malloc( total * sizeof(float));
    float *buffer_rho= (float *) malloc( total * sizeof(float));

    // --- 1. OPEN BINARY DATASET ---
    FILE *fp_vp=fopen(VP_FILE,"rb");
    read_count = fread(buffer_vp, sizeof(float), total, fp_vp);
    if (read_count != total) {
        printf("Warning: read %zu elements (expected %d)\n", read_count, total);
    }
    fclose(fp_vp);

    FILE *fp_vs=fopen(VS_FILE,"rb");
    read_count = fread(buffer_vs, sizeof(float), total, fp_vs);
    if (read_count != total) {
        printf("Warning: read %zu elements (expected %d)\n", read_count, total);
    }
    fclose(fp_vs);

    FILE *fp_rho=fopen(RHO_FILE,"rb");
    read_count = fread(buffer_rho, sizeof(float), total, fp_rho);
    if (read_count != total) {
        printf("Warning: read %zu elements (expected %d)\n", read_count, total);
    }
    fclose(fp_rho);


    // --- 2. CALCULATE SPATIAL COORDINATES ---
    /* Target floating-point coordinates inside real spatial domains.
       Using native 0-based index spaces: 0.0 <= coords < MAX - 1 */
    char line[1024];
    double r_depth = 0; 
    double r_lat   = 0; 
    double r_lon   = 0;  


  fprintf(stderr,"\n INPUT: lon_idx lat_idx dep_idx (start at 0)\n");

  while (fgets(line, 1024, stdin) != NULL) {

    if(line[0]=='#') continue;  // a comment line
    if (sscanf(line,"%lf %lf %lf", &r_lon, &r_lat, &r_depth) == 3) {

      /* Compute lower integer base nodes */
      int32_t idep = (int32_t)floor(r_depth);
      int32_t ilat = (int32_t)floor(r_lat);
      int32_t ilon = (int32_t)floor(r_lon);

      /* Native NetCDF Boundaries protection validator */
      if (idep < 0 || ilat < 0 || ilon < 0 || 
          idep + 1 >= DEPTH_MAX || ilat + 1 >= LAT_MAX || ilon + 1 >= LON_MAX) {
          fprintf(stderr, "Fatal Error: Coordinates fall out of Binary grid boundaries.\n");
          return 1;
      }

      /* Compute precise unit cell fractional offset values */
      float fdep = (float)(r_depth - (double)idep);
      float flat = (float)(r_lat - (double)ilat);
      float flon = (float)(r_lon - (double)ilon);
  
      // --- 3. FETCH CORNERS & INTERPOLATE ---
      float corners_vp[8], corners_vs[8], corners_rho[8];

      printf("\nQuerying Binary 2x2x2 buffer starting at index: (%d, %d, %d)...\n", idep, ilat, ilon);
    
      read_voxel_corner_values_binary(buffer_vp, idep, ilat, ilon, corners_vp);
      read_voxel_corner_values_binary(buffer_vs, idep, ilat, ilon, corners_vs);
      read_voxel_corner_values_binary(buffer_rho, idep, ilat, ilon, corners_rho);

      dump_corners(corners_vp);
  
      /* Run mathematical trilinear interpolation equations over extracted corners */
      float interp_vp  = trilinear_interp(corners_vp, fdep, flat, flon);
      float interp_vs  = trilinear_interp(corners_vs, fdep, flat, flon);
      float interp_rho = trilinear_interp(corners_rho, fdep, flat, flon);

      printf("--- Binary Interp Profile Results at Coordinate (%.3f, %.3f, %.3f) ---\n", r_depth, r_lat, r_lon);
      printf(" -> vp  (P-Wave Velocity) : %.4f m/s\n", interp_vp);
      printf(" -> vs  (S-Wave Velocity) : %.4f m/s\n", interp_vs);
      printf(" -> rho (Mass Density)    : %.4f kg/m^3\n", interp_rho);
    }
  }
  
  // --- 4. CLEANUP ---
  free(buffer_vp);
  free(buffer_vs);
  free(buffer_rho);
  return 0;
}

