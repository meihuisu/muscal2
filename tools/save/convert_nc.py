#!/usr/bin/env python3
import os
import sys
import xarray as xr
import tiledb.cf

def convert_netcdf_to_tiledb_chunked(nc_file_path, tiledb_group_uri):
    if not os.path.exists(nc_file_path):
        print(f"Error: Source file '{nc_file_path}' not found.")
        sys.exit(1)

    print(f"Opening NetCDF file (chunked): {nc_file_path}...")
    ds = xr.open_dataset(
        nc_file_path,
        chunks={
            "depth": 70,
            "latitude": 300,
            "longitude": 300
        }
    )
    print(ds)
    print(f"Converting -> TileDB (chunked streaming): {tiledb_group_uri}")
    tiledb.cf.from_xarray(ds, tiledb_group_uri)
    print("Conversion complete successfully!")

if __name__ == "__main__":
    input_nc = "model_MUSCAL_CANVAS_dll0.01_vardz_float32_cmpd.nc"
    output_tdb = "convert_tiledb_array"
    convert_netcdf_to_tiledb_chunked(input_nc, output_tdb)

