CC      ?= gcc
OS       = $(shell uname)
CCOPTS	 = -g -fPIC
LIBS	 = 
INCS	 = 
VERSION ?= 0.0.3
NAME     = mempool
BUILDROOT ?= $(CURDIR)/build
MAINTAINER ?= Unknown <unknown@example.com>

ifeq ($(OS),Darwin)
	LIB_EXT      = dylib
	SHARED_FLAG  = -dynamiclib
	RPATH_FLAG   = -Wl,-rpath,@loader_path
	SONAME_FLAG  =
else
	LIB_EXT      = so
	SHARED_FLAG  = -shared
	RPATH_FLAG   = -Wl,-rpath,'$$ORIGIN'
	SONAME_FLAG  = -Wl,-soname,libmempool.$(LIB_EXT)
endif

BINARY	 = pool_test
LIBRARY	 = libmempool.$(LIB_EXT)

SRC	 = libmempool.c pool_test.c
OBJ	 = $(SRC:.c=.o)

all: $(BINARY)

$(LIBRARY): libmempool.o
	$(CC) $(SHARED_FLAG) $(SONAME_FLAG) -o $@ $^

$(BINARY): $(LIBRARY) pool_test.o
	$(CC) $(CCOPTS) -o $(BINARY) pool_test.o -L. -lmempool $(RPATH_FLAG) $(LIBS)

.c.o:
	$(CC) -c $(CCOPTS) $< $(INCS)

install: $(LIBRARY)
	install -d $(DESTDIR)/usr/local/lib $(DESTDIR)/usr/local/include
	install -m 0755 $(LIBRARY) $(DESTDIR)/usr/local/lib/$(LIBRARY)
	install -m 0644 libmempool.h $(DESTDIR)/usr/local/include/libmempool.h

DIST_DIR := $(BUILDROOT)/dist
TARBALL := $(DIST_DIR)/$(NAME)-$(VERSION).tar.gz

dist:
	@mkdir -p $(DIST_DIR)
	@git archive --format=tar.gz --output=$(TARBALL) --prefix=$(NAME)-$(VERSION)/ HEAD
	@echo "Created $(TARBALL)"

RPMTOP := $(BUILDROOT)/rpmbuild
rpm: dist
	@mkdir -p $(RPMTOP)/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
	@cp $(TARBALL) $(RPMTOP)/SOURCES/
	@cp mempool.spec $(RPMTOP)/SPECS/
	@rpmbuild --define "_topdir $(RPMTOP)" -ba mempool.spec

srpm: dist
	@mkdir -p $(RPMTOP)/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
	@cp $(TARBALL) $(RPMTOP)/SOURCES/
	@cp mempool.spec $(RPMTOP)/SPECS/
	@rpmbuild --define "_topdir $(RPMTOP)" -bs mempool.spec

DEBROOT := $(BUILDROOT)/deb
ARCH ?= $(shell dpkg-architecture -qDEB_BUILD_ARCH 2>/dev/null || uname -m)
deb: all
	@mkdir -p $(DEBROOT)/$(NAME)_$(VERSION)_$(ARCH)/DEBIAN
	@mkdir -p $(DEBROOT)/$(NAME)_$(VERSION)_$(ARCH)/usr/local/lib
	@mkdir -p $(DEBROOT)/$(NAME)_$(VERSION)_$(ARCH)/usr/local/include
	@cp $(LIBRARY) $(DEBROOT)/$(NAME)_$(VERSION)_$(ARCH)/usr/local/lib/
	@cp libmempool.h $(DEBROOT)/$(NAME)_$(VERSION)_$(ARCH)/usr/local/include/
	@printf "Package: $(NAME)\nVersion: $(VERSION)\nSection: libs\nPriority: optional\nArchitecture: $(ARCH)\nMaintainer: $(MAINTAINER)\nDescription: Minimal memory pool allocator\n" > $(DEBROOT)/$(NAME)_$(VERSION)_$(ARCH)/DEBIAN/control
	@dpkg-deb --build $(DEBROOT)/$(NAME)_$(VERSION)_$(ARCH)

sdeb: dist
	@mkdir -p $(DEBROOT)/$(NAME)_$(VERSION)_source/DEBIAN
	@mkdir -p $(DEBROOT)/$(NAME)_$(VERSION)_source/usr/src/$(NAME)-$(VERSION)
	@tar -xf $(TARBALL) -C $(DEBROOT)/$(NAME)_$(VERSION)_source/usr/src/
	@printf "Package: $(NAME)-source\nVersion: $(VERSION)\nSection: libs\nPriority: optional\nArchitecture: all\nMaintainer: $(MAINTAINER)\nDescription: Source package for minimal memory pool allocator\n" > $(DEBROOT)/$(NAME)_$(VERSION)_source/DEBIAN/control
	@dpkg-deb --build $(DEBROOT)/$(NAME)_$(VERSION)_source

clean:
	rm -f $(BINARY) $(OBJ) $(LIBRARY)
