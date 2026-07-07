#!/usr/bin/env python3
import os
import tiledb
import numpy as np

def read_velocity_model(tiledb_group_uri, target_depth, target_lat, target_lon):
    """
    Retrieves vp, vs, and rho values for a given geographic point by finding 
    the nearest matching index in the coordinate arrays.
    """
    if not os.path.exists(tiledb_group_uri):
        print(f"Error: TileDB Group '{tiledb_group_uri}' not found.")
        return None

    # 1. Read coordinate arrays from Group Metadata
    with tiledb.Group(tiledb_group_uri, "r") as group:
        depth_coords = group.meta["depth_coords"]
        lat_coords = group.meta["latitude_coords"]
        lon_coords = group.meta["longitude_coords"]

    # 2. Find the nearest array indices using NumPy argmin
    idx_depth = int(np.abs(depth_coords - target_depth).argmin())
    idx_lat = int(np.abs(lat_coords - target_lat).argmin())
    idx_lon = int(np.abs(lon_coords - target_lon).argmin())

    print(f"Target Point: Depth={target_depth}m, Lat={target_lat}°, Lon={target_lon}°")
    print(f"Mapped Array Indices: [{idx_depth}, {idx_lat}, {idx_lon}]")
    print(f"Closest Grid Point: Depth={depth_coords[idx_depth]}m, Lat={lat_coords[idx_lat]}°, Lon={lon_coords[idx_lon]}°\n")

    results = {}
    
    # 3. Read the attributes from the sub-arrays inside the group
    for var_name in ["vp", "vs", "rho"]:
        array_uri = os.path.join(tiledb_group_uri, var_name)
        
        with tiledb.open(array_uri, "r") as array:
            # Slice the single point out from the 3D dense array
            val = array[idx_depth, idx_lat, idx_lon][var_name][0]
            results[var_name] = val

    return results

if __name__ == "__main__":
    group_path = "convert_tiledb_array"
    
    # Example target coordinates (Adjust based on your geographical boundaries)
    d = 500.0   # depth in meters
    lat = 35.5  # latitude
    lon = -120.0 # longitude

    data = read_velocity_model(group_path, d, lat, lon)
    
    if data:
        print("--- Point Query Results ---")
        print(f"P-wave Velocity (vp) : {data['vp']:.2f} m/s")
        print(f"S-wave Velocity (vs) : {data['vs']:.2f} m/s")
        print(f"Density (rho)        : {data['rho']:.2f} kg/m^3")

