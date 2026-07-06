#!/usr/bin/env python3

#
#./query_mod.py ./model_updated_CenCal2.nc -120.52 36.255 0
#

import sys
import netCDF4 as nc
import numpy as np
from scipy.interpolate import interpn
import pdb


def load_ncdata(fpath, varnm):
    ds = nc.Dataset(fpath)
    loaddata = ds[varnm][:]
    var=loaddata.data
    return var

def interpolate_mod(loni, lati, depi, vari, lonq, latq, depq):
    points = (depi, lati, loni)
    pointq = np.vstack((depq, latq, lonq)).T
    varq = interpn(points, vari, pointq, method='linear', bounds_error=False, fill_value=np.nan)
    return varq


args=sys.argv
ncfpath=args[1]
lonq, latq, depq = float(args[2]), float(args[3]), float(args[4])
print( 'searching for ', lonq, latq, depq )


loni = load_ncdata(ncfpath, 'longitude')
print('loni => ',len(loni), loni[0], loni[(len(loni)-1)])
#print(loni)
lati = load_ncdata(ncfpath, 'latitude')
print('lati => ',len(lati), lati[0], lati[(len(lati)-1)])
print(lati)
depi = load_ncdata(ncfpath, 'depth')
print('depi => ',len(depi), depi[0], depi[(len(depi)-1)])
#print(depi)

lonitem= np.where(loni == lonq)
latitem= np.where(lati == latq)
depitem= np.where(depi == depq)

lon_idx= lonitem[0][0]
lat_idx= latitem[0][0]
dep_idx= depitem[0][0]

print("target index (lon,lat,dep) ", lon_idx, lat_idx, dep_idx)

vpi = load_ncdata(ncfpath, 'vp')
vsi = load_ncdata(ncfpath, 'vs')
rhoi = load_ncdata(ncfpath, 'rho')

print('vpi size=> ',len(vpi), len(vpi[0]), len(vpi[0][0]))
print('vsi size=> ',len(vsi), len(vsi[0]), len(vsi[0][0]))
print('rhoi size => ',len(rhoi), len(rhoi[0]), len(rhoi[0][0]))

print("using index, vpi =>", vpi[dep_idx][lat_idx][lon_idx], "vsi =>", vsi[dep_idx][lat_idx][lon_idx], "rhoi =>", rhoi[dep_idx][lat_idx][lon_idx])

dep_cnt=len(vpi)
lat_cnt=len(vpi[0])
lon_cnt=len(vpi[0][0])
offset= (dep_idx)*(lat_cnt * lon_cnt)+(lat_idx)*(lon_cnt)+lon_idx 

print("offset..", offset)

vpi_1d=vpi.reshape(-1)
vsi_1d=vsi.reshape(-1)
rhoi_1d=rhoi.reshape(-1)

print("using offset, vpi =>", vpi_1d[offset], "vsi =>", vsi_1d[offset], "rhoi =>", rhoi_1d[offset])

dep_scale_factor=111000.

vpq=interpolate_mod(loni, lati, depi/dep_scale_factor, vpi, lonq, latq, depq/dep_scale_factor)
vsq=interpolate_mod(loni, lati, depi/dep_scale_factor, vsi, lonq, latq, depq/dep_scale_factor)
rhoq=interpolate_mod(loni, lati, depi/dep_scale_factor, rhoi, lonq, latq, depq/dep_scale_factor)
print(f"Vp, Vs, Density = {vpq} m/s, {vsq} m/s, {rhoq} kg/m^3")
