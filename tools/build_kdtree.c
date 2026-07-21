/** 
  
  build_kdtree.c
  -- turn a list of lat-lon-z into a kdtree, 
     flatten the kdtree, 
     and write the flatten kdtree into a binary file

  Usage: ./build_kdtree num-of-points file-of-surface-latlonz flatten-file

NOTE:
    ./build_kdtree 4 surface.in surface.bin

**/

#include <stdio.h>
#include <string.h>
#include "kdtree_util.h"

/*
    build_kdtree num-of-points file-of-surface-latlonz flatten-file
    ./build_kdtree 4 surface.in surface.out
*/ 

void usage() {
  printf("Usage: build_kdtree num_points datafile flatkdtreefile\n\n");
  exit(0);
}

#define KD_MAX_LINE 1000

int muscal2_ucvm_debug=1;
int muscal2_ucvm_debug_detail=0;
FILE *stderrfp=NULL;

int main(int argc, char **argv)
{
  char datafile[KD_MAX_LINE];
  char flatfile[KD_MAX_LINE];
  char line[KD_MAX_LINE];
  int maxnum;
  FILE *fp;
  double lat, lon, depth, vs, vp, density;
  KDlld *pnts;
  KDVec3 *vpnts;
  KDNode *nodes; // tree nodes
  KDNodeDisk *dnodes; // tree nodes for disk

  if(argc != 4) { usage(); }
  if(sscanf(argv[1], "%d", &maxnum) != 1) { return 1; }
  strcpy(datafile, argv[2]);
  strcpy(flatfile, argv[3]);

  if(muscal2_ucvm_debug) {
      stderrfp = fopen("query_debug.log", "w+");
      fprintf(stderrfp,"\n===== START build_kdtree ===== \n\n");
  }

  pnts = malloc(maxnum * sizeof(KDlld));
  vpnts = malloc(maxnum * sizeof(KDVec3));
  dnodes = malloc(maxnum * sizeof(KDNodeDisk));

  fp=fopen(datafile,"r");
  int numread=0;
// read all the points
  while (numread != maxnum && fgets(line, KD_MAX_LINE, fp) != NULL ) {
      if(line[0]=='#') continue;  // a comment line
				  //
      if (sscanf(line,"%lf %lf %lf %lf %lf %lf", &lon, &lat, &depth, &vs, &vp, &density) == 6) {
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

  int pos = 0;
  int flatten_top= flatten_kdtree(nodes, dnodes, &pos);

  if(muscal2_ucvm_debug) { fprintf(stderrfp,"flattend -- %d surface points\n",pos); }

  write_flatten_kdtree(flatfile, dnodes, numread);

  // free all malloc
  free_kdtree(nodes);
  free(pnts);
  free(vpnts);
  if(muscal2_ucvm_debug) { fprintf(stderrfp,"\n..DONE..\n"); fclose(stderrfp); }
}
