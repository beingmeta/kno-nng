KNOCONFIG       ::= knoconfig
prefix		::= $(shell ${KNOCONFIG} prefix)
libsuffix	::= $(shell ${KNOCONFIG} libsuffix)
KNO_CFLAGS	::= -I. -I./installed/include -fPIC $(shell ${KNOCONFIG} cflags)
KNO_LDFLAGS	::= -fPIC -L./installed/lib $(shell ${KNOCONFIG} ldflags)
NNG_CFLAGS   ::=
NNG_LDFLAGS  ::=
CFLAGS		::= ${CFLAGS} ${KNO_CFLAGS} ${BSON_CFLAGS} ${NNG_CFLAGS}
LDFLAGS		::= ${LDFLAGS} ${KNO_LDFLAGS} ${BSON_LDFLAGS} ${NNG_LDFLAGS}
CMODULES	::= $(DESTDIR)$(shell ${KNOCONFIG} cmodules)
LIBS		::= $(shell ${KNOCONFIG} libs)
LIB		::= $(shell ${KNOCONFIG} lib)
INCLUDE		::= $(shell ${KNOCONFIG} include)
KNO_VERSION	::= $(shell ${KNOCONFIG} version)
KNO_MAJOR	::= $(shell ${KNOCONFIG} major)
KNO_MINOR	::= $(shell ${KNOCONFIG} minor)
PKG_RELEASE	::= $(cat ./etc/release)
DPKG_NAME	::= $(shell ./etc/dpkgname)
MKSO		::= $(CC) -shared $(LDFLAGS) $(LIBS)
MSG		::= echo
SYSINSTALL      ::= /usr/bin/install -c
PKG_NAME	::= nng
PKG_RELEASE     ::= $(shell cat etc/release)
PKG_VERSION	::= ${KNO_MAJOR}.${KNO_MINOR}.${PKG_RELEASE}
APKREPO         ::= $(shell ${KNOCONFIG} apkrepo)
CODENAME	::= $(shell ${KNOCONFIG} codename)
RELSTATUS	::= $(shell ${KNOCONFIG} status)

GPGID = FE1BC737F9F323D732AA26330620266BE5AFF294
SUDO  = $(shell which sudo)

default: staticlibs
	make nng.${libsuffix}

STATICLIBS=installed/lib/libnng.a

nng/.git:
	git submodule init
	git submodule update
nng/cmake-build/build.ninja: nng/.git
	if test ! -d nng/cmake-build; then mkdir nng/cmake-build; fi && \
	cd nng/cmake-build && \
	cmake -G Ninja \
	      -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF \
	      -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
	      -DBUILD_SHARED_LIBS=off \
	      -DCMAKE_INSTALL_PREFIX=../../installed \
	      ..

nng.o: nng.c makefile ${STATICLIBS}
	$(CC) $(CFLAGS) -o $@ -c $<
	@$(MSG) CC "(NNG)" $@
nng.so: nng.o makefile
	 @$(MKSO) -o $@ nng.o -Wl,-soname=$(@F).${PKG_VERSION} \
	          -Wl,--allow-multiple-definition \
	          -Wl,--whole-archive ${STATICLIBS} -Wl,--no-whole-archive \
		  $(LDFLAGS) ${STATICLIBS}
	 @if test ! -z "${COPY_CMODS}"; then cp $@ ${COPY_CMODS}; fi;
	 @$(MSG) MKSO "(NNG)" $@

nng.dylib: nng.o
	@$(MACLIBTOOL) -install_name \
		`basename $(@F) .dylib`.${KNO_MAJOR}.dylib \
		$(DYLIB_FLAGS) $(NNG_LDFLAGS) \
		-o $@ nng.o 
	@if test ! -z "${COPY_CMODS}"; then cp $@ ${COPY_CMODS}; fi;
	@$(MSG) MACLIBTOOL "(NNG)" $@

${STATICLIBS}:
	make nng/cmake-build/build.ninja
	cd nng/cmake-build; ninja install
	if test -d installed/lib; then \
	  echo > /dev/null; \
	elif test -d installed/lib64; then \
	  ln -sf lib64 installed/lib; \
	else echo "No install libdir"; \
	fi

staticlibs: ${STATICLIBS}

${CMODULES}:
	@install -d ${CMODULES}

install: ${CMODULES}
	@${SUDO} ${SYSINSTALL} ${PKG_NAME}.${libsuffix} ${CMODULES}/${PKG_NAME}.so.${PKG_VERSION}
	@echo === Installed ${CMODULES}/${PKG_NAME}.so.${PKG_VERSION}
	@${SUDO} ln -sf ${PKG_NAME}.so.${PKG_VERSION} ${CMODULES}/${PKG_NAME}.so.${KNO_MAJOR}.${KNO_MINOR}
	@echo === Linked ${CMODULES}/${PKG_NAME}.so.${KNO_MAJOR}.${KNO_MINOR} to ${PKG_NAME}.so.${PKG_VERSION}
	@${SUDO} ln -sf ${PKG_NAME}.so.${PKG_VERSION} ${CMODULES}/${PKG_NAME}.so.${KNO_MAJOR}
	@echo === Linked ${CMODULES}/${PKG_NAME}.so.${KNO_MAJOR} to ${PKG_NAME}.so.${PKG_VERSION}
	@${SUDO} ln -sf ${PKG_NAME}.so.${PKG_VERSION} ${CMODULES}/${PKG_NAME}.so
	@echo === Linked ${CMODULES}/${PKG_NAME}.so to ${PKG_NAME}.so.${PKG_VERSION}

embed-install update:
	@if test -d ../../../lib/kno; then \
	  cp ${PKG_NAME}.${libsuffix} ../../../lib/kno; \
	  echo "Updated $(abspath ../../../lib/kno/${PKG_NAME}.${libsuffix})"; \
	else echo "Not embedded in KNO build"; fi

clean:
	rm -f *.o *.${libsuffix}
deepclean deep-clean: clean
	if test -f nng/Makefile; then cd nngkno; make clean; fi;
	rm -rf nng/cmake-build installed

fresh: clean
	make default

debian: nng.c makefile \
	dist/debian/rules dist/debian/control \
	dist/debian/changelog.base
	rm -rf debian
	cp -r dist/debian debian

debian/changelog: debian nng.c nng.h makefile
	cat debian/changelog.base | \
		knomod debchangelog kno-${PKG_NAME} ${CODENAME} ${RELSTATUS} > $@.tmp
	if test ! -f debian/changelog; then \
	  mv debian/changelog.tmp debian/changelog; \
	elif diff debian/changelog debian/changelog.tmp 2>&1 > /dev/null; then \
	  mv debian/changelog.tmp debian/changelog; \
	else rm debian/changelog.tmp; fi

dist/debian.built: nng.c makefile debian/changelog
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

# Alpine packaging

${APKREPO}/dist/x86_64:
	@install -d $@

staging/alpine:
	@install -d $@

staging/alpine/APKBUILD: dist/alpine/APKBUILD staging/alpine
	cp dist/alpine/APKBUILD staging/alpine

staging/alpine/kno-${PKG_NAME}.tar: staging/alpine
	git archive --prefix=kno-${PKG_NAME}/ -o staging/alpine/kno-${PKG_NAME}.tar HEAD

dist/alpine.done: staging/alpine/APKBUILD makefile ${STATICLIBS} \
	staging/alpine/kno-${PKG_NAME}.tar ${APKREPO}/dist/x86_64
	cd staging/alpine; \
		abuild -P ${APKREPO} clean cleancache cleanpkg && \
		abuild checksum && \
		abuild -P ${APKREPO} && \
		touch ../../$@

alpine: dist/alpine.done

.PHONY: alpine

