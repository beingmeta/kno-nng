# Edit to adapt
PKGNAME	          = nng
LIBNAME	          = nng
KNOCONFIG         = knoconfig
KNOBUILD          = knobuild

prefix		::= $(shell ${KNOCONFIG} prefix)
libsuffix	::= $(shell ${KNOCONFIG} libsuffix)
CMODULES	::= $(DESTDIR)$(shell ${KNOCONFIG} cmodules)
LIBS		::= $(shell ${KNOCONFIG} libs)
LIB		::= $(shell ${KNOCONFIG} lib)
INCLUDE		::= $(shell ${KNOCONFIG} include)
KNO_VERSION	::= $(shell ${KNOCONFIG} version)
KNO_MAJOR	::= $(shell ${KNOCONFIG} major)
KNO_MINOR	::= $(shell ${KNOCONFIG} minor)
PKG_VERSION     ::= $(shell u8_gitversion ./etc/knomod_version)
PKG_MAJOR       ::= $(shell cat ./etc/knomod_version | cut -d. -f1)
FULL_VERSION    ::= ${KNO_MAJOR}.${KNO_MINOR}.${PKG_VERSION}
PATCHLEVEL      ::= $(shell u8_gitpatchcount ./etc/knomod_version)

SUDO            ::= $(shell which sudo)
INIT_CFLAGS     ::= ${CFLAGS}
INIT_LDFLAGS    ::= ${LDFLAGS}
KNO_CFLAGS	::= -I. -fPIC $(shell ${KNOCONFIG} cflags)
KNO_LDFLAGS	::= -fPIC $(shell ${KNOCONFIG} ldflags)
KNO_LIBS	::= $(shell ${KNOCONFIG} libs)
NNG_CFLAGS      ::= -Inng-install/include
NNG_LDFLAGS     ::= -Lnng-install/lib -lnng

XCFLAGS		  = ${INIT_CFLAGS} ${KNO_CFLAGS} ${NNG_CFLAGS}
XLDFLAGS		  = ${INIT_LDFLAGS} ${KNO_LDFLAGS} ${NNG_LDFLAGS}
MKSO		  = $(CC) -shared $(LDFLAGS) $(LIBS)
SYSINSTALL        = /usr/bin/install -c
DIRINSTALL        = /usr/bin/install -d
MSG		  = echo
MACLIBTOOL	  = $(CC) -dynamiclib -single_module -undefined dynamic_lookup \
			$(XLDFLAGS)

GPGID           ::= ${OVERRIDE_GPGID:-FE1BC737F9F323D732AA26330620266BE5AFF294}
CODENAME	::= $(shell ${KNOCONFIG} codename)
REL_BRANCH	::= $(shell ${KNOBUILD} getbuildopt REL_BRANCH current)
REL_STATUS	::= $(shell ${KNOBUILD} getbuildopt REL_STATUS stable)
REL_PRIORITY	::= $(shell ${KNOBUILD} getbuildopt REL_PRIORITY medium)
ARCH            ::= $(shell ${KNOBUILD} getbuildopt BUILD_ARCH || uname -m)
APKREPO         ::= $(shell ${KNOBUILD} getbuildopt APKREPO /srv/repo/kno/apk)
APK_ARCH_DIR      = ${APKREPO}/staging/${ARCH}
RPMDIR		  = dist

# Meta targets

# .buildmode contains the default build target (standard|debugging)
# debug/normal targets change the buildmode
# module build targets depend on .buildmode

default build: staticlibs .buildmode
	make $(shell cat .buildmode)

module: staticlibs
	make nng.${libsuffix}

standard:
	make module
debugging:
	make XCFLAGS="-O0 -g3" module

.buildmode:
	echo standard > .buildmode

debug:
	echo debugging > .buildmode
	make
normal:
	echo standard > .buildmode
	make

STATICLIBS=nng-install/lib/libnng.a

nng/CMakeLists.txt:
	git submodule init
	git submodule update
nng-build nng-install:
	${DIRINSTALL} $@
nng-build/build.ninja: nng/CMakeLists.txt nng-build
	cd nng-build && \
	cmake -G Ninja \
	      -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
	      -DBUILD_SHARED_LIBS=off \
	      -DCMAKE_INSTALL_PREFIX=../nng-install \
	      ../nng

nng.o: nng.c makefile ${STATICLIBS} .buildmode
	$(CC) $(XCFLAGS) -D_FILEINFO="\"$(shell u8_fileinfo ./$< $(dirname $(pwd))/)\""  -o $@ -c $<
	@$(MSG) CC "(NNG)" $@
nng.so: nng.o makefile .buildmode
	 @$(MKSO) -o $@ nng.o -Wl,-soname=$(@F).${FULL_VERSION} \
	          -Wl,--allow-multiple-definition \
	          -Wl,--whole-archive ${STATICLIBS} -Wl,--no-whole-archive \
		  $(XLDFLAGS) ${STATICLIBS}
	 @$(MSG) MKSO "(NNG)" $@

nng.dylib: nng.o
	@$(MACLIBTOOL) -install_name \
		`basename $(@F) .dylib`.${KNO_MAJOR}.dylib \
		$(DYLIB_FLAGS) $(NNG_LDFLAGS) \
		-o $@ nng.o 
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
	${SUDO} u8_install_shared ${LIBNAME}.${libsuffix} ${CMODULES} ${FULL_VERSION} "${SYSINSTALL}"

embed-install update:
	@if test -d ../../../lib/kno; then \
	  cp ${LIBNAME}.${libsuffix} ../../../lib/kno; \
	  echo "Updated $(abspath ../../../lib/kno/${LIBNAME}.${libsuffix})"; \
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

