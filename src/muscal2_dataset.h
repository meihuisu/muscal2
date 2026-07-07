/**
 * @file muscal2_dataset.h
 *
**/

#ifndef MUSCAL2_DATASET_H
#define MUSCAL2_DATASET_H

// setup for future possible expansion
#define MUSCAL2_DATASET_MAX 1

#define MUSCAL2_CACHE_LAYER_MAX 20
#define MUSCAL2_CACHE_COL_MAX 10

#include "kdtree_util.h"
#include "tiledb/tiledb.h"

typedef struct muscal2_properties_t muscal2_properties_t;

/** The MUSCAL2, a dataset's working structure. */
/** 
       tiledb: tildb_ctx
          -file
              config ..nx/ny/nz

**/
typedef struct muscal2_dataset_t {
	/** Number of x(lon) points */
	int nx;
	/** Number of y(lat) points */
	int ny;
	/** Number of z(dep) points */
	int nz;

        /* surfaces  */
        KDNodeSetup *kdsurface;

	/** list of longitudes **/
	float *longitudes;
	/** list of latitudes **/
	float *latitudes;
	/** list of depths **/
	float *depths;

	int elems;

// for tiledb dataset
        tiledb_ctx_t *tiledb_ctx;
        char *tiledb_array_uri;

} muscal2_dataset_t;

typedef struct muscal2_pt_info_t {
	float lon;
	float lat;
	float dep;
        int lon_idx;
	int lat_idx;
	int dep_idx;
	float lon_percent;
	float lat_percent;
	float dep_percent;
} muscal2_pt_info_t;

/* utilitie functions */
muscal2_dataset_t *make_a_muscal2_dataset(char *datadir, char *datafile);

int free_muscal2_dataset(muscal2_dataset_t *data);
muscal2_cache_col_t *find_a_cache_col(muscal2_dataset_t *dataset, int target_lat_idx, int target_lon_idx);
void free_a_cache_col(muscal2_cache_col_t *col);
muscal2_cache_layer_t *find_a_cache_layer(muscal2_dataset_t *dataset, int target_dep_idx);
void free_a_cache_layer(muscal2_cache_layer_t *layer);
void add_surface_data(muscal2_dataset_t  *data, char *sfile, int s_count);

int get_one_property_binary(muscal2_dataset_t *dataset, muscal2_pt_info_t *pt, muscal2_properties_t *data);
int get_one_muscal21d_property(muscal2_dataset_t *dataset, muscal2_pt_info_t *pt, muscal2_properties_t *data);
void get_interp_property_binary(muscal2_dataset_t *dataset, muscal2_pt_info_t *pt, muscal2_properties_t *data);


#endif

