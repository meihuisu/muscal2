/**
         muscal2_util.c
**/

#include <string.h>

#include "ucvm_model_dtypes.h"
#include "muscal2.h"
#include "muscal2_util.h"
#include <tiledb/tiledb.h>

/*** for surface data ***/
int free_kdnodesetup(KDNodeSetup *sptr) {
  free(sptr->pnts);
  free(sptr->vpnts);
  free_kdtree(sptr->nodes);
  free(sptr); 
  return SUCCESS;
}

void add_surface_data(muscal2_dataset_t *model, char *filepath, int sz) {

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
  model->kdsurface=kdsurface;
}

/**** for muscal2_dataset_t ****/
muscal2_dataset_t * muscal2_read_dataset(char *datadir, char *datafile) {

    muscal2_dataset_t *model= (muscal2_dataset_t *) malloc(sizeof(muscal2_dataset_t)*1);

    int sz=strlen(datadir)+strlen(datafile)+2;
    model->tiledb_uri= (char  *) malloc(sizeof(char) * sz);
    snprintf(model->tiledb_uri,sz,"%s/%s",datadir,datafile);
    if(muscal2_ucvm_debug) fprintf(stderrfp," data file ..%s\n", model->tiledb_uri);

/* setup ctx */
    int rc=tiledb_ctx_alloc(NULL, &model->tiledb_ctx);
    tiledb_array_alloc(model->tiledb_ctx, model->tiledb_uri, &model->tiledb_array);
    if (tiledb_array_open(model->tiledb_ctx, model->tiledb_array, TILEDB_READ) != TILEDB_OK) {
        fprintf(stderr, "[ERROR] Unable to open TileDB array '%s' for reading.\n", model->tiledb_uri);
        if (model->tiledb_array) tiledb_array_free(&model->tiledb_array);
        if (model->tiledb_ctx) tiledb_ctx_free(&model->tiledb_ctx);
        return NULL;
    }
    if(muscal2_ucvm_debug) {
      fprintf(stderrfp, "Successfully opened array '%s' in read mode.\n", model->tiledb_uri);
    }

    // extract metadata from tiledb_uri
    tiledb_datatype_t value_type;
    uint32_t value_num;
    const void* value_ptr;

    // Fetch a direct internal pointer to the metadata values
    tiledb_array_get_metadata(model->tiledb_ctx, model->tiledb_array, "coords_depth", &value_type, &value_num, &value_ptr);
    if (value_ptr != NULL) {
        model->nz = value_num;
        model->depths = (float*)malloc(model->nz * sizeof(float));
        // Copy the raw bytes securely into our typed array structure
        memcpy(model->depths, value_ptr, model->nz * sizeof(float));
        
        if(muscal2_ucvm_debug) {
          fprintf(stderrfp,"--- DEPTH METADATA (Total Elements Found: %u) ---\n", value_num);
          fprintf(stderrfp,"First 5 values: ");
          for (int i = 0; i < (model->nz < 5 ? model->nz : 5); i++) {
             fprintf(stderrfp,"%f ", model->depths[i]);
          }
          fprintf(stderrfp,"\nLast value: %f\n\n", model->depths[model->nz - 1]);
        }
    } else {
        fprintf(stderr,"[ERROR] Metadata key 'coords_depth' was not found or is empty.\n\n");
    }

    tiledb_array_get_metadata(model->tiledb_ctx, model->tiledb_array, "coords_latitude", &value_type, &value_num, &value_ptr);
    if (value_ptr != NULL) {
        model->ny= value_num;
        model->latitudes = (float*)malloc(model->ny * sizeof(float));
        memcpy(model->latitudes, value_ptr, model->ny * sizeof(float));
        
        if(muscal2_ucvm_debug) {
          fprintf(stderrfp,"--- LATITUDE METADATA (Total Elements Found: %u) ---\n", value_num);
          fprintf(stderrfp,"First 5 values: ");
          for (int i = 0; i < (model->ny < 5 ? model->ny : 5); i++) {
            fprintf(stderrfp,"%f ", model->latitudes[i]);
          }
          fprintf(stderrfp,"\nLast value: %f\n\n", model->latitudes[model->ny - 1]);
        }
    } else {
        fprintf(stderr,"[ERROR] Metadata key 'coords_latitude' was not found or is empty.\n\n");
    }

    tiledb_array_get_metadata(model->tiledb_ctx, model->tiledb_array, "coords_longitude", &value_type, &value_num, &value_ptr);
    if (value_ptr != NULL) {
        model->nx = value_num;
        model->longitudes = (float*)malloc(model->nx * sizeof(float));
        memcpy(model->longitudes, value_ptr, model->nx * sizeof(float));
        
        if(muscal2_ucvm_debug) {
          fprintf(stderrfp,"--- LONGITUDE METADATA (Total Elements Found: %u) ---\n", value_num);
          fprintf(stderrfp,"First 5 values: ");
          for (int i = 0; i < (model->nx < 5 ? model->nx : 5); i++) {
              fprintf(stderrfp,"%f ", model->longitudes[i]);
          }
          fprintf(stderrfp,"\nLast value: %f\n\n", model->longitudes[model->nx - 1]);
        }
    } else {
        fprintf(stderr,"[ERROR] Metadata key 'coords_longitude' was not found or is empty.\n\n");
    }

    model->kdsurface=NULL;
    return model;
}

void muscal2_read_surface(muscal2_dataset_t *model, int count, char *datadir, char *surface_file) {
    char filepath[1024];
    sprintf(filepath, "%s/%s", datadir, surface_file);
    if(muscal2_ucvm_debug) fprintf(stderrfp," data file ..%s\n", filepath);
    add_surface_data(model, filepath, count);
}

// pretty print the metadata of the dataset
void dump_dataset_metadata(muscal2_dataset_t *model) {
    int nx=model->nx;
    int ny=model->ny;
    int nz=model->nz;

    if(muscal2_ucvm_debug_detail) {
        fprintf(stderrfp, "  Longitudes: %d\n", nx);
        for(int i=0;i<nx; i++) {
            fprintf(stderrfp, "%d  %f\n", i, model->longitudes[i]);
       	}
    }

    if(muscal2_ucvm_debug_detail) {
        fprintf(stderrfp, "  Latitude: %d\n", ny);
        for(int i=0;i<ny; i++) {
            fprintf(stderrfp, "%d  %f\n", i, model->latitudes[i]);
       	}
    }

    if(muscal2_ucvm_debug_detail) {
        fprintf(stderrfp, "  Depths: %d\n", nz);
        for(int i=0;i<nz; i++) {
            fprintf(stderrfp, "%d  %f\n", i, model->depths[i]);
       	}
    }
}

int free_muscal2_dataset(muscal2_dataset_t *model) {
    free(model->depths);
    free(model->latitudes);
    free(model->longitudes);

    if(model->kdsurface!=NULL) {
      free_kdnodesetup(model->kdsurface);
    }

    if (model->tiledb_array) {
        tiledb_array_close(model->tiledb_ctx, model->tiledb_array);
        tiledb_array_free(&model->tiledb_array);
    }
    if (model->tiledb_ctx) tiledb_ctx_free(&model->tiledb_ctx);

    free(model);
    return SUCCESS;
}

/**** straight or trilinear/bilinear or muscal2_1d ****/
int get_one_property(muscal2_dataset_t *model, muscal2_pt_info_t *pt, muscal2_properties_t *data) {
    tiledb_query_t* query = NULL;
    tiledb_subarray_t* subarray = NULL;

    float real_depth = model->depths[pt->dep_idx];
    float real_lat   = model->latitudes[pt->lat_idx];
    float real_lon   = model->longitudes[pt->lon_idx];

    float val_vp = 0.0f;
    float val_vs = 0.0f;
    float val_rho = 0.0f;
    uint64_t bytes_vp = sizeof(float);
    uint64_t bytes_vs = sizeof(float);
    uint64_t bytes_rho = sizeof(float);

    tiledb_query_alloc(model->tiledb_ctx, model->tiledb_array, TILEDB_READ, &query);
    tiledb_query_set_layout(model->tiledb_ctx, query, TILEDB_ROW_MAJOR);
    tiledb_subarray_alloc(model->tiledb_ctx, model->tiledb_array, &subarray);

    int32_t depth_range[] = {pt->dep_idx, pt->dep_idx};
    int32_t lat_range[]   = {pt->lat_idx, pt->lat_idx};
    int32_t lon_range[]   = {pt->lon_idx, pt->lon_idx};

    tiledb_subarray_add_range(model->tiledb_ctx, subarray, 0, &depth_range[0], &depth_range[1], NULL);
    tiledb_subarray_add_range(model->tiledb_ctx, subarray, 1, &lat_range[0],  &lat_range[1],  NULL);
    tiledb_subarray_add_range(model->tiledb_ctx, subarray, 2, &lon_range[0],  &lon_range[1],  NULL);
    tiledb_query_set_subarray_t(model->tiledb_ctx, query, subarray);

    tiledb_query_set_data_buffer(model->tiledb_ctx, query, "vp", &val_vp, &bytes_vp);
    tiledb_query_set_data_buffer(model->tiledb_ctx, query, "vs", &val_vs, &bytes_vs);
    tiledb_query_set_data_buffer(model->tiledb_ctx, query, "rho", &val_rho, &bytes_rho);

    if (tiledb_query_submit(model->tiledb_ctx, query) != TILEDB_OK) {
        tiledb_error_t* err = NULL;
        tiledb_ctx_get_last_error(model->tiledb_ctx, &err);
        const char* msg = NULL;
        tiledb_error_message(err, &msg);
        fprintf(stderr, "[ERROR] TileDB query read failed: %s\n", msg);
        tiledb_error_free(&err);
    } else {

        tiledb_query_status_t status;
        tiledb_query_get_status(model->tiledb_ctx, query, &status);

        if (status == TILEDB_COMPLETED) {
            if(muscal2_ucvm_debug) {
                fprintf(stderrfp,"Array Indices    -> Depth Idx: %d | Lat Idx: %d | Lon Idx: %d\n", 
                   pt->dep_idx, pt->lat_idx, pt->lon_idx);
                fprintf(stderrfp,"Physical Coords  -> Depth: %.2f | Latitude: %.4f | Longitude: %.4f\n", 
                   real_depth, real_lat, real_lon);
                fprintf(stderrfp,"Material Values  -> VP  : %f m/s\n", val_vp);
                fprintf(stderrfp,"                    VS  : %f m/s\n", val_vs);
                fprintf(stderrfp,"                    RHO : %f kg/m^3\n", val_rho);
            }
            data->vp=val_vp;
            data->vs=val_vs;
            data->rho=val_rho;
            } else {
                fprintf(stderr, "[ERROR] Query finished with unexpected status flag: %d\n", status);
            }
    }

    if (subarray) tiledb_subarray_free(&subarray);
    if (query) tiledb_query_free(&query);
}

int get_interp_property(muscal2_dataset_t *model, muscal2_pt_info_t *pt, muscal2_properties_t *data) {
   return 1;

}

int _buffer_offset(muscal2_dataset_t *model, int x_idx, int  y_idx, int z_idx) {
    int nx=model->nx;
    int ny=model->ny;
    int nz=model->nz;

    int offset= (z_idx)*(ny * nx)+(y_idx)*(nx)+x_idx;
    if(muscal2_ucvm_debug) { fprintf(stderrfp,"\nTarget offset %d : idx lon/lat/dep = %d/%d/%d\n", offset,x_idx, y_idx, z_idx); }

    return offset;
}

int get_1dnn_property(muscal2_dataset_t *model, muscal2_pt_info_t *pt, muscal2_properties_t *data) {
    if(muscal2_ucvm_debug) { fprintf(stderrfp,"\ncalling get_1dnn_property\n"); }

    KDVec3 *best=NULL;
    float best_dist=FLT_MAX;
    int best_idx=0;

    KDVec3 query;
    KDNodeSetup *kdsurface=model->kdsurface;
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


float _interp_a_point(muscal2_dataset_t *model, float *buffer, muscal2_pt_info_t *pt) {
    int lon_idx=pt->lon_idx;
    int lat_idx=pt->lat_idx;
    int dep_idx=pt->dep_idx;
    float lon_percent=pt->lon_percent;
    float lat_percent=pt->lat_percent;
    float dep_percent=pt->dep_percent;

    if(pt->lon_idx < 0 || pt->lat_idx < 0 || pt->dep_idx < 0 ||
         pt->lon_idx +1 >= model->nx || pt->lat_idx +1 >= model->ny || pt->dep_idx+1 >= model->nz ) {
        // out of bound
        return -1;	
    }

    float val0= buffer[_buffer_offset(model,lon_idx,lat_idx,dep_idx)];      // x,    y, z
    float val1= buffer[_buffer_offset(model,lon_idx+1,lat_idx,dep_idx)];    // x+1,  y, z 
    float val2= buffer[_buffer_offset(model,lon_idx,lat_idx+1,dep_idx)];    // x,  y+1, z 
    float val3= buffer[_buffer_offset(model,lon_idx+1,lat_idx+1,dep_idx)];  // x+1,y+1, z
    float val4= buffer[_buffer_offset(model,lon_idx,lat_idx,dep_idx+1)];    // x,    y, z+1
    float val5= buffer[_buffer_offset(model,lon_idx+1,lat_idx,dep_idx+1)];  // x+1,  y, z+1
    float val6= buffer[_buffer_offset(model,lon_idx,lat_idx+1,dep_idx+1)];  // x,  y+1, z+1
    float val7= buffer[_buffer_offset(model,lon_idx+1,lat_idx+1,dep_idx+1)];// x+1,y+1, z+1
        
    float val00= val0 * (1-lon_percent) + val1 * lon_percent;    
    float val11= val4 * (1-lon_percent) + val5 * lon_percent;    
    float val22= val2 * (1-lon_percent) + val3 * lon_percent;    
    float val33= val6 * (1-lon_percent) + val7 * lon_percent;    

    float val000 = val00 * (1-lat_percent) + val22 * lat_percent;
    float val111 = val11 * (1-lat_percent) + val33 * lat_percent;

    float val0000 = val000 * (1-dep_percent) + val111 * dep_percent;
    return val0000;
}
