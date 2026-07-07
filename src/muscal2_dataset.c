/**
         muscal2_util.c
**/

#include <string.h>

#include "ucvm_model_dtypes.h"
#include "muscal2.h"
#include "um_netcdf.h"

#include "muscal2_util.h"

/*** for surface data ***/
int free_kdnodesetup(KDNodeSetup *sptr) {
  free(sptr->pnts);
  free(sptr->vpnts);
  free_kdtree(sptr->nodes);
  free(sptr); 
  return SUCCESS;
}

void add_surface_data(muscal2_dataset_t  *data, char  *filepath, int sz) {

  double lat, lon, depth, vs, vp, density;
  char line[KD_MAX_LINE];

  KDNodeSetup *kdsurface = (KDNodeSetup *) malloc(1 * sizeof(KDNodeSetup)); ;

  kdsurface->pnts = malloc(sz * sizeof(KDlld));
  kdsurface->vpnts = malloc(sz * sizeof(KDVec3));

  FILE *fp=fopen(filepath,"r");

  int numread=0;
// read all the points
  while (numread != sz && fgets(line, KD_MAX_LINE, fp) != NULL ) {
      if(line[0]=='#') continue;  // a comment line
      if (sscanf(line,"%lf %lf %lf %lf %lf %lf", &lon, &lat, &depth, &vp, &vs, &density) == 6) {
         kdsurface->pnts[numread].lat=lat;
         kdsurface->pnts[numread].lon=lon;
         kdsurface->pnts[numread].depth=depth;
         kdsurface->pnts[numread].vs=vs;
         kdsurface->pnts[numread].vp=vp;
         kdsurface->pnts[numread].density=density;
         lld_to_xyz(&kdsurface->vpnts[numread], lat, lon, depth, numread);
         numread++;
      }
  }
  fclose(fp);
  kdsurface->nodes = build_kdtree(kdsurface->vpnts, numread, 0);
  data->kdsurface=kdsurface;
}

/**** for muscal2_dataset_t ****/
muscal2_dataset_t *make_a_muscal2_dataset(muscal2_configuration_t *config, char *datadir, char *datafile) {
    char filepath[256];
    size_t nelems= 0;
    nc_type vtype;

    muscal2_dataset_t *dataset=(muscal2_dataset_t *)malloc(sizeof(muscal2_dataset_t));

    sprintf(filepath, "%s/%s", datadir, datafile);
    if(muscal2_ucvm_debug) fprintf(stderrfp," data file ..%s\n", filepath);

/* setup ctx */
    int rc=tiledb_ctx_alloc(NULL, dataset->tiledb_ctx);
    int sz=strlen(datadir)+strlen(datafile)+2;
     
    dataset->tiledb_array_uri= (char  *) malloc(sizeof(char) * sz);
    strcpy(dataset->tiledb_array_uri,"%s/%s",datadir,datafile);

    int nx=config->nx;
    int ny=config->ny;
    int nz=config->nz;

/* setup nx/ny/nz and void ptrs */
    data->longitudes=(float *) malloc(nx * sizeof(float));
XXX
    if(muscal2_ucvm_debug_detail) {
        fprintf(stderrfp, "  Longitudes: %d\n", nx);
        for(int i=0;i<nx; i++) {
            fprintf(stderrfp, "%d  %f\n", i, data->longitudes[i]);
       	}
    }

    data->latitudes=(float *) malloc(ny * sizeof(float));
XXX
    if(muscal2_ucvm_debug_detail) {
        fprintf(stderrfp, "  Latitude: %d\n", ny);
        for(int i=0;i<ny; i++) {
            fprintf(stderrfp, "%d  %f\n", i, data->latitudes[i]);
       	}
    }

    data->depths=(float *) malloc (nz * sizeof(float));
XXX
    if(muscal2_ucvm_debug_detail) {
        fprintf(stderrfp, "  Depths: %d\n", nz);
        for(int i=0;i<nz; i++) {
            fprintf(stderrfp, "%d  %f\n", i, data->depths[i]);
       	}
    }

    data->kdsurface=NULL;
    return data;
}



int free_muscal2_dataset(muscal2_dataset_t *data) {
    free(data->depths);
    free(data->latitudes);
    free(data->longitudes);

    if(data->kdsurface!=NULL) {
      free_kdnodesetup(data->kdsurface);
    }

    free(data);
    return SUCCESS;
}

/**** straight or trilinear/bilinear ****/
int _buffer_offset(muscal2_dataset_t * dataset, int x_idx, int  y_idx, int z_idx) {
    int nx=dataset->nx;
    int ny=dataset->ny;
    int nz=dataset->nz;

    int offset= (z_idx)*(ny * nx)+(y_idx)*(nx)+x_idx;
    if(muscal2_ucvm_debug) { fprintf(stderrfp,"\nTarget offset %d : idx lon/lat/dep = %d/%d/%d\n", offset,x_idx, y_idx, z_idx); }

    return offset;
}

int get_one_property_binary(muscal2_dataset_t *dataset, muscal2_pt_info_t *pt, muscal2_properties_t *data) {
    int offset= _buffer_offset(dataset, pt->lon_idx, pt->lat_idx, pt->dep_idx);

    data->vp=dataset->vp_buffer[offset];
    data->vs=dataset->vs_buffer[offset];
    data->rho=dataset->rho_buffer[offset];
    return offset;
}

int get_one_muscal21d_property(muscal2_dataset_t *dataset, muscal2_pt_info_t *pt, muscal2_properties_t *data) {
    if(muscal2_ucvm_debug) { fprintf(stderrfp,"\ncalling get_one_muscal21d_property\n"); }

    KDVec3 *best=NULL;
    float best_dist=FLT_MAX;
    int best_idx=0;

    KDVec3 query;
    KDNodeSetup *kdsurface=dataset->kdsurface;
    KDlld *pnts=kdsurface->pnts;

    lld_to_xyz(&query, pt->lat, pt->lon, pt->dep, 0);
    kdtree_nearest(kdsurface->nodes, &query, &best, &best_dist);
    best_idx=best->lldindex;

    if(muscal2_ucvm_debug) { 
      fprintf(stderrfp,"FOUND: %d(%lf):   %lf %lf %lf\n\n", best_idx, best_dist, pnts[best_idx].lon, pnts[best_idx].lat, pnts[best_idx].depth);
    }

    data->vp=pnts[best_idx].vp;
    data->vs=pnts[best_idx].vs;
    data->rho=pnts[best_idx].density;
    return best_idx;
}


float _interp_a_point(muscal2_dataset_t *dataset, float *buffer, muscal2_pt_info_t *pt) {
    int lon_idx=pt->lon_idx;
    int lat_idx=pt->lat_idx;
    int dep_idx=pt->dep_idx;
    float lon_percent=pt->lon_percent;
    float lat_percent=pt->lat_percent;
    float dep_percent=pt->dep_percent;

    if(pt->lon_idx < 0 || pt->lat_idx < 0 || pt->dep_idx < 0 ||
         pt->lon_idx +1 >= dataset->nx || pt->lat_idx +1 >= dataset->ny || pt->dep_idx+1 >= dataset->nz ) {
        // out of bound
        return -1;	
    }

    float val0= buffer[_buffer_offset(dataset,lon_idx,lat_idx,dep_idx)];      // x,    y, z
    float val1= buffer[_buffer_offset(dataset,lon_idx+1,lat_idx,dep_idx)];    // x+1,  y, z 
    float val2= buffer[_buffer_offset(dataset,lon_idx,lat_idx+1,dep_idx)];    // x,  y+1, z 
    float val3= buffer[_buffer_offset(dataset,lon_idx+1,lat_idx+1,dep_idx)];  // x+1,y+1, z
    float val4= buffer[_buffer_offset(dataset,lon_idx,lat_idx,dep_idx+1)];    // x,    y, z+1
    float val5= buffer[_buffer_offset(dataset,lon_idx+1,lat_idx,dep_idx+1)];  // x+1,  y, z+1
    float val6= buffer[_buffer_offset(dataset,lon_idx,lat_idx+1,dep_idx+1)];  // x,  y+1, z+1
    float val7= buffer[_buffer_offset(dataset,lon_idx+1,lat_idx+1,dep_idx+1)];// x+1,y+1, z+1
        
    float val00= val0 * (1-lon_percent) + val1 * lon_percent;    
    float val11= val4 * (1-lon_percent) + val5 * lon_percent;    
    float val22= val2 * (1-lon_percent) + val3 * lon_percent;    
    float val33= val6 * (1-lon_percent) + val7 * lon_percent;    

    float val000 = val00 * (1-lat_percent) + val22 * lat_percent;
    float val111 = val11 * (1-lat_percent) + val33 * lat_percent;

    float val0000 = val000 * (1-dep_percent) + val111 * dep_percent;
    return val0000;
}

void get_interp_property_binary(muscal2_dataset_t *dataset, muscal2_pt_info_t *pt, muscal2_properties_t *data) {

    if(muscal2_ucvm_debug) { fprintf(stderrfp,"\nInterp PROCESSING for vp\n"); }
    data->vp = _interp_a_point(dataset, dataset->vp_buffer, pt);
    if(muscal2_ucvm_debug) { fprintf(stderrfp,"\nInterp PROCESSING for vs\n"); }
    data->vs = _interp_a_point(dataset, dataset->vs_buffer, pt);
    if(muscal2_ucvm_debug) { fprintf(stderrfp,"\nInterp PROCESSING for rho\n"); }
    data->rho = _interp_a_point(dataset, dataset->rho_buffer, pt);
    return;
}


/**** for muscal2_cache_col_t ****/
muscal2_cache_col_t *_add_a_cache_col(muscal2_dataset_t *dataset, int target_lat_idx, int target_lon_idx) {
 
   int nz=dataset->nz;

   muscal2_cache_col_t *col= (muscal2_cache_col_t *) malloc(sizeof(muscal2_cache_col_t));

   col->cache_col_lat_idx=target_lat_idx;
   col->cache_col_lon_idx=target_lon_idx;

   // alloc space first 
   col->col_vp_buffer=(float *) malloc(nz * sizeof(float));
   col->col_vs_buffer=(float *) malloc(nz * sizeof(float));
   col->col_rho_buffer=(float *) malloc(nz * sizeof(float));

   cache_depth_col_float(dataset->ncid, dataset->vp_varid,
                        nz, target_lat_idx, target_lon_idx, col->col_vp_buffer);
   cache_depth_col_float(dataset->ncid, dataset->vs_varid,
                        nz, target_lat_idx, target_lon_idx, col->col_vs_buffer);
   cache_depth_col_float(dataset->ncid, dataset->rho_varid,
                        nz, target_lat_idx, target_lon_idx, col->col_rho_buffer);
   return col;
}

muscal2_cache_col_t *find_a_cache_col(muscal2_dataset_t *dataset, int target_lat_idx, int target_lon_idx) {

   int cnt= dataset->col_cache_cnt; 
   muscal2_cache_col_t *col;

   for(int i=0; i< cnt; i++) {
     col=dataset->col_cache[i];
     if((col->cache_col_lat_idx == target_lat_idx) &&
		      (col->cache_col_lon_idx == target_lon_idx) ) {
        // found it
        return col;
     }
   }
   // load it from the netcdf file
   col=_add_a_cache_col(dataset, target_lat_idx, target_lon_idx);
    
   // find a space to put in (in case it is full)
   if( cnt < MUSCAL2_CACHE_COL_MAX) {
       dataset->col_cache[cnt]=col;
       dataset->col_cache_cnt=cnt+1;
       } else {
// else has to free one out
         int use_idx=(cnt+1) % MUSCAL2_CACHE_COL_MAX;
         free_a_cache_col(dataset->col_cache[use_idx]);
         dataset->col_cache[use_idx]=col;

   }

   return col;     
}

void free_a_cache_col(muscal2_cache_col_t *col) {

   // free buffer first 
   free(col->col_vp_buffer);
   free(col->col_vs_buffer);
   free(col->col_rho_buffer);

   free(col);
}

/**** for muscal2_cache_layer_t ****/
muscal2_cache_layer_t *_add_a_cache_layer(muscal2_dataset_t *dataset, int target_dep_idx) {

    int nx=dataset->nx;
    int ny=dataset->ny;

    if(muscal2_ucvm_debug) { fprintf(stderrfp, "  Loading a new layer: %zu\n", target_dep_idx); }
    muscal2_cache_layer_t *layer= (muscal2_cache_layer_t *) malloc(sizeof(muscal2_cache_layer_t));

    layer->cache_layer_dep_idx=target_dep_idx;

    layer->layer_vp_buffer = (float * )malloc((nx*ny) * sizeof(float));
    layer->layer_vs_buffer = (float * )malloc((nx*ny) * sizeof(float));
    layer->layer_rho_buffer = (float * )malloc((nx*ny) * sizeof(float));

    cache_latlon_layer_float(dataset->ncid, dataset->vp_varid,
                      target_dep_idx, ny, nx, layer->layer_vp_buffer);
    cache_latlon_layer_float(dataset->ncid, dataset->vs_varid,
                      target_dep_idx, ny, nx, layer->layer_vs_buffer);
    cache_latlon_layer_float(dataset->ncid, dataset->rho_varid,
                      target_dep_idx, ny, nx, layer->layer_rho_buffer);
    return layer;
}

muscal2_cache_layer_t *find_a_cache_layer(muscal2_dataset_t *dataset, int target_dep_idx) {

   int cnt= dataset->layer_cache_cnt;
   muscal2_cache_layer_t *layer;

   for(int i=0; i< cnt; i++) {
     layer=dataset->layer_cache[i];
     if((layer->cache_layer_dep_idx == target_dep_idx) ) {
        // found it
        return layer;
     }
   }
   // load it from the netcdf file
   layer=_add_a_cache_layer(dataset, target_dep_idx);

   // find a space to put in (in case it is full)
   if( cnt < MUSCAL2_CACHE_LAYER_MAX) {
       dataset->layer_cache[cnt]=layer;
       dataset->layer_cache_cnt=cnt+1;
       } else {
// else has to free one out
         int use_idx=(cnt+1) % MUSCAL2_CACHE_LAYER_MAX;
         free_a_cache_layer(dataset->layer_cache[use_idx]);
         dataset->layer_cache[use_idx]=layer;
   }
   return layer;     
}

void free_a_cache_layer(muscal2_cache_layer_t *layer) {

   free(layer->layer_vp_buffer);
   free(layer->layer_vs_buffer);
   free(layer->layer_rho_buffer);

   free(layer);
}
