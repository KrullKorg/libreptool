LIBS = $(REPTOOL_LIBS)

AM_CPPFLAGS = $(REPTOOL_CFLAGS) \
              -DG_LOG_DOMAIN=\"libreptool\"

parser.tab.c parser.tab.h: parser.y
	bison -d $(srcdir)/parser.y

lexycal.yy.c lexycal.yy.h: lexycal.fl
	flex -o $(srcdir)/lexycal.yy.c --header-file=$(srcdir)/lexycal.yy.h $(srcdir)/lexycal.fl

lib_LTLIBRARIES = libreptool.la

libreptool_la_LDFLAGS = -no-undefined

rptmarshal.c: rptmarshal.h reptool_marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $(srcdir)/reptool_marshal.list --body --prefix=_rpt_marshal > $(srcdir)/$@

rptmarshal.h: reptool_marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $(srcdir)/reptool_marshal.list --header --prefix=_rpt_marshal > $(srcdir)/$@

libreptool_la_SOURCES = \
                        parser.tab.c \
                        lexycal.yy.c \
                        rptobject.c \
                        rptobjecttext.c \
                        rptobjectline.c \
                        rptobjectrect.c \
                        rptobjectellipse.c \
                        rptobjectimage.c \
                        rptreport.c \
                        rptprint.c \
                        rptcommon.c \
                        rptmarshal.c

libreptool_include_HEADERS = \
                  libreptool.h \
                  rptobject.h \
                  rptobjecttext.h \
                  rptobjectline.h \
                  rptobjectrect.h \
                  rptobjectellipse.h \
                  rptobjectimage.h \
                  rptreport.h \
                  rptprint.h \
                  rptcommon.h

noinst_HEADERS = \
                 parser.tab.h \
                 lexycal.yy.h \
                 rptreport_priv.h \
                 rptmarshal.h

EXTRA_DIST = \
             parser.y \
             lexycal.fl

libreptool_includedir = $(includedir)/libreptool
