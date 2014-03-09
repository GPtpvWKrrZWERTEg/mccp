.SUFFIXES:	.a .la .ln .o .lo .s .S .c .cc .cpp .i .y .l

.c.o:
	LC_ALL=C $(CC) $(CFLAGS) $(CPPFLAGS) -c $(srcdir)/$< -o $@

.c.lo:
	$(LTCOMPILE_CC) -c $(srcdir)/$*.c

.c.i:
	LC_ALL=C $(CC) -E $(CPPFLAGS) $(srcdir)/$*.c | uniq > $(srcdir)/$*.i

.cc.o:
	LC_ALL=C $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $(srcdir)/$< -o $@

.cc.lo:
	$(LTCOMPILE_CXX) -c $(srcdir)/$< -o $@

.cc.i:
	LC_ALL=C $(CXX) -E $(CPPFLAGS) $(srcdir)/$*.cc | uniq > $(srcdir)/$*.i

.cpp.o:
	LC_ALL=C $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $(srcdir)/$< -o $@

.cpp.lo:
	$(LTCOMPILE_CXX) -c $(srcdir)/$< -o $@

.cpp.i:
	LC_ALL=C $(CXX) -E $(CPPFLAGS) $(srcdir)/$*.cpp | uniq > $(srcdir)/$*.i

.s.lo:
	$(LTCOMPILE_CC) -c $(srcdir)/$*.s

.s.o:
	LC_ALL=C $(CC) $(CFLAGS) $(CPPFLAGS) -c $(srcdir)/$*.s

.S.lo:
	$(LTCOMPILE_CC) -c $(srcdir)/$*.S

.S.o:
	LC_ALL=C $(CC) $(CFLAGS) $(CPPFLAGS) -c $(srcdir)/$*.S

.c.s:
	LC_ALL=C $(CC) -S $(CFLAGS) $(CPPFLAGS) -c $(srcdir)/$*.c \
		-o $(srcdir)/$*.s

.cc.s:
	LC_ALL=C $(CXX) -S $(CXXFLAGS) $(CPPFLAGS) -c $(srcdir)/$*.cc \
		-o $(srcdir)/$*.s

.cpp.s:
	LC_ALL=C $(CXX) -S $(CXXFLAGS) $(CPPFLAGS) -c $(srcdir)/$*.cpp \
		-o $(srcdir)/$*.s

.y.c:
	$(YACC) -d -o $@ $?

.l.c:
	$(LEX) -o $@ $?


all::		ALL
ALL::		$(TARGETS)

ifdef INSTALL_EXE_TARGETS
install::	install-exe
install-exe::	$(INSTALL_EXE_TARGETS)
	@if test ! -z "$(INSTALL_EXE_TARGETS)" -a \
		 ! -z "$(INSTALL_EXE_DIR)" ; then \
		$(MKDIR) $(INSTALL_EXE_DIR) > /dev/null 2>&1 ; \
		for i in $(INSTALL_EXE_TARGETS) ; do \
			$(LTINSTALL_EXE) $$i $(INSTALL_EXE_DIR) ; \
		done ; \
	fi
else
install::	install-exe
install-exe::
	@true
endif

ifdef INSTALL_LIB_TARGETS
install::	install-lib
install-lib::	$(INSTALL_LIB_TARGETS)
	@if test ! -z "$(INSTALL_LIB_TARGETS)" -a \
		 ! -z "$(INSTALL_LIB_DIR)" ; then \
		$(MKDIR) $(INSTALL_LIB_DIR) > /dev/null 2>&1 ; \
		for i in $(INSTALL_LIB_TARGETS) ; do \
			$(LTINSTALL_LIB) $$i $(INSTALL_LIB_DIR) ; \
		done ; \
	fi
else
install::	install-lib
install-lib::
	@true
endif

ifdef INSTALL_HEADER_TARGETS
install::	install-header
install-header::	$(INSTALL_HEADER_TARGETS)
	@if test ! -z "$(INSTALL_HEADER_TARGETS)" -a \
		 ! -z "$(INSTALL_HEADER_DIR)" ; then \
		$(MKDIR) $(INSTALL_HEADER_DIR) > /dev/null 2>&1 ; \
		for i in $(INSTALL_HEADER_TARGETS) ; do \
			$(LTINSTALL_HEADER) $$i $(INSTALL_HEADER_DIR) ; \
		done ; \
	fi
else
install::	install-header
install-header::
	@true
endif

ifdef INSTALL_CONFIG_TARGETS
install::	install-config
install-config::	$(INSTALL_CONFIG_TARGETS)
	@if test ! -z "$(INSTALL_CONFIG_TARGETS)" -a \
		 ! -z "$(INSTALL_CONFIG_DIR)" ; then \
		$(MKDIR) $(INSTALL_CONFIG_DIR) > /dev/null 2>&1 ; \
		for i in $(INSTALL_CONFIG_TARGETS) ; do \
			$(INSTALL_DATA) $$i $(INSTALL_CONFIG_DIR) ; \
		done ; \
	fi
else
install::	install-config
install-config::
	@true
endif

ifdef SRCS
depend::
	@if test ! -z "$(SRCS)" ; then \
		> .depend ; \
		$(CC) -M $(CPPFLAGS) $(SRCS) | sed 's:\.o\::\.lo\::' \
		> .depend ; \
		if test $$? -ne 0 ; then \
			echo depend in `pwd` failed. ; \
		else \
			echo depend in `pwd` succeeded. ; \
		fi ; \
	fi
else
depend::
	@true
endif

ifdef TARGET_LIB
$(TARGET_LIB):	$(OBJS)
	$(LTCLEAN) $@
	$(LTLIB_CC) -o $@ $(OBJS) $(LDFLAGS) $(DEP_LIBS)
endif

ifdef TARGET_EXE
$(TARGET_EXE):	$(OBJS)
	$(LTCLEAN) $@
	$(LTEXE_CC) -o $@ $(OBJS) $(DEP_LIBS) $(LDFLAGS) 
endif

clean::
	$(LTCLEAN) $(OBJS) *.i *~ *.~*~ core core.* *.core $(TARGETS) \
		*.o *.lo \
		./clang.mk ./scan-build.mk ./icc.mk \
		./fortify.mk ./fortify.fpr ./fortify.rtf ./fortify.pdf
	$(RM) -rf ./html ./scan-result .libs

distclean::	clean
	$(RM) Makefile .depend

clang::
	@( \
		$(MAKE) clean ; \
		$(RM) -rf ./clang.mk ; \
		echo "CC = clang" > ./clang.mk ; \
		if test $$? -eq 0; then \
			(SITECONF_MK=`pwd`/clang.mk $(MAKE)) ; \
		fi ; \
		$(RM) -f ./clang.mk ; \
	)

icc::
	@( \
		$(MAKE) clean ; \
		$(RM) -rf ./icc.mk ; \
		echo "CC = icc" > ./icc.mk ; \
		if test $$? -eq 0; then \
			(SITECONF_MK=`pwd`/icc.mk $(MAKE)) ; \
		fi ; \
		$(RM) -f ./icc.mk ; \
	)

warn-check::
	@( \
		$(MAKE) clean > /dev/null 2>&1 ; \
		$(MAKE) > ./m.out 2>&1 ; \
		if test $$? -eq 0 -a -f ./m.out ; then \
			grep ' warning:' ./m.out | awk -F: '{ print $$1 }' | \
			sort | uniq ; \
		fi ; \
		$(RM) -f ./m.out ; \
	)

warn-blame::
	@( \
		$(MAKE) clean > /dev/null 2>&1 ; \
		$(MAKE) > ./m.out 2>&1 ; \
		if test $$? -eq 0 -a -f ./m.out ; then \
			sh $(MKRULESDIR)/warn-blame.sh ./m.out ; \
		fi ; \
		$(RM) -f ./m.out ; \
	)

scan-build::
	@( \
		$(MAKE) clean > /dev/null 2>&1 ; \
		$(RM) -rf ./scan-build.mk ./scan-result ; \
		scan-build sh -c 'echo CC = $${CC} > ./scan-build.mk' \
			> /dev/null 2>&1 ; \
		if test $$? -eq 0; then \
			(SITECONF_MK=`pwd`/scan-build.mk \
				scan-build -o ./scan-result \
				sh -c "$(MAKE)" ; ) ; \
		fi ; \
		$(RM) -f ./scan-build.mk ; \
	)

scan-build-blame::
	@( \
		$(RM) -f ./m.out ; \
		$(MAKE) scan-build > ./m.out 2>&1 ; \
		if test $$? -eq 0 -a -r ./m.out ; then \
			sh $(MKRULESDIR)/warn-blame.sh ./m.out ; \
		fi ; \
		$(RM) -f ./m.out ; \
	)

fortify::
	@( \
		$(MAKE) clean ; \
		$(RM) -rf ./fortify.mk ./fortify.fpr ; \
		echo 'ifeq ($(__SITECONF__),.post.)' > ./fortify.mk ; \
		echo 'CC=sourceanalyzer -f $(TOPDIR)/fortify.fpr -append gcc' >> ./fortify.mk ; \
		echo 'endif' >> ./fortify.mk ; \
		(SITECONF_MK=`pwd`/fortify.mk $(MAKE)) ; \
		$(RM) -f ./fortify.mk ; \
		if test -f ./fortify.fpr; then \
			ReportGenerator -format rtf -source ./fortify.fpr \
				-f ./fortify.rtf ; \
		fi ; \
	)

dostext::
	sh $(MKRULESDIR)/doDosText.sh

doxygen::
	sh $(MKRULESDIR)/mkfiles.sh
	$(RM) -rf ./html ./latex
	doxygen $(MKRULESDIR)/doxygen.conf
	$(RM) -rf ./latex ./.files

ifdef DIRS
all depend clean distclean install install-exe install-lib install-header install-config dostext doxygen::
	@for i in / $(DIRS) ; do \
		case $$i in \
			/) continue ;; \
			*) if test -d $$i ; then \
				(cd $$i && $(MAKE) $@) || exit 1 ; \
			   fi ;; \
		esac ; \
	done
endif


wc::
	@find . -type f -name '*.c' -o -name '*.cpp' -o -name '*.h' | \
	xargs wc

beautify::
	@find . -type f -name '*.c' -o -name '*.cpp' -o -name '*.h' | \
	xargs sh $(MKRULESDIR)/beautify

revert::
	@git status . | grep modified: | awk '{ print $$NF }' | \
	xargs git checkout

Makefiles::
	@if test -x $(TOPDIR)/config.status; then \
		(cd $(TOPDIR); LC_ALL=C sh ./config.status; \
			$(RM) config.log) ; \
	fi

ifdef LIBTOOL_DEPS
libtool::	$(LIBTOOL_DEPS)
	@if test -x $(TOPDIR)/config.status; then \
		(cd $(TOPDIR); LC_ALL=C sh ./config.status libtool; \
			$(RM) config.log) ; \
	fi
endif


$(DEP_MCCP_UTIL_LIB)::
	(cd $(BUILD_LIBDIR) && $(MAKE) $(MCCP_UTIL_LIB))
