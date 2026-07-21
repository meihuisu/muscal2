# Autoconf/automake file

objects =

# General compiler/linker flags
AM_TILEDB_CFLAGS = ${CFLAGS} ${CPPFLAGS} -I$(prefix)/include -I${UCVM_INSTALL_PATH}/lib/tiledb/include
AM_TILEDB_LDFLAGS = ${LDFLAGS} -L$(prefix)/lib ${LIBS} -L${UCVM_INSTALL_PATH}/lib/tiledb/lib64 -ltiledb -Wl,-rpath,${UCVM_INSTALL_PATH}/lib/tiledb/lib64 -lm

AM_NC_CFLAGS = ${CFLAGS} ${CPPFLAGS} -I$(prefix)/include -I${UCVM_INSTALL_PATH}/lib/netcdf/include
AM_NC_LDFLAGS = ${LDFLAGS} -L$(prefix)/lib ${LIBS} -L${UCVM_INSTALL_PATH}/lib/netcdf/lib -lnetcdf -lm

AM_CFLAGS = ${CFLAGS} ${CPPFLAGS} -I$(prefix)/include -I${UCVM_INSTALL_PATH}/lib/tiledb/include -I${UCVM_INSTALL_PATH}/lib/netcdf/include
AM_LDFLAGS = ${LDFLAGS} -L$(prefix)/lib ${LIBS} -L${UCVM_INSTALL_PATH}/lib/tiledb/lib64 -ltiledb -Wl,-rpath,${UCVM_INSTALL_PATH}/lib/tiledb/lib64 -L${UCVM_INSTALL_PATH}/lib/netcdf/lib -lnetcdf -lm


TOOL_TARGETS = muscal2_surface build_kdtree query_kdtree
TARGETS = read_coords read_point lookup_point lookup_nc_point

gcc10-gcc -g -I${UCVM_INSTALL_PATH}/lib/netcdf/include -I/home/mei/muscal-git/TileDB/opt/tiledb/include convert_nc_to_tiledb.c -o convert_nc_to_tiledb -L${UCVM_INSTALL_PATH}/lib/netcdf/lib -lnetcdf -L/home/mei/muscal-git/TileDB/opt/tiledb/lib64 -ltiledb -Wl,-rpath,/home/mei/muscal-git/TileDB/opt/tiledb/lib64

./convert_nc_to_tiledb


all: $(TARGETS) $(TOOL_TARGETS)

install:
	cp muscal2_surface ${prefix}/bin
	cp build_kdtree ${prefix}/bin
	cp query_kdtree ${prefix}/bin

clean:
	rm -rf $(TARGETS)
	rm -rf *.o

$(objects): %.o: %.c
	$(CC) -fPIC -DDYNAMIC_LIBRARY $(AM_CFLAGS) -o $@ -c $^

convert_nc_to_tiledb.o: convert_nc_to_tiledb.c
	$(CC) $(AM_CFLAGS) -o $@ -c $^

convert_nc_to_tiledb : convert_nc_to_tiledb.o 
	$(CC) -o $@ $^ $(AM_LDFLAGS)

read_coords.o: read_coords.c
	$(CC) $(AM_TILEDB_CFLAGS) -o $@ -c $^

read_coords : read_coords.o 
	$(CC) -o $@ $^ $(AM_TILEDB_LDFLAGS)

read_point.o: read_point.c
	$(CC) $(AM_TILEDB_CFLAGS) -o $@ -c $^

read_point : read_point.o 
	$(CC) -o $@ $^ $(AM_TILEDB_LDFLAGS)

lookup_point.o: lookup_point.c
	$(CC) $(AM_TILEDB_CFLAGS) -o $@ -c $^

lookup_point : lookup_point.o 
	$(CC) -o $@ $^ $(AM_TILEDB_LDFLAGS)

lookup_nc_point.o: lookup_nc_point.c
	$(CC) $(AM_NC_CFLAGS) -o $@ -c $^

lookup_nc_point : lookup_nc_point.o 
	$(CC) -o $@ $^ $(AM_NC_LDFLAGS)

muscal2_surface.o: muscal2_surface.c
	$(CC) $(AM_NC_CFLAGS) -o $@ -c $^

muscal2_surface : muscal2_surface.o libmuscal2.a
	$(CC) -o $@ $^ $(AM_NC_LDFLAGS)

build_kdtree.o: build_kdtree.c
	$(CC) $(AM_NC_CFLAGS) -o $@ -c $^

build_kdtree: build_kdtree.o kdtree_util.o
	$(CC) -o $@ $^ $(AM_NC_LDFLAGS)

query_kdtree: query_kdtree.o kdtree_util.o
	$(CC) -o $@ $^ $(AM_NC_LDFLAGS)

