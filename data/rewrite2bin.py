#!/usr/bin/env python3

## rewrite2bin.py

import sys
import netCDF4 as nc
import numpy as np
import pdb


def load_ncdata(fpath, varnm):
    ds = nc.Dataset(fpath)
    loaddata = ds[varnm][:]
    var=loaddata.data
    ds.close()
    return var

def check_bindata(fpath,idx):
    data = np.fromfile(fpath, dtype=np.float32)
    print("FOR ", fpath, "  first ", data[idx]);
    del data
    return;



#### MAIN ####
args=sys.argv
ncfpath=args[1]

loni = load_ncdata(ncfpath, 'longitude')
lon_cnt=len(loni)
print('loni/first/last/lon_cnt => ',len(loni), loni[0], loni[(len(loni)-1)], lon_cnt)
#print(loni)

lati = load_ncdata(ncfpath, 'latitude')
lat_cnt=len(lati)
print('lati/first/last/lat_cnt => ',len(lati), lati[0], lati[(len(lati)-1)], lat_cnt)
#print(lati)

depi = load_ncdata(ncfpath, 'depth')
dep_cnt=len(depi)
print('depi/first/last/dep_cnt => ',len(depi), depi[0], depi[(len(depi)-1)], dep_cnt)
#print(depi)

vpi = load_ncdata(ncfpath, 'vp')
vpi_1d=vpi.flatten()
vpi_1d=vpi_1d.astype(np.float32)
with open('vp.dat', 'wb') as f:
    vpi_1d.tofile(f)
del vpi
del vpi_1d

vsi = load_ncdata(ncfpath, 'vs')
vsi_1d=vsi.flatten()
vsi_1d=vsi_1d.astype(np.float32)
#idx = np.where(~np.isnan(vsi_1d))[0][0]
#value = vsi_1d[idx]
#print("ORIG vs idx(", idx, ") value =>",value)
with open('vs.dat', 'wb') as f:
    vsi_1d.tofile(f)
del vsi
del vsi_1d
#check_bindata("vs.dat",idx)

rhoi = load_ncdata(ncfpath, 'rho')
rhoi_1d=rhoi.flatten()
rhoi_1d=rhoi_1d.astype(np.float32)
with open('rho.dat', 'wb') as f:
    rhoi_1d.tofile(f)
del rhoi
del rhoi_1d

