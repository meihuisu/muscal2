# Autoconf/automake file

objects = muscal2_util.o muscal2_dataset.o cJSON.o kdtree_util.o

# General compiler/linker flags
AM_CFLAGS = ${CFLAGS} ${CPPFLAGS} -I$(prefix)/include -I${UCVM_INSTALL_PATH}/lib/tiledb/include
AM_LDFLAGS = ${LDFLAGS} -L$(prefix)/lib ${LIBS} -L${UCVM_INSTALL_PATH}/lib/tiledb/lib64 -ltiledb -Wl,-rpath,${UCVM_INSTALL_PATH}/lib/tiledb/lib64 -lm

TARGETS = muscal2_query libmuscal2.a libmuscal2.so 

all: $(TARGETS)

install:
	mkdir -p ${prefix}
	mkdir -p ${prefix}/lib
	mkdir -p ${prefix}/include
	cp libmuscal2.so ${prefix}/lib
	cp libmuscal2.a ${prefix}/lib
	cp muscal2.h ${prefix}/include
	cp muscal2_query ${prefix}/bin

clean:
	rm -rf $(TARGETS)
	rm -rf *.o

libmuscal2.a: muscal2_static.o muscal2_util.o muscal2_dataset.o cJSON.o kdtree_util.o
	$(AR) rcs $@ $^

libmuscal2.so: muscal2.o muscal2_util.o muscal2_dataset.o cJSON.o kdtree_util.o
	$(CC) -shared $(AM_FCFLAGS) -o libmuscal2.so $^ $(AM_LDFLAGS)

muscal2.o: muscal2.c
	$(CC) -fPIC -DDYNAMIC_LIBRARY -o $@ -c $^ $(AM_CFLAGS)

muscal2_static.o: muscal2.c
	$(CC) -o $@ -c $^ $(AM_CFLAGS)

$(objects): %.o: %.c
	$(CC) -fPIC -DDYNAMIC_LIBRARY $(AM_CFLAGS) -o $@ -c $^

muscal2_query.o: muscal2_query.c
	$(CC) $(AM_CFLAGS) -o $@ -c $^

muscal2_query : muscal2_query.o libmuscal2.a
	$(CC) -o $@ $^ $(AM_LDFLAGS)

