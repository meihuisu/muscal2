# Autoconf/automake file

objects =

# General compiler/linker flags
AM_CFLAGS = ${CFLAGS} ${CPPFLAGS} -I$(prefix)/include -I${UCVM_INSTALL_PATH}/lib/tiledb/include
AM_LDFLAGS = ${LDFLAGS} -L$(prefix)/lib ${LIBS} -L${UCVM_INSTALL_PATH}/lib/tiledb/lib64 -ltiledb -Wl,-rpath,${UCVM_INSTALL_PATH}/lib/tiledb/lib64 -lm

TARGETS = muscal2_surface build_kdtree query_kdtree

all: $(TARGETS)

install:
	cp muscal2_surface ${prefix}/bin
	cp build_kdtree ${prefix}/bin
	cp query_kdtree ${prefix}/bin

clean:
	rm -rf $(TARGETS)
	rm -rf *.o

$(objects): %.o: %.c
	$(CC) -fPIC -DDYNAMIC_LIBRARY $(AM_CFLAGS) -o $@ -c $^

muscal2_surface.o: muscal2_surface.c
	$(CC) $(AM_CFLAGS) -o $@ -c $^

muscal2_surface : muscal2_surface.o libmuscal2.a
	$(CC) -o $@ $^ $(AM_LDFLAGS)

build_kdtree.o: build_kdtree.c
	$(CC) $(AM_CFLAGS) -o $@ -c $^

build_kdtree: build_kdtree.o kdtree_util.o
	$(CC) -o $@ $^ $(AM_LDFLAGS)

query_kdtree: query_kdtree.o kdtree_util.o
	$(CC) -o $@ $^ $(AM_LDFLAGS)

