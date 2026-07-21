/**         
    lookup_nc_point.c
    
    Usage: ./lookup_nc_point
                
LOOK:
    const char* nc_filename = "/var/www/html/CVM_DATASET_DIRECTORY/model/muscal/model_MUSCAL_CANVAS_dll0.01_vardz_float32_cmpd.nc";

    // --- Hardcoded geographic inputs for testing ---
    float input_depth = 45.2f;   // in meters
    float input_lat   = 34.0025f; // in degrees
    float input_lon   = -118.241f; // in degrees

**/             

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <netcdf.h>

// Handle NetCDF errors safely
#define ERR(e) {if (e) {fprintf(stderr, "[NetCDF Error] %s\n", nc_strerror(e)); exit(-1);}}

typedef struct {
    int32_t nearest_idx;
    float fraction;
} LookupResult;

// Core mathematical mapping function (same algorithm, direction-agnostic)
LookupResult find_nearest_and_fraction(const float* coord_array, size_t total_elements, float target_value) {
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

    for (size_t i = 0; i < total_elements - 1; i++) {
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
    const char* nc_filename = "/var/www/html/CVM_DATASET_DIRECTORY/model/muscal/model_MUSCAL_CANVAS_dll0.01_vardz_float32_cmpd.nc";

    // --- Hardcoded geographic inputs for testing ---
    float input_depth = 45.2f;   // in meters
    float input_lat   = 34.0025f; // in degrees
    float input_lon   = -118.241f; // in degrees

    int ncid;
    int dep_dimid, lat_dimid, lon_dimid;
    size_t dep_len, lat_len, lon_len;
    int dep_varid, lat_varid, lon_varid;
    int vp_varid, vs_varid, rho_varid;

    // Open the NetCDF file in read-only mode
    ERR(nc_open(nc_filename, NC_NOWRITE, &ncid));

    // 1. Get Dimension IDs and their respective lengths
    ERR(nc_inq_dimid(ncid, "depth", &dep_dimid));
    ERR(nc_inq_dimlen(ncid, dep_dimid, &dep_len));

    ERR(nc_inq_dimid(ncid, "latitude", &lat_dimid));
    ERR(nc_inq_dimlen(ncid, lat_dimid, &lat_len));

    ERR(nc_inq_dimid(ncid, "longitude", &lon_dimid));
    ERR(nc_inq_dimlen(ncid, lon_dimid, &lon_len));

    // 2. Allocate memory and read the 1D Coordinate Variables
    float* depth_coords = (float*)malloc(dep_len * sizeof(float));
    float* lat_coords   = (float*)malloc(lat_len * sizeof(float));
    float* lon_coords   = (float*)malloc(lon_len * sizeof(float));

    ERR(nc_inq_varid(ncid, "depth", &dep_varid));
    ERR(nc_get_var_float(ncid, dep_varid, depth_coords));

    ERR(nc_inq_varid(ncid, "latitude", &lat_varid));
    ERR(nc_get_var_float(ncid, lat_varid, lat_coords));

    ERR(nc_inq_varid(ncid, "longitude", &lon_varid));
    ERR(nc_get_var_float(ncid, lon_varid, lon_coords));

    // 3. Compute nearest indexes and structural cell fractions
    LookupResult dep_res = find_nearest_and_fraction(depth_coords, dep_len, input_depth);
    LookupResult lat_res = find_nearest_and_fraction(lat_coords, lat_len, input_lat);
    LookupResult lon_res = find_nearest_and_fraction(lon_coords, lon_len, input_lon);

    // 4. Retrieve single-point values from the 3D variables (vp, vs, rho)
    // NetCDF expects arrays specifying the starting index (`start`) and counts (`count`)
    size_t start[3] = { (size_t)dep_res.nearest_idx, (size_t)lat_res.nearest_idx, (size_t)lon_res.nearest_idx };
    size_t count[3] = { 1, 1, 1 }; // Pull exactly one cell

    float val_vp = 0.0f;
    float val_vs = 0.0f;
    float val_rho = 0.0f;

    ERR(nc_inq_varid(ncid, "vp", &vp_varid));
    ERR(nc_get_vara_float(ncid, vp_varid, start, count, &val_vp));

    ERR(nc_inq_varid(ncid, "vs", &vs_varid));
    ERR(nc_get_vara_float(ncid, vs_varid, start, count, &val_vs));

    ERR(nc_inq_varid(ncid, "rho", &rho_varid));
    ERR(nc_get_vara_float(ncid, rho_varid, start, count, &val_rho));

    // 5. Output Results
    printf("===================================================\n");
    printf("NETCDF DIRECT GEOGRAPHIC MAPPING RESULTS\n");
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
           depth_coords[dep_res.nearest_idx], lat_coords[lat_res.nearest_idx], lon_coords[lon_res.nearest_idx]);
    printf("---------------------------------------------------\n");
    printf("Material Values  -> VP  : %f m/s\n", val_vp);
    printf("                    VS  : %f m/s\n", val_vs);
    printf("                    RHO : %f kg/m^3\n", val_rho);
    printf("===================================================\n");

    // Clean up memory and file descriptors
    free(depth_coords);
    free(lat_coords);
    free(lon_coords);
    ERR(nc_close(ncid));

    return 0;
}

