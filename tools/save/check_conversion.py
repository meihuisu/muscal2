import netCDF4
import tiledb
import numpy as np

nc_path = "model_MUSCAL_CANVAS_dll0.01_vardz_float32_cmpd.nc"
tiledb_path = "muscal_canvas_tiledb_array"

print("Opening datasets...")
nc_ds = netCDF4.Dataset(nc_path, "r")
tdb_arr = tiledb.open(tiledb_path, "r")

# Extract NetCDF matrices completely into memory (~400MB total)
print("Loading NetCDF arrays...")
nc_vp = nc_ds.variables["vp"][:]
nc_vs = nc_ds.variables["vs"][:]
nc_rho = nc_ds.variables["rho"][:]

# Extract TileDB matrices. 
# CRITICAL: Slicing from 1 to MAX+1 because your TileDB array is 1-indexed!
print("Loading TileDB arrays...")
tdb_data = tdb_arr[1:211, 1:1252, 1:1302]
tdb_vp = tdb_data["vp"]
tdb_vs = tdb_data["vs"]
tdb_rho = tdb_data["rho"]

def verify_attribute(name, nc_matrix, tdb_matrix):
    print(f"\n--- Validating Attribute: [{name}] ---")
    
    # 1. Shape validation
    print(f"  NetCDF Shape: {nc_matrix.shape} | TileDB Shape: {tdb_matrix.shape}")
    if nc_matrix.shape != tdb_matrix.shape:
        print("  ❌ ERROR: Shape mismatch!")
        return False
        
    # 2. Global Statistical Check
    # (We use np.nanmin/max because the dataset explicitly contains NaN values)
    print(f"  NetCDF Min/Max: {np.nanmin(nc_matrix):.4f} / {np.nanmax(nc_matrix):.4f}")
    print(f"  TileDB Min/Max: {np.nanmin(tdb_matrix):.4f} / {np.nanmax(tdb_matrix):.4f}")

    # 3. Bit-for-Bit exact check
    # equal_nan=True ensures that NaN == NaN evaluates to True
    mismatches = np.count_nonzero(~np.isclose(nc_matrix, tdb_matrix, equal_nan=True))
    
    if mismatches == 0:
        print(f"  ✅ SUCCESS: [{name}] is a 100% bit-for-bit exact match!")
        return True
    else:
        print(f"  ❌ ERROR: Found {mismatches} mismatched array elements!")
        return False

# Run validations
v1 = verify_attribute("vp", nc_vp, tdb_vp)
v2 = verify_attribute("vs", nc_vs, tdb_vs)
v3 = verify_attribute("rho", nc_rho, tdb_rho)

# Clean close
nc_ds.close()
tdb_arr.close()

if v1 and v2 and v3:
    print("\n🎉 CONVERSION IS 100% VERIFIED AND CORRECT!")
else:
    print("\n🚨 CONVERSION FAILED VALIDATION.")

