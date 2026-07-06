/**
 * @file muscal2.h
 * @brief Main header file for MUSCAL2 library.
 * @version 1.0
 *
 * Delivers the MUSCAL2 model 
 *
 */
#ifndef MUSCAL2_H
#define MUSCAL2_H

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "muscal2_dataset.h"
#include "muscal2_util.h"

/** Defines a return value of success */
#define SUCCESS 0
/** Defines a return value of failure */
#define FAIL 1

/* config string */
#define MUSCAL2_CONFIG_MAX 1000
#define MUSCAL2_DATASET_MAX 1

extern int muscal2_ucvm_debug;
extern int muscal2_ucvm_debug_detail;
extern FILE *stderrfp;

// Structures
/** Defines a point (latitude, longitude, and depth) in WGS84 format */
typedef struct muscal2_point_t {
	/** Longitude member of the point */
	double longitude;
	/** Latitude member of the point */
	double latitude;
	/** Depth member of the point */
	double depth;
} muscal2_point_t;

/** Defines the material properties this model will retrieve. */
typedef struct muscal2_properties_t {
	/** P-wave velocity in meters per second */
	double vp;
	/** S-wave velocity in meters per second */
	double vs;
	/** Density in g/m^3 */
	double rho;
	/** Qp */
	double qp;
	/** Qs */
	double qs;
} muscal2_properties_t;

/**
Dimensions: 3
  dim[0] name=depth len=84
  dim[1] name=latitude len=381
  dim[2] name=longitude len=471
Total elements: 15073884
**/

/** The MUSCAL2 configuration structure. */
typedef struct muscal2_configuration_t {
	/** The zone of UTM projection */
	int utm_zone;
	/** The model directory */
	char model_dir[128];

	/** interpolation on or off (1 or 0) */
	int interpolation;

	/** use_binary on or off (1 or 0) */
	int use_binary;
	/** use_tiledb on or off (1 or 0) */
	int use_tiledb;

	/** too_big on or off (1 or 0) */
	int too_big;
	/** add muscal21d on or off (1 or 0) */
	int enable_1d;

        /* how many datasets are in the model */
        int dataset_cnt;
        char *dataset_files[MUSCAL2_DATASET_MAX];  //strdup
	char *dataset_labels[MUSCAL2_DATASET_MAX]; // strdup
        char *surface_files[MUSCAL2_DATASET_MAX];  //strdup
        int surface_counts[MUSCAL2_DATASET_MAX];
						  
} muscal2_configuration_t;

typedef struct muscal2_model_t {
        int dataset_cnt;
        muscal2_dataset_t *datasets[MUSCAL2_DATASET_MAX];
} muscal2_model_t;


// Constants
/** The version of the model. */
extern const char *muscal2_version_string;

// Variables
/** Set to 1 when the model is ready for query. */
extern int muscal2_is_initialized;

/** Configuration parameters. */
extern muscal2_configuration_t *muscal2_configuration;

/** Holds pointers to the velocity model data OR indicates it can be read from file. */
extern muscal2_model_t *muscal2_velocity_model;

// UCVM API Required Functions

#ifdef DYNAMIC_LIBRARY

/** Initializes the model */
int model_init(const char *dir, const char *label);
/** Cleans up the model (frees memory, etc.) */
int model_finalize();
/** Returns version information */
int model_version(char *ver, int len);
/** Queries the model */
int model_query(muscal2_point_t *points, muscal2_properties_t *data, int numpts);

int (*get_model_init())(const char *, const char *);
int (*get_model_query())(muscal2_point_t *, muscal2_properties_t *, int);
int (*get_model_finalize())();
int (*get_model_version())(char *, int);

#endif

// MUSCAL2 Related Functions

/** Initializes the model */
int muscal2_init(const char *dir, const char *label);
/** Cleans up the model (frees memory, etc.) */
int muscal2_finalize();
/** Returns version information */
int muscal2_version(char *ver, int len);
/** Queries the model */
int muscal2_query(muscal2_point_t *points, muscal2_properties_t *data, int numpts);

// Non-UCVM Helper Functions
//
/** Reads the configuration file and helper functions. */
int muscal2_read_configuration(char *file, muscal2_configuration_t *config);
int muscal2_configuration_finalize(muscal2_configuration_t *config);

/** Prints out the error string. */
void muscal2_print_error(char *err);
/** Retrieves the value at a specified grid point in the model. */
void muscal2_read_properties(int x, int y, int z, muscal2_properties_t *data);
/** Attempts to malloc the model size in memory and read it in. */
int muscal2_read_model(muscal2_configuration_t *config, muscal2_model_t *model, char* dir);
/** toggle debug flag **/
void muscal2_setdebug();

/** helper function for velocity_model **/
int muscal2_velocity_model_init(muscal2_model_t *model);
int muscal2_velocity_model_finalize(muscal2_model_t *model);

/** parse JSON metadata blob per dataset **/
int _setup_a_dataset(muscal2_configuration_t *conf, char *blobstr);

void _trimLast(char *str, char m);
void _splitline(char* lptr, char key[], char value[]);

#endif
