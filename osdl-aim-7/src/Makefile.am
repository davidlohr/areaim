
INCLUDES 	= -I$(top_builddir)/include
bin_PROGRAMS = reaim

# if WITH_READLINE
# LIBADD_READLINE = -lreadline
# else
LIBADD_READLINE =
# endif

reaim_LDADD = $(top_builddir)/lib/libltp.a $(LIBADD_READLINE)
reaim_SOURCES = driver.c suite.h files.h ram.c aimlist.h \
	rand.c times_ms.c testerr.h add.c creat_clo.c drv_funcs.c \
	div.c disk_funcs.c pipe_test.c mul.c num_fcns.c int_fcns.c \
	disk1.c disk_src.c fun.c funcal.c funcal.h

install-data-local:
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(datadir)/reaim; \
	$(mkinstalldirs) $(DESTDIR)$(datadir)/reaim/results; \
	for workfile in $(top_srcdir)/data/workfile.*; do \
	  if test -f $$workfile; then \
 	    $(INSTALL_DATA) $$workfile $(DESTDIR)$(datadir)/reaim; \
	  fi \
	done 
	for srpt in $(srcdir)/*.sh; do \
	  if test -f $$srpt; then \
 	    $(INSTALL_DATA) $$srpt $(DESTDIR)$(datadir)/reaim; \
	    chmod ugo+x $(DESTDIR)$(datadir)/reaim/*.sh; \
	  fi \
	done 
	if test -f $(top_srcdir)/data/reaim.config; then \
	  $(INSTALL_DATA) $(top_srcdir)/data/reaim.config $(DESTDIR)$(datadir)/reaim; \
	fi
	if test -f $(top_srcdir)/data/fakeh.tgz; then \
	  $(INSTALL_DATA) $(top_srcdir)/data/fakeh.tgz $(DESTDIR)$(datadir)/reaim; \
	fi
	if test -f $(srcdir)/true; then \
	  $(INSTALL_DATA) $(srcdir)/true $(DESTDIR)$(datadir)/reaim; \
	  chmod uo+x $(DESTDIR)$(datadir)/reaim/true; \
	fi
	if test -f $(srcdir)/README; then \
	  $(INSTALL_DATA) $(srcdir)/README $(DESTDIR)$(datadir)/reaim; \
	fi
	@/bin/sh $(top_srcdir)/scripts/instruct.sh
