TOPDIR = .
 
include $(TOPDIR)/Preamble.make

RPM_TOPDIR = $(CURDIR)

RPMBUILD = $(shell /usr/bin/which rpmbuild 2>/dev/null || /usr/bin/which rpm 2>/dev/null || echo /bin/false)

SUSEBUILD = $(shell if test -r /etc/SuSE-release; then echo yes; else echo no; fi)

PYVERSION = $(shell echo $(pyexecdir) | sed -e 's/.*python\([0-9]\.[0-9]\).*/\1/')

ifeq ($(SUSEBUILD),yes)
PYGTK_NAME = python-gtk
CHKCONFIG_DEP = aaa_base
COMPILE_PY = 0
else
PYGTK_NAME = pygtk2
CHKCONFIG_DEP = chkconfig
COMPILE_PY = 1
endif

TOOLSARCH = $(shell $(TOPDIR)/rpmarch.guess tools $(TOPDIR))

ifeq ($(TOOLSARCH),error)
$(error could not detect architecture for tools)
endif

SUBDIRS = libo2dlm libo2cb libocfs2 fsck.ocfs2 mkfs.ocfs2 mounted.ocfs2 tunefs.ocfs2 debugfs.ocfs2 o2cb_ctl ocfs2_hb_ctl mount.ocfs2 listuuid sizetest extras patches

ifdef BUILD_OCFS2CDSL
SUBDIRS += ocfs2cdsl
endif

ifdef BUILD_OCFS2CONSOLE
SUBDIRS += ocfs2console
endif

SUBDIRS += vendor

DEBIAN_FILES =					\
	debian/README.Debian			\
	debian/changelog			\
	debian/compat				\
	debian/control				\
	debian/copyright			\
	debian/ocfs2-tools.docs			\
	debian/ocfs2-tools.install		\
	debian/ocfs2-tools.manpages		\
	debian/ocfs2-tools.postinst		\
	debian/ocfs2-tools.prerm		\
	debian/ocfs2console.install		\
	debian/ocfs2console.manpages		\
	debian/ocfs2console.postinst		\
	debian/ocfs2console.prerm		\
	debian/rules

DIST_FILES = \
	COPYING					\
	CREDITS					\
	MAINTAINERS				\
	README					\
	README.O2CB				\
	Config.make.in				\
	Preamble.make				\
	Postamble.make				\
	aclocal.m4				\
	glib-2.0.m4				\
	python.m4				\
	pythondev.m4				\
	runlog.m4				\
	config.guess				\
	config.sub				\
	configure				\
	configure.in				\
	install-sh				\
	mkinstalldirs				\
	rpmarch.guess				\
	documentation/ocfs2_faq.txt		\
	documentation/users_guide.txt		\
	documentation/samples/cluster.conf	\
	$(DEBIAN_FILES)

DIST_RULES = dist-subdircreate

.PHONY: dist dist-subdircreate dist-bye dist-fresh distclean

dist-subdircreate:
	$(TOPDIR)/mkinstalldirs $(DIST_DIR)/documentation/samples
	$(TOPDIR)/mkinstalldirs $(DIST_DIR)/debian

dist-bye:
	-rm -rf $(DIST_TOPDIR)

dist-fresh: dist-bye
	$(TOPDIR)/mkinstalldirs $(DIST_TOPDIR)

dist: dist-fresh dist-all
	GZIP=$(GZIP_OPTS) tar chozf $(DIST_TOPDIR).tar.gz $(DIST_TOPDIR)
	$(MAKE) dist-bye

distclean: clean
	rm -f Config.make config.status config.cache config.log

srpm: dist
	$(RPMBUILD) -bs --define "_sourcedir $(RPM_TOPDIR)" --define "_srcrpmdir $(RPM_TOPDIR)" --define "pygtk_name $(PYGTK_NAME)" --define "pyversion $(PYVERSION)" --define "chkconfig_dep $(CHKCONFIG_DEP)" --define "compile_py $(COMPILE_PY)" $(TOPDIR)/vendor/common/ocfs2-tools.spec

rpm: srpm
	$(RPMBUILD) --rebuild --define "pygtk_name $(PYGTK_NAME)" --define "pyversion $(PYVERSION)" --define "chkconfig_dep $(CHKCONFIG_DEP)" --define "compile_py $(COMPILE_PY)" $(TOOLSARCH) "ocfs2-tools-$(DIST_VERSION)-$(RPM_VERSION).src.rpm"

def:
	@echo $(TOOLSARCH)

include $(TOPDIR)/Postamble.make
