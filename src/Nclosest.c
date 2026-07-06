#include <float.h>
#include <stdio.h>
#include <stdlib.h>

//  top N closet point

// Structure to cleanly group a found point and its squared distance
typedef struct {
    float dist;
    KDVec3 *point;
} KDResult;

// Helper function to maintain a bounded sorted list of the top N closest points
void knn_insert(KDResult *results, int N, float dist, KDVec3 *point) {
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
void kdtree_n_nearest_recursive(KDNode *node, KDVec3 *query, KDResult *results, int N) {
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
    KDResult *results = (KDResult *)malloc(N * sizeof(KDResult));
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

