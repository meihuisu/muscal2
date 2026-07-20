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
#include "muscal2.h"
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
        tiledb_array_t *tiledb_array;
        char *tiledb_uri;

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
muscal2_dataset_t * muscal2_read_dataset(char *datadir, char *datafile);
void muscal2_read_surface(muscal2_dataset_t *model, int count, char *datadir, char *surface_file);

int get_one_property(muscal2_dataset_t *model, muscal2_pt_info_t *pt, muscal2_properties_t *data);
int get_interp_property(muscal2_dataset_t *model, muscal2_pt_info_t *pt, muscal2_properties_t *data);
int get_1dnn_property(muscal2_dataset_t *model, muscal2_pt_info_t *pt, muscal2_properties_t *data);

void add_surface_data(muscal2_dataset_t  *model, char *sfile, int s_count);
void dump_dataset_metadata(muscal2_dataset_t *model);
int free_muscal2_dataset(muscal2_dataset_t *model);

#endif

