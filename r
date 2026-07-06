
autoconf
automake --add-missing --force-missing

./configure --prefix=/var/www/html/UCVM_rel/web/model/UCVM_TARGET/model/muscal2 --enable-shared CPPFLAGS='-I/var/www/html/UCVM_rel/web/model/UCVM_TARGET/lib/hdf5/include -I/var/www/html/UCVM_rel/web/model/UCVM_TARGET/lib/netcdf/include' LDFLAGS='-L/var/www/html/UCVM_rel/web/model/UCVM_TARGET/lib/hdf5/lib -L/var/www/html/UCVM_rel/web/model/UCVM_TARGET/lib/netcdf/lib -Wl,-rpath,/var/www/html/UCVM_rel/web/model/UCVM_TARGET/lib/hdf5/lib -Wl,-rpath,/var/www/html/UCVM_rel/web/model/UCVM_TARGET/lib/netcdf/lib' LIBS='-lhdf5 -lnetcdf'
