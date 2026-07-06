/** 
  
  kdtree_util.h
  -- kdtree access code

**/

#ifndef KDTREE_UTIL_H
#define KDTREE_UTIL_H

#include <stdlib.h>
#include <float.h>
#include <stdio.h>
#include <assert.h>

#define EARTH_RADIUS_M 6371000.0
#define KD_MAX_LINE 1000

extern int muscal2_ucvm_debug;
extern int muscal2_ucvm_debug_detail;
extern FILE *stderrfp;


// reference lat/lon/depth/material properties
// for 'background expansion'
typedef struct KDlld {
   float lat;
   float lon;
   float depth; 
   float vs;
   float vp;
   float density;
} KDlld;

// base on KDlld
typedef struct KDVec3 {
    float x;
    float y;
    float z;
    int lldindex;
} KDVec3;

typedef struct KDNode {
    KDVec3 point;
    int axis;  // 0=x, 1=y, 2=z
    struct KDNode *left;
    struct KDNode *right;
} KDNode;


typedef struct KDNodeSetup {
  KDlld *pnts;
  KDVec3 *vpnts;
  KDNode *nodes; // tree nodes
} KDNodeSetup;

// search result for N nearest neighbors
typedef struct KDNodeResult {
    float dist;
    KDVec3 *point;
} KDNodeResult;

typedef struct {
    float x;
    float y;
    float z;
    int lldindex; // original data index -> KDlld3
    int axis;
    int left;  // index of left child (-1 if none)
    int right; // index of right child (-1 if none)
} KDNodeDisk;


/** access **/ 
void lld_to_xyz(KDVec3 *p, float lat, float lon, float depth, int idx);
KDNode* build_kdtree(KDVec3 *pts, int n, int depth);
void free_kdtree(KDNode *node);
int flatten_kdtree(KDNode *node, KDNodeDisk *out, int *pos);
void write_flatten_kdtree(const char *fname, KDNodeDisk *nodes, int n);
KDNodeDisk *read_flatten_kdtree(const char *fname, int n);

/** usage **/
float dist_sq(KDVec3* a, KDVec3* b);
void kdtree_nearest(KDNode *root, KDVec3* query, KDVec3 **best, float *best_dist);
void kdtree_nearest_full(KDlld *pnts, KDNode *root, KDVec3* query, KDVec3 **best, float *best_dist);
void kdtree_n_nearest(KDNode *root, KDVec3 *query, int N, KDVec3 **best_nodes, float *best_dists);

#endif // KDTREE_UTIL_H
