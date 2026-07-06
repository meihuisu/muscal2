# Autoconf/automake file

# General compiler/linker flags
AM_CFLAGS = ${CFLAGS}
AM_FCFLAGS = ${FCFLAGS}
AM_LDFLAGS = ${LDFLAGS}

all:
if WITH_MUSCAL2_LARGEDATA_DIR
	echo "USING WITH_MUSCAL2_LARGEDATA_DIR in muscal2"
else
	echo "NOT USING WITH_MUSCAL2_LARGEDATA_DIR in muscal2"
	if [ ! -d "muscal2" ]; then \
		./make_data_files.py; \
	fi 
endif

install:
if WITH_MUSCAL2_LARGEDATA_DIR
	if [ ! -d "${prefix}/data/muscal2" ]; then \
		$(MKDIR_P) ${prefix}/data; \
		$(LN_S) -f $(MUSCAL2_LARGEDATA_DIR) ${prefix}/data/muscal2; \
		echo "USING WITH_MUSCAL2_LARGEDATA_DIR in muscal2"; \
	fi
	cp config ${prefix}/data
else
	if [ ! -d "${prefix}/data/muscal2" ]; then \
		$(MKDIR_P) ${prefix}/data; \
		mv muscal2 ${prefix}/data; \
		$(LN_S) -f ${prefix}/data/muscal2 .; \
		echo "NOT USING WITH_MUSCAL2_LARGEDATA_DIR in muscal2"; \
	fi
	cp config ${prefix}/data
endif

clean:
	echo "clean"
