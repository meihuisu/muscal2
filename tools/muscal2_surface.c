/*
 * @file muscal2_surface.c
 * @brief Extract surface material properities from MUSCAL2 dataset in a fix step
 * @author - SCEC
 * @version 1.0
 *
 * Usage: ./muscal2_surface [-i layerlist] 
 *
 * output format:
 * -120.557570 35.100350 0.000000 1410.000000 3170.000000
 *
 */

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "ucvm_model_dtypes.h"
#include "muscal2_util.h"
#include "cJSON.h"
#include "muscal2.h"

int muscal2_debug=0;

void extract_surface(float dep);

int _compare_double(double f1, double f2) {
  double precision = 0.00001;
  if (((f1 - precision) < f2) && ((f1 + precision) > f2)) {
    return 1;
    } else {
      return 0;
  }
}

/* Usage function */
void usage() {
  printf("     muscal2_surface - (c) SCEC\n");
  printf("Extract surface material properties from a MUSCAL2 model\n");
  printf("\tusage: muscal2_surface [-d][-h] [-i file.in]\n\n");
  printf("Flags:\n");
  printf("\t-d enable debug/verbose mode\n");
  printf("\t-i layer list <optional>\n");
  printf("\t-h usage\n\n");
  printf("Input format is: \n");
  printf("\t depth-in-meter\n\n");
  printf("Output format is:\n");
  printf("\t vp vs rho\n\n");
  exit (0);
}

extern char *optarg;
extern int optind, opterr, optopt;

/**
 * Initializes and MUSCAL2 in standalone mode with ucvm plugin 
 * api.
 *
 * @param argc The number of arguments.
 * @param argv The argument strings.
 * @return A zero value indicating success.
 */

int MAX_LAYER=210;

int main(int argc, char* const argv[]) {
    int rc;
    int opt;
    int dep_flag=0;
    FILE *dfp=NULL;
    

    /* Parse options */
    while ((opt = getopt(argc, argv, "dhi:")) != -1) {
      switch (opt) {
      case 'i':
        dep_flag=1;
        dfp=fopen(optarg,"r");
        break;
      case 'd':
        muscal2_debug=1;
        break;
      case 'h':
        usage();
        exit(0);
        break;
      default: /* '?' */
        usage();
        exit(1);
      }
    }

    if(muscal2_debug) { muscal2_setdebug(); }

    // Initialize the model. 
    // try to use Use UCVM_INSTALL_PATH
    char *envstr=getenv("UCVM_INSTALL_PATH");
    if(envstr != NULL) {
      assert(muscal2_init(envstr, "muscal2") == 0);
      } else {
        assert(muscal2_init("..", "muscal2") == 0);
    }
    if(muscal2_debug) { fprintf(stderrfp,"Loaded the model successfully.\n"); }

    char line[1001];
    int depth;
    int done=0;
    int idx=0;

    muscal2_dataset_t *dataset= muscal2_dataset->datasets[0];
    float *deplist=dataset->depths;

    while (!done) {
      if(!dep_flag) { // grab a value
        int cnt=dataset->nz;
        for(int i=0; i<cnt;  i++) {
          depth=deplist[i];
          if(muscal2_debug) { fprintf(stderrfp,"depths: %d\n",depth); }
          //extract_surface(depth);
          idx++;
        }
        done=1;
        } else { 
          if(fgets(line, 1000, dfp) == NULL) { 
            done=1;
            } else {
              if(muscal2_debug) { fprintf(stderrfp,"LINE: %s\n",line); }
              if(line[0]!='#') { 
                 if (sscanf(line,"%d", &depth) == 1) {
                   // extract_surface(depth);
                   idx++;
                 }
              }
          }
      }
    }

    assert(muscal2_finalize() == 0);
    if(muscal2_debug) { fprintf(stderrfp ,"Model closed successfully.\n"); }

    if(dep_flag) fclose(dfp);

    return 0;
}


void extract_surface(float dep) {

    if(muscal2_debug) { fprintf(stderrfp, "calling : depth(%d)\n", dep); }
    int data_idx=0; // first and only one
    muscal2_dataset_t *dataset= muscal2_dataset->datasets[data_idx];

    float *lon_list=dataset->longitudes;
    float *lat_list=dataset->latitudes;
    float *dep_list=dataset->depths;
    int nx=dataset->nx;
    int ny=dataset->ny;
    int nz=dataset->nz;

    int numpoints=nx*ny*nz;

    int target_dep_idx=find_buffer_idx_clamped(dep_list,nz,dep);

    // grab a layer from cache or try to load it from external data file
    muscal2_cache_layer_t *layer=find_a_cache_layer(dataset, target_dep_idx);

if(muscal2_debug){ fprintf(stderrfp,">> Using External data/Layer cache - current count=%d\n", dataset->layer_cache_cnt); }
    float *tmp_vp_buffer=layer->layer_vp_buffer;
    float *tmp_vs_buffer=layer->layer_vs_buffer;
    float *tmp_rho_buffer=layer->layer_rho_buffer;

// offset= (dep_idx)*(lat_cnt * lon_cnt)+(lat_idx)*(lon_cnt)+lon_idx
// numpoints
    int surface=0;
    int tmp;
    // iterate through lat and then lon 
    for(int i=0; i < ny; i++) { // outside always count as nan
       for(int j=0; j < nx; j++)  {
          surface=0;
          int offset = (nx * i) + j;     // x,y
          float vp = tmp_vp_buffer[offset];   
          float vs = tmp_vs_buffer[offset];
          float rho = tmp_rho_buffer[offset];

          if(isnan(vp) || isnan(vs)) continue;

          if(j-1 < 0 && j+1 > nx) { surface=1; }
          if(i-1 < 0 && i+1 > ny) { surface=1; }

   	  tmp = (nx * i) + (j-1); // x-1,y
          if(isnan(tmp_vp_buffer[tmp]) || isnan(tmp_vp_buffer[tmp])) { surface=1;}
	  tmp = (nx * i) + (j+1); // x+1,y
          if(isnan(tmp_vp_buffer[tmp]) || isnan(tmp_vp_buffer[tmp])) { surface=1;}
	  tmp = (nx * (i-1)) + j; // x,y-1
          if(isnan(tmp_vp_buffer[tmp]) || isnan(tmp_vp_buffer[tmp])) { surface=1;}
	  tmp = (nx * (i+1)) + j; // x,y+1
          if(isnan(tmp_vp_buffer[tmp]) || isnan(tmp_vp_buffer[tmp])) { surface=1;}

          if(surface) { // found surface..
            fprintf(stderr,"%lf %lf %lf %lf %lf %lf\n", lon_list[j], lat_list[i], dep, vp, vs, rho);
          }
       }
    }
}
