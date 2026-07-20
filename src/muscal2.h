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

/** The MUSCAL2 configuration structure. */
typedef struct muscal2_configuration_t {
	/** The zone of UTM projection */
	int utm_zone;
	/** The model directory */
	char model_dir[128];

	/** interpolation on or off (1 or 0) */
	int interpolation;

	/** add 1d on or off (1 or 0) */
	int enable_1d;

        /* how many datasets are in the model */
        char *dataset_file;  //strdup
	char *dataset_label; // strdup
        char *surface_file;  //strdup
        int surface_count;
} muscal2_configuration_t;

// Constants
/** The version of the model. */
extern const char *muscal2_version_string;

// Variables
/** Set to 1 when the model is ready for query. */
extern int muscal2_is_initialized;

/** Configuration parameters. */
extern muscal2_configuration_t *muscal2_configuration;

/** Holds pointers to the velocity model data. */
extern muscal2_dataset_t *muscal2_dataset;

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
/** toggle debug flag **/
void muscal2_setdebug();

/** parse JSON metadata blob per dataset **/
int _setup_a_dataset(muscal2_configuration_t *conf, char *blobstr);

void _trimLast(char *str, char m);
void _splitline(char* lptr, char key[], char value[]);

#endif
