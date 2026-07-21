/** 
  
  query_kdtree.c
  -- convert a set of lat-lon-z into a kdtree, 
     make nearest point query against the kdtree

  ./query_kdtree 945765 $UCVM_INSTALL_PATH/model/muscal2/data/muscal2/surface_945765.in in

  where in : lon lat depth

         -113.282 35.35 3000
         ...
**/

#include <stdio.h>
#include <string.h>
#include <float.h>
#include "kdtree_util.h"

/*
    query_kdtree num-of-points file-of-surface-latlonz query_point
    ./query_kdtree 4 surface.in in
*/ 

void usage() {
  printf("Usage: query_kdtree num_points datafile query_list\n\n");
  exit(0);
}

#define KD_MAX_LINE 1000
int muscal2_ucvm_debug=0;
int muscal2_ucvm_debug_detail=0;
FILE *stderrfp=NULL;

int main(int argc, char **argv)
{
  char datafile[KD_MAX_LINE];
  char queryfile[KD_MAX_LINE];
  char line[KD_MAX_LINE];
  int maxnum;
  FILE *fp;
  double lat, lon, depth, vs, vp, density;
  KDlld *pnts;
  KDVec3 *vpnts;
  KDNode *nodes; // tree nodes

  if(argc != 4) { usage(); }
  if(sscanf(argv[1], "%d", &maxnum) != 1) { return 1; }
  strcpy(datafile, argv[2]);
  strcpy(queryfile, argv[3]);

  if(muscal2_ucvm_debug) {
      stderrfp = fopen("query_debug.log", "w+");
      fprintf(stderrfp,"\n===== START kdtree_query ===== \n\n");
  }

  pnts = malloc(maxnum * sizeof(KDlld));
  vpnts = malloc(maxnum * sizeof(KDVec3));

  fp=fopen(datafile,"r");
  int numread=0;
// read all the points
  while (numread != maxnum && fgets(line, KD_MAX_LINE, fp) != NULL ) {
      if(line[0]=='#') continue;  // a comment line
      if (sscanf(line,"%lf %lf %lf %lf %lf %lf", &lon, &lat, &depth, &vp, &vs, &density) == 6) {
         pnts[numread].lat=lat;
         pnts[numread].lon=lon;
         pnts[numread].depth=depth;
         pnts[numread].vs=vs;
         pnts[numread].vp=vp;
         pnts[numread].density=density;
	 lld_to_xyz(&vpnts[numread], lat, lon, depth, numread);   
         numread++;
      }
  }
  fclose(fp);
  nodes = build_kdtree(vpnts, numread, 0);
  if(muscal2_ucvm_debug) { fprintf(stderrfp,"kdtree with -- %d surface points\n",numread); }

  // QUERY against kdtree
  fp=fopen(queryfile,"r");
  int numquery=0;
  KDVec3 query;
/*
  while (fgets(line, KD_MAX_LINE, fp) != NULL ) {
      KDVec3 *best=NULL;
      float best_dist = FLT_MAX;
      if(line[0]=='#') continue;  // a comment line
      if(sscanf(line,"%lf %lf %lf", &lon, &lat, &depth) == 3) {
         if(muscal2_ucvm_debug) { fprintf(stderrfp,"  query point -> lat(%lf) lon(%lf) depth(%lf) \n", lat, lon, depth); }
	 lld_to_xyz(&query, lat, lon, depth, 0);
	 kdtree_nearest(nodes, &query, &best, &best_dist);
         int best_idx=best->lldindex;
         if(muscal2_ucvm_debug) { fprintf(stderrfp,"FOUND: %d(%lf):   %lf %lf %lf\n\n", best_idx, best_dist, pnts[best_idx].lon, pnts[best_idx].lat, pnts[best_idx].depth); }

         numquery++;
      }
  }
*/
  while (fgets(line, KD_MAX_LINE, fp) != NULL ) {

      KDVec3 *best;
      float best_dist=FLT_MAX;

      if(line[0]=='#') continue;  // a comment line
      if(sscanf(line,"%lf %lf %lf", &lon, &lat, &depth) == 3) {
         if(muscal2_ucvm_debug) { fprintf(stderrfp,"  query point -> lat(%lf) lon(%lf) depth(%lf) \n", lat, lon, depth); }
	 lld_to_xyz(&query, lat, lon, depth, 0);
         if(muscal2_ucvm_debug_detail) {
	   kdtree_nearest_full(pnts,nodes, &query, &best, &best_dist);
           } else {
	     kdtree_nearest(nodes, &query, &best, &best_dist);
         }
         int best_idx=best->lldindex;
         if(muscal2_ucvm_debug) { 
fprintf(stderrfp,"FOUND: %d(%lf):   %lf %lf %lf\n", best_idx, best_dist, pnts[best_idx].lon, pnts[best_idx].lat, pnts[best_idx].depth);
fprintf(stderrfp,"           vs%lf vp%lf rho%lf\n\n", pnts[best_idx].vs, pnts[best_idx].vp, pnts[best_idx].density);
 }

         numquery++;
      }
  }

  if(muscal2_ucvm_debug) { fprintf(stderrfp,"total for %d locations\n", numquery); }

  // remember to free all malloc
  //free nodes 
  free_kdtree(nodes);
  free(pnts);
  free(vpnts);
  if(muscal2_ucvm_debug) { fprintf(stderrfp,"\n..DONE..\n"); fclose(stderrfp); }
}
