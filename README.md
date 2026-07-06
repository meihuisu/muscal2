# The Multi-scale Statewide CALifornia Velocity Model with TileDB (muscal2)

<a href="https://github.com/sceccode/muscal.git"><img src="https://github.com/sceccode/muscal/wiki/images/muscal_logo.png"></a>

[![License](https://img.shields.io/badge/License-BSD_3--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)
![GitHub repo size](https://img.shields.io/github/repo-size/sceccode/muscal)
[![muscal-ucvm-ci Actions Status](https://github.com/SCECcode/muscal/workflows/muscal-ucvm-ci/badge.svg)](https://github.com/SCECcode/muscal/actions)

The Multi-Scale CALifornia (MUSCAL) statewide Vp and Vs velocity models provide 
high-quality integrated description of seismic structures across the state. 
Starting with the CANVAS base model (Doody et al., 2023), MUSCAL incorporates 
multiple regional and local velocity datasets into a unified structure, capturing 
features ranging from broad crustal-mantle structures to fine-scale local 
anomalies such as sedimentary basins.
 
To ensure quality, the merged multi-scale models underwent a data-informed refinement
process guided by simulations of small validation events. A key feature of MUSCAL is 
the inclusion of a locally optimized near-surface low-velocity taper (LVT), specifically 
designed to better represent under-resolved shallow structures and improve the accuracy 
of ground-motion predictions.

## Installation

This package is intended to be installed as part of the UCVM framework,
version 25.7 or higher. 

## Contact the authors

If you would like to contact the authors regarding this software,
please e-mail software@scec.org. Note this e-mail address should
be used for questions regarding the software itself (e.g. how
do I link the library properly?). Questions regarding the model's
science (e.g. on what paper is the MUSCAL based?) should be directed
to the model's authors, located in the AUTHORS file.

## To build in standalone mode

To install this package on your computer, please run the following commands:

<pre>
  libtoolize --copy --force
  aclocal -I m4
  autoconf
  automake --add-missing --force-missing
  ./configure --prefix=/dir/to/install
  make
  make install
</pre>

<pre>
example:

./configure --prefix=$UCVM_INSTALL_PATH --enable-shared CPPFLAGS='-I$UCVM_INSTALL_PATH/lib/hdf5/include -I$UCVM_INSTALL_PATH/lib/netcdf/include' LDFLAGS='-L$UCVM_INSTALL_PATH/lib/hdf5/lib -L$UCVM_INSTALL_PATH/lib/netcdf/lib -Wl,-rpath,$UCVM_INSTALL_PATH/lib/hdf5/lib -Wl,-rpath,$UCVM_INSTALL_PATH/lib/netcdf/lib' LIBS='-lhdf5 -lnetcdf'
</pre>

## Note

Optional 1d background base on model's surface/boundary datapoints 

Preprocessing : 
   Extract all surface points from the MUSCAL file layer by layer and create binary surface_945765.in 

Model initialization :
   Load surface points and create KDtree of surface points with 3 axis (lon/lat/depth) 

Query access
   Fill in background with nearest neighboring surface point from surface KDtree 

### muscal_query

### muscal_surface

### query_kdtree

### build_kdtree
