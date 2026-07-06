/** 
  
  kdtree_util.c.c

**/

#include <math.h>
#include "kdtree_util.h"

static inline float deg2rad(float d) {
    return d * M_PI / 180.0;
}

void lld_to_xyz_rough(KDVec3 *p, float lat, float lon, float depth, int idx)
{
    float r = EARTH_RADIUS_M - depth;
    float latr = deg2rad(lat);
    float lonr = deg2rad(lon);

    p->x = r * cos(latr) * cos(lonr);
    p->y = r * cos(latr) * sin(lonr);
    p->z = r * sin(latr);
    p->lldindex=idx;
}

// to TRUE ECEF global space -- global 3D Cartesian space
/** GEMINI
You have hit the nail completely on the head. Your intuition about the axes having wildly 
different scales is spot on, but the reality is even more fascinating: your KD-tree isn't 
broken at all. It is doing its job 100% correctly. The point at -114° Lon, 33° Lat, 950m Depth
is mathematically closer in 3D straight-line space to your query than the point at 250m depth.
Here is exactly why this mind-bending geometric quirk is happening.
The ECEF "Corner-Cutting" Effect
Because you are converting Geodetic coordinates (Latitude, Longitude, Depth) into 3D 
Cartesian coordinates (ECEF x,y,z), your KD-tree is calculating distances as straight lines 
piercing through the Earth's crust rather than curving along its surface.  When you look at 
a globe, lines of longitude form a wedge shape that converges as you move toward the center 
of the Earth. Because of this convergence, going deeper actually shrinks the horizontal 
distance between lines of longitude.  If we calculate the exact Euclidean distances in 
meters using WGS84 ECEF coordinates for your points, look at what happens:

Point Role	Coordinates (Lon, Lat, Depth)	Straight-Line Distance to Query
Your Query	        -113°, 33°, 250m	Baseline
What You Expected	-114°, 33°, 250m	93,448.37 meters
What the Tree Found	-114°, 33°, 950m	93,445.87 meters

By dropping from 250m depth to 950m depth, you are moving closer to the Earth's axis. 
Because the horizontal gap between -113° and -114° longitude is massive (roughly 93.4 
kilometers) compared to the vertical depth change (700 meters), "cutting the corner" 
deeper into the Earth actually saves about 2.5 meters of straight-line distance. 
In fact, due to the Earth's curvature, the absolute closest 3D straight-line point 
on that -114° meridian plane is roughly at a depth of 1,220 meters! The KD-tree just 
picked the closest available depth option you gave it.
**/
void lld_to_xyz_global(KDVec3 *p, float lat, float lon, float depth, int lldidx)
{
    const double a = 6378137.0;            // WGS84 semi-major axis
    const double e2 = 6.69437999014e-3;    // eccentricity^2
    
    double nlat = lat * M_PI / 180.0;
    double nlon = lon * M_PI / 180.0;
    double h = depth * -1.0;  // depth → positive depth

    double sin_lat = sin(nlat);
    double cos_lat = cos(nlat);

    double N = a / sqrt(1.0 - e2 * sin_lat * sin_lat);

    p->x = (N + h) * cos_lat * cos(nlon);
    p->y = (N + h) * cos_lat * sin(nlon);
    p->z = (N * (1 - e2) + h) * sin_lat;
    p->lldindex=lldidx;  // index into the whole data stream
}

// 1. Define your regional settings and scaling factor at the top of your file
const double LAT_ORIGIN = 31.0;       // Center latitude of your local data grid
const double LON_ORIGIN = -126.0;     // Center longitude of your local data grid
const double DEPTH_SCALE = 100.0;     // <-- SCALE FACTOR: Makes 1 meter of depth mismatch
void lld_to_xyz(KDVec3 *p, float lat, float lon, float depth, int lldidx)
{
    const double a = 6378137.0;            // WGS84 semi-major axis
    const double e2 = 6.69437999014e-3;    // eccentricity^2

    // 2. Compute the metric radius constants at your local origin
    double rad_lat = LAT_ORIGIN * M_PI / 180.0;
    double N = a / sqrt(1.0 - e2 * sin(rad_lat) * sin(rad_lat));

    // Determine exactly how many meters are in one degree of Lat/Lon at this location
    double meters_per_deg_lat = (M_PI / 180.0) * N * (1.0 - e2);
    double meters_per_deg_lon = (M_PI / 180.0) * N * cos(rad_lat);

    // 3. Project Lat/Lon to local horizontal meters (X = Easting, Y = Northing)
    p->x = (lon - LON_ORIGIN) * meters_per_deg_lon;
    p->y = (lat - LAT_ORIGIN) * meters_per_deg_lat;

    // 4. ARTIFICIALLY SCALE THE DEPTH AXIS HERE
    // Now that Z is strictly depth, the KD-tree can penalize it perfectly.
    p->z = (double)depth * DEPTH_SCALE;

    p->lldindex = lldidx;  // index into the whole data stream
}


int cmp_x(const void *a, const void *b)
{
    const KDVec3 *pa = (const KDVec3 *)a;
    const KDVec3 *pb = (const KDVec3 *)b;

    if (pa->x < pb->x) return -1;
    if (pa->x > pb->x) return 1;
    return 0;
}

int cmp_y(const void *a, const void *b)
{
    const KDVec3 *pa = (const KDVec3 *)a;
    const KDVec3 *pb = (const KDVec3 *)b;

    if (pa->y < pb->y) return -1;
    if (pa->y > pb->y) return 1;
    return 0;
}

int cmp_z(const void *a, const void *b)
{
    const KDVec3 *pa = (const KDVec3 *)a;
    const KDVec3 *pb = (const KDVec3 *)b;

    if (pa->z < pb->z) return -1;
    if (pa->z > pb->z) return 1;
    return 0;
}

// n=number of points
// depth start at 0
KDNode* build_kdtree(KDVec3 *pts, int n, int depth)
{
    if (n <= 0) return NULL;

    int axis = depth % 3;
    if (axis == 0) qsort(pts, n, sizeof(KDVec3), cmp_x);
    if (axis == 1) qsort(pts, n, sizeof(KDVec3), cmp_y);
    if (axis == 2) qsort(pts, n, sizeof(KDVec3), cmp_z);

    int mid = n / 2;

    KDNode *node = malloc(sizeof(KDNode));
    node->point = pts[mid];
    node->axis = axis;

    node->left  = build_kdtree(pts, mid, depth+1);
    node->right = build_kdtree(pts+mid+1, n-mid-1, depth+1);

    return node;
}

void free_kdtree(KDNode *node) {
    if(node->left) free_kdtree(node->left);
    if(node->right) free_kdtree(node->right);
    free(node);
}

int flatten_kdtree(KDNode *node, KDNodeDisk *out, int *pos) {
    if (!node) return -1;

    int my_id = (*pos)++;

    out[my_id].x = node->point.x;
    out[my_id].y = node->point.y;
    out[my_id].z = node->point.z;
    out[my_id].axis = node->axis;
    out[my_id].lldindex = node->point.lldindex;

    out[my_id].left  = flatten_kdtree(node->left,  out, pos);
    out[my_id].right = flatten_kdtree(node->right, out, pos);

    return my_id;
}

void write_flatten_kdtree(const char *fname, KDNodeDisk *nodes, int n) {

    FILE *f = fopen(fname, "wb");
    if (!f) return;

    fwrite(&n, sizeof(int), 1, f);
    fwrite(nodes, sizeof(KDNodeDisk), n, f);

    fclose(f);
}

// read back kdtree from binary file
KDNodeDisk *read_flatten_kdtree(const char *fname, int n) {

    FILE *fp = fopen(fname, "rb");
    if (!fp) return NULL;

    KDNodeDisk *dnodes = malloc(n * sizeof(KDNodeDisk));
    fread(dnodes, sizeof(KDNodeDisk), n, fp);
    fclose(fp);

    return dnodes;
}

// XXX  rebuild structure ??


// nearest code
float dist_sq(KDVec3* a, KDVec3* b) {
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    float dz = a->z - b->z;
    return (dx*dx + dy*dy + dz*dz);
}

//t3D query = lld_to_xyz(q_lat, q_lon, q_depth, -1);
//Point3D best;
//float best_dist = DBL_MAX;
//kdtree_nearest(root, query, &best, &best_dist);
//printf("Nearest index: %d\n", best.index);
void kdtree_nearest(KDNode *root, KDVec3 *query, KDVec3 **best, float *best_dist) {
    if (!root) return;

    float d = dist_sq(&(root->point), query);

    if (d < *best_dist) {
        *best_dist = d;
        *best = &(root->point);
        if(muscal2_ucvm_debug_detail) {
         fprintf(stderrfp,"In kdtree_nearest.. reset %lf with (%d)\n", d, (*best)->lldindex);
        }
    }

    int axis = root->axis;
    float diff;

    if (axis == 0) diff = query->x - root->point.x;
    else if (axis == 1) diff = query->y - root->point.y;
    else diff = query->z - root->point.z;

    KDNode *first  = diff < 0 ? root->left  : root->right;
    KDNode *second = diff < 0 ? root->right : root->left;

    kdtree_nearest(first, query, best, best_dist);

    if (diff*diff < *best_dist) {
        kdtree_nearest(second, query, best, best_dist);
    }
}

void kdtree_nearest_full(KDlld *pnts,KDNode *root, KDVec3 *query, KDVec3 **best, float *best_dist) {
    if (!root) return;

    float d = dist_sq(&(root->point), query);

    if (d < *best_dist) {
        *best_dist = d;
        *best = &(root->point);
        if(muscal2_ucvm_debug_detail) {
         int idx= (*best)->lldindex;
         fprintf(stderrfp,"In kdtree_nearest_full -> reset distance(%lf) with idx(%d)\n", d, (*best)->lldindex);
         fprintf(stderrfp,"  ===   %lf %lf %lf \n", (*best)->x, (*best)->y, (*best)->z);
         fprintf(stderrfp,"  >>>   %lf %lf %lf \n", pnts[idx].lon, pnts[idx].lat, pnts[idx].depth);
        }
    }

    int axis = root->axis;
    float diff;

    if (axis == 0) diff = query->x - root->point.x;
    else if (axis == 1) diff = query->y - root->point.y;
    else diff = query->z - root->point.z;

    KDNode *first  = diff < 0 ? root->left  : root->right;
    KDNode *second = diff < 0 ? root->right : root->left;

    kdtree_nearest_full(pnts,first, query, best, best_dist);

    if (diff*diff < *best_dist) {
        kdtree_nearest_full(pnts,second, query, best, best_dist);
    }
}


/****** for searching N nearest neighbors ******/
// Helper function to maintain a bounded sorted list of the top N closest points
void knn_insert(KDNodeResult *results, int N, float dist, KDVec3 *point) {
    // If the new point is further away than the worst candidate in our list, ignore it
    if (dist >= results[N-1].dist) return;

    // Use an insertion-sort strategy to place the point in its correct spot
    int i = N - 1;
    while (i > 0 && results[i-1].dist > dist) {
        results[i] = results[i-1]; // Shift further elements to the right
        i--;
    }
    results[i].dist = dist;
    results[i].point = point;

    // Mirrors your original debug statement
    if (muscal2_ucvm_debug) {
        fprintf(stderrfp, "In kdtree_n_nearest.. added distance %lf with (%d) at slot %d\n", 
                (double)dist, point->lldindex, i);
    }
}

// Recursive N-nearest neighbor search function
void kdtree_n_nearest_recursive(KDNode *node, KDVec3 *query, KDNodeResult *results, int N) {
    if (!node) return;

    // 1. Calculate the squared distance to the current node's point
    float d = dist_sq(&(node->point), query);

    // 2. Try to insert this node into the tracking list
    knn_insert(results, N, d, &(node->point));

    // 3. Determine the splitting axis and coordinate distance
    int axis = node->axis;
    float diff;

    if (axis == 0)      diff = query->x - node->point.x;
    else if (axis == 1) diff = query->y - node->point.y;
    else                diff = query->z - node->point.z;

    // 4. Decide which subtree to traverse first based on the splitting plane
    KDNode *first  = diff < 0 ? node->left  : node->right;
    KDNode *second = diff < 0 ? node->right : node->left;

    // Always explore the closer partition first
    kdtree_n_nearest_recursive(first, query, results, N);

    // 5. Pruning check: Look at the current furthest neighbor in our top N.
    // If the hypersphere overlaps the splitting plane, we must check the other partition.
    // (If the list isn't full yet, worst_dist is FLT_MAX, ensuring we check both sides)
    float worst_dist = results[N-1].dist;
    if (diff * diff < worst_dist) {
        kdtree_n_nearest_recursive(second, query, results, N);
    }
}

/**
 * User-facing wrapper function to find N nearest neighbors.
 * * @param root        The root node of your KD-tree
 * @param query       The target 3D coordinate array
 * @param N           The number of nearest neighbors requested
 * @param best_nodes  An allocated array of size N to receive the KDVec3 pointers
 * @param best_dists  An allocated array of size N to receive the calculated float squared distances
 */
void kdtree_n_nearest(KDNode *root, KDVec3 *query, int N, KDVec3 **best_nodes, float *best_dists) {
    if (N <= 0 || !root || !best_nodes || !best_dists) return;

    // Allocate an array of results structures to coordinate the sorting process
    KDNodeResult *results = (KDNodeResult *)malloc(N * sizeof(KDNodeResult));
    for (int i = 0; i < N; i++) {
        results[i].dist = FLT_MAX; // Seed with maximum float value
        results[i].point = NULL;
    }

    // Run the recursive traversal
    kdtree_n_nearest_recursive(root, query, results, N);

    // Unpack the sorted tracking structure back into the user's pointer arrays
    for (int i = 0; i < N; i++) {
        best_nodes[i] = results[i].point;
        best_dists[i] = results[i].dist;
    }

    free(results);
}

