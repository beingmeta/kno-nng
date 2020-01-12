prefix		::= $(shell knoconfig prefix)
libsuffix	::= $(shell knoconfig libsuffix)
KNO_CFLAGS	::= -I. -fPIC $(shell knoconfig cflags)
KNO_LDFLAGS	::= -fPIC $(shell knoconfig ldflags)
NNG_CFLAGS   ::=
NNG_LDFLAGS  ::=
CFLAGS		::= ${CFLAGS} ${KNO_CFLAGS} ${BSON_CFLAGS} ${NNG_CFLAGS}
LDFLAGS		::= ${LDFLAGS} ${KNO_LDFLAGS} ${BSON_LDFLAGS} ${NNG_LDFLAGS}
CMODULES	::= $(DESTDIR)$(shell knoconfig cmodules)
LIBS		::= $(shell knoconfig libs)
LIB		::= $(shell knoconfig lib)
INCLUDE		::= $(shell knoconfig include)
KNO_VERSION	::= $(shell knoconfig version)
KNO_MAJOR	::= $(shell knoconfig major)
KNO_MINOR	::= $(shell knoconfig minor)
PKG_RELEASE	::= $(cat ./etc/release)
DPKG_NAME	::= $(shell ./etc/dpkgname)
MKSO		::= $(CC) -shared $(LDFLAGS) $(LIBS)
MSG		::= echo
SYSINSTALL      ::= /usr/bin/install -c
MOD_NAME	::= nngkno
MOD_RELEASE     ::= $(shell cat etc/release)
MOD_VERSION	::= ${KNO_MAJOR}.${KNO_MINOR}.${MOD_RELEASE}

GPGID = FE1BC737F9F323D732AA26330620266BE5AFF294
SUDO  = $(shell which sudo)

default: staticlibs
	make nngkno.${libsuffix}

STATICLIBS=installed/lib/libnng.a

nng/.git:
	git submodule init
	git submodule update
nng/cmake-build/build.ninja: nng/.git
	if test ! -d nng/cmake-build; then mkdir nng/cmake-build; fi && \
	cd nng/cmake-build && \
	cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF \
	      -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
	      -DBUILD_SHARED_LIBS=off \
	      -DCMAKE_INSTALL_PREFIX=../../installed \
	      ..

nngkno.o: nngkno.c makefile ${STATICLIBS}
	@$(CC) $(CFLAGS) -o $@ -c $<
	@$(MSG) CC "(NNGKNO)" $@
nngkno.so: nngkno.o makefile
	 $(MKSO) -o $@ nngkno.o -Wl,-soname=$(@F).${MOD_VERSION} \
	          -Wl,--allow-multiple-definition \
	          -Wl,--whole-archive ${STATICLIBS} -Wl,--no-whole-archive \
		 $(LDFLAGS) ${STATICLIBS}
	 @$(MSG) MKSO "(NNGKNO)" $@

nngkno.dylib: nngkno.o
	@$(MACLIBTOOL) -install_name \
		`basename $(@F) .dylib`.${KNO_MAJOR}.dylib \
		$(DYLIB_FLAGS) $(NNG_LDFLAGS) \
		-o $@ nngkno.o 
	@$(MSG) MACLIBTOOL "(NNGKNO)" $@

${STATICLIBS}: nng/cmake-build/build.ninja
	cd nng/cmake-build; ninja install
staticlibs: ${STATICLIBS}

install:
	@${SUDO} ${SYSINSTALL} ${MOD_NAME}.${libsuffix} ${CMODULES}/${MOD_NAME}.so.${MOD_VERSION}
	@echo === Installed ${CMODULES}/${MOD_NAME}.so.${MOD_VERSION}
	@${SUDO} ln -sf ${MOD_NAME}.so.${MOD_VERSION} ${CMODULES}/${MOD_NAME}.so.${KNO_MAJOR}.${KNO_MINOR}
	@echo === Linked ${CMODULES}/${MOD_NAME}.so.${KNO_MAJOR}.${KNO_MINOR} to ${MOD_NAME}.so.${MOD_VERSION}
	@${SUDO} ln -sf ${MOD_NAME}.so.${MOD_VERSION} ${CMODULES}/${MOD_NAME}.so.${KNO_MAJOR}
	@echo === Linked ${CMODULES}/${MOD_NAME}.so.${KNO_MAJOR} to ${MOD_NAME}.so.${MOD_VERSION}
	@${SUDO} ln -sf ${MOD_NAME}.so.${MOD_VERSION} ${CMODULES}/${MOD_NAME}.so
	@echo === Linked ${CMODULES}/${MOD_NAME}.so to ${MOD_NAME}.so.${MOD_VERSION}

clean:
	rm -f *.o *.${libsuffix}
deepclean deep-clean: clean
	if test -f nng/Makefile; then cd nngkno; make clean; fi;
	rm -rf nng/cmake-build installed

debian: nngkno.c makefile \
	dist/debian/rules dist/debian/control \
	dist/debian/changelog.base
	rm -rf debian
	cp -r dist/debian debian

debian/changelog: debian nngkno.c makefile
	@cat debian/changelog.base | etc/gitchangelog kno-nng > $@.tmp
	@if test ! -f debian/changelog; then \
	  mv debian/changelog.tmp debian/changelog; \
	 elif diff debian/changelog debian/changelog.tmp 2>&1 > /dev/null; then \
	  mv debian/changelog.tmp debian/changelog; \
	else rm debian/changelog.tmp; fi

dist/debian.built: nngkno.c makefile debian/changelog
	dpkg-buildpackage -sa -us -uc -b -rfakeroot && \
	touch $@

dist/debian.signed: dist/debian.built
	debsign --re-sign -k${GPGID} ../kno-nng_*.changes && \
	touch $@

dist/debian.updated: dist/debian.signed
	dupload -c ./debian/dupload.conf --nomail --to bionic ../kno-nng_*.changes && touch $@

deb debs dpkg dpkgs: dist/debian.signed

update-apt: dist/debian.updated

debinstall: dist/debian.signed
	${SUDO} dpkg -i ../kno-nng*.deb

debclean:
	rm -rf ../kno-nng_* ../kno-nng-* debian

debfresh:
	make debclean
	make dist/debian.signed
