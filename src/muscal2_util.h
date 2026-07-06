/**
 * @file muscal2_util.h
 *
**/

#ifndef MUSCAL2_UTIL_H
#define MUSCAL2_UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <math.h>
#include <ctype.h>

extern int debug;

float *get_binary_float_buffer(const char *path, char *datafile, int total);

int find_buffer_idx(float *buffer, size_t nelems, float target);
int find_buffer_idx_clamped(float *buffer, size_t nelems, float target);
float find_cell_percent(float *buffer, float target, int idx);

void dump_corners(FILE *fp, float *corners);
float *float_array_it(const char *str, size_t *n);

#endif

