KNOCONFIG         = knoconfig
KNOBUILD          = knobuild
NNGINSTALL        = nng-install

prefix		::= $(shell ${KNOCONFIG} prefix)
libsuffix	::= $(shell ${KNOCONFIG} libsuffix)
CMODULES	::= $(DESTDIR)$(shell ${KNOCONFIG} cmodules)
LIBS		::= $(shell ${KNOCONFIG} libs)
LIB		::= $(shell ${KNOCONFIG} lib)
INCLUDE		::= $(shell ${KNOCONFIG} include)
KNO_VERSION	::= $(shell ${KNOCONFIG} version)
KNO_MAJOR	::= $(shell ${KNOCONFIG} major)
KNO_MINOR	::= $(shell ${KNOCONFIG} minor)
PKG_RELEASE	::= $(cat ./etc/release)
DPKG_NAME	::= $(shell ./etc/dpkgname)
SUDO            ::= $(shell which sudo)

INIT_CFLAGS     ::= ${CFLAGS}
INIT_LDFLAGS    ::= ${LDFLAGS}
KNO_CFLAGS	::= -I. -fPIC $(shell ${KNOCONFIG} cflags)
KNO_LDFLAGS	::= -fPIC $(shell ${KNOCONFIG} ldflags)
NNG_CFLAGS      ::= -I${NNGINSTALL}/include
NNG_LDFLAGS     ::= -L${NNGINSTALL}/lib -lnng

CFLAGS		  = ${INIT_CFLAGS} ${KNO_CFLAGS} ${NNG_CFLAGS}
LDFLAGS		  = ${INIT_LDFLAGS} ${KNO_LDFLAGS} ${NNG_LDFLAGS}
MKSO		  = $(CC) -shared $(LDFLAGS) $(LIBS)
SYSINSTALL        = /usr/bin/install -c
DIRINSTALL        = /usr/bin/install -d
MSG		  = echo

PKG_NAME	  = nng
GPGID             = FE1BC737F9F323D732AA26330620266BE5AFF294
PKG_VERSION	  = ${KNO_MAJOR}.${KNO_MINOR}.${PKG_RELEASE}
PKG_RELEASE     ::= $(shell cat etc/release)
CODENAME	::= $(shell ${KNOCONFIG} codename)
REL_BRANCH	::= $(shell ${KNOBUILD} getbuildopt REL_BRANCH current)
REL_STATUS	::= $(shell ${KNOBUILD} getbuildopt REL_STATUS stable)
REL_PRIORITY	::= $(shell ${KNOBUILD} getbuildopt REL_PRIORITY medium)
ARCH            ::= $(shell ${KNOBUILD} getbuildopt BUILD_ARCH || uname -m)
APKREPO         ::= $(shell ${KNOBUILD} getbuildopt APKREPO /srv/repo/kno/apk)
APK_ARCH_DIR      = ${APKREPO}/staging/${ARCH}

default: staticlibs
	make nng.${libsuffix}

STATICLIBS=${NNGINSTALL}/lib/libnng.a

nng/.git:
	git submodule init
	git submodule update
nng-build nng-install:
	${DIRINSTALL} $@
nng-build/build.ninja: nng/.git nng-build
	cd nng-build && \
	cmake -G Ninja \
	      -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
	      -DBUILD_SHARED_LIBS=off \
	      -DCMAKE_INSTALL_PREFIX=../nng-install \
	      ../nng

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

nng-install/lib/libnng.a: nng-build/build.ninja nng-install
	cd nng-build; ninja && ninja install
	if test -d nng-install/lib; then \
	  echo > /dev/null; \
	elif test -d nng-install/lib64; then \
	  ln -sf lib64 nng-install/lib; \
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
	rm -rf nng-build nng-install

fresh: clean
	make default
deep-fresh: deep-clean
	make default

gitup gitup-trunk:
	git checkout trunk && git pull

# Debian building

DEBFILES=changelog.base compat control copyright dirs docs install

debian: nng.c makefile \
	dist/debian/rules dist/debian/control \
	dist/debian/changelog.base
	rm -rf debian
	cp -r dist/debian debian
	cd debian; chmod a-x ${DEBFILES}

debian/.build_setup:
	if ! which cmake > /dev/null; then sudo apt install cmake; fi
	if ! which ninja > /dev/null; then sudo apt install ninja-build; fi
	touch $@

debian/changelog: debian nng.c nng.h makefile
	cat debian/changelog.base | \
		knobuild debchangelog kno-${PKG_NAME} ${CODENAME} \
			${REL_BRANCH} ${REL_STATUS} ${REL_PRIORITY} \
	    > $@.tmp
	if test ! -f debian/changelog; then \
	  mv debian/changelog.tmp debian/changelog; \
	elif diff debian/changelog debian/changelog.tmp 2>&1 > /dev/null; then \
	  mv debian/changelog.tmp debian/changelog; \
	else rm debian/changelog.tmp; fi

dist/debian.built: nng.c makefile debian/changelog debian/.build_setup
	dpkg-buildpackage -sa -us -uc -b -rfakeroot && \
	touch $@

dist/debian.signed: dist/debian.built
	if test "$GPGID" = "none" || test -z "${GPGID}"; then  	\
	  echo "Skipping debian signing";			\
	  touch $@;						\
	else 							\
	  debsign --re-sign -k${GPGID} ../kno-nng_*.changes && 	\
	  touch $@;						\
	fi;

deb debs dpkg dpkgs: dist/debian.signed

debinstall: dist/debian.signed
	${SUDO} dpkg -i ../kno-nng*.deb

debclean:
	rm -rf ../kno-nng_* ../kno-nng-* debian

debfresh:
	make debclean
	make dist/debian.signed

# Alpine packaging

staging/alpine:
	@install -d $@

staging/alpine/APKBUILD: dist/alpine/APKBUILD staging/alpine
	cp dist/alpine/APKBUILD staging/alpine

staging/alpine/kno-${PKG_NAME}.tar: staging/alpine
	git archive --prefix=kno-${PKG_NAME}/ -o staging/alpine/kno-${PKG_NAME}.tar HEAD

dist/alpine.setup: staging/alpine/APKBUILD makefile ${STATICLIBS} \
	staging/alpine/kno-${PKG_NAME}.tar
	if [ ! -d ${APK_ARCH_DIR} ]; then mkdir -p ${APK_ARCH_DIR}; fi && \
	( cd staging/alpine; \
		abuild -P ${APKREPO} clean cleancache cleanpkg && \
		abuild checksum ) && \
	touch $@

dist/alpine.done: dist/alpine.setup
	( cd staging/alpine; abuild -P ${APKREPO} ) && touch $@
dist/alpine.installed: dist/alpine.setup
	( cd staging/alpine; abuild -i -P ${APKREPO} ) && touch dist/alpine.done && touch $@


alpine: dist/alpine.done
install-alpine: dist/alpine.done

.PHONY: alpine

