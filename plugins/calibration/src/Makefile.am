HEADER_PATH = ../include/relacs/calibration
include ${top_srcdir}/moc4.mk

pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = \
    libcalibrationrestartdelay.la \
    libcalibrationstimulusdelay.la  \
    libcalibrationattenuatorcheck.la \
    libcalibrationinputrangecheck.la



libcalibrationrestartdelay_la_CPPFLAGS = \
    -I$(top_srcdir)/shapes/include \
    -I$(top_srcdir)/daq/include \
    -I$(top_srcdir)/plot/include \
    -I$(top_srcdir)/numerics/include \
    -I$(top_srcdir)/options/include \
    -I$(top_srcdir)/widgets/include \
    -I$(top_srcdir)/relacs/include \
    -I$(srcdir)/../include \
    $(QT_CPPFLAGS) $(NIX_CPPFLAGS)

libcalibrationrestartdelay_la_LDFLAGS = \
    -module -avoid-version \
    $(QT_LDFLAGS) $(NIX_LDFLAGS)

libcalibrationrestartdelay_la_LIBADD = \
    $(top_builddir)/relacs/src/librelacs.la \
    $(top_builddir)/widgets/src/librelacswidgets.la \
    $(top_builddir)/plot/src/librelacsplot.la \
    $(top_builddir)/options/src/librelacsoptions.la \
    $(top_builddir)/daq/src/librelacsdaq.la \
    $(top_builddir)/shapes/src/librelacsshapes.la \
    $(top_builddir)/numerics/src/librelacsnumerics.la \
    $(QT_LIBS) $(NIX_LIBS) $(GSL_LIBS)

$(libcalibrationrestartdelay_la_OBJECTS) : moc_restartdelay.cc

libcalibrationrestartdelay_la_SOURCES = restartdelay.cc

libcalibrationrestartdelay_la_includedir = $(pkgincludedir)/calibration

libcalibrationrestartdelay_la_include_HEADERS = $(HEADER_PATH)/restartdelay.h



libcalibrationstimulusdelay_la_CPPFLAGS = \
    -I$(top_srcdir)/shapes/include \
    -I$(top_srcdir)/daq/include \
    -I$(top_srcdir)/plot/include \
    -I$(top_srcdir)/numerics/include \
    -I$(top_srcdir)/options/include \
    -I$(top_srcdir)/widgets/include \
    -I$(top_srcdir)/relacs/include \
    -I$(srcdir)/../include \
    $(QT_CPPFLAGS) $(NIX_CPPFLAGS)

libcalibrationstimulusdelay_la_LDFLAGS = \
    -module -avoid-version \
    $(QT_LDFLAGS) $(NIX_LDFLAGS)

libcalibrationstimulusdelay_la_LIBADD = \
    $(top_builddir)/relacs/src/librelacs.la \
    $(top_builddir)/widgets/src/librelacswidgets.la \
    $(top_builddir)/plot/src/librelacsplot.la \
    $(top_builddir)/options/src/librelacsoptions.la \
    $(top_builddir)/daq/src/librelacsdaq.la \
    $(top_builddir)/shapes/src/librelacsshapes.la \
    $(top_builddir)/numerics/src/librelacsnumerics.la \
    $(QT_LIBS) $(NIX_LIBS) $(GSL_LIBS)

$(libcalibrationstimulusdelay_la_OBJECTS) : moc_stimulusdelay.cc

libcalibrationstimulusdelay_la_SOURCES = stimulusdelay.cc

libcalibrationstimulusdelay_la_includedir = $(pkgincludedir)/calibration

libcalibrationstimulusdelay_la_include_HEADERS = $(HEADER_PATH)/stimulusdelay.h



libcalibrationattenuatorcheck_la_CPPFLAGS = \
    -I$(top_srcdir)/shapes/include \
    -I$(top_srcdir)/daq/include \
    -I$(top_srcdir)/numerics/include \
    -I$(top_srcdir)/options/include \
    -I$(top_srcdir)/datafile/include \
    -I$(top_srcdir)/plot/include \
    -I$(top_srcdir)/widgets/include \
    -I$(top_srcdir)/relacs/include \
    -I$(srcdir)/../include \
    $(QT_CPPFLAGS) $(NIX_CPPFLAGS)

libcalibrationattenuatorcheck_la_LDFLAGS = \
    -module -avoid-version \
    $(QT_LDFLAGS) $(NIX_LDFLAGS)

libcalibrationattenuatorcheck_la_LIBADD = \
    $(top_builddir)/relacs/src/librelacs.la \
    $(top_builddir)/widgets/src/librelacswidgets.la \
    $(top_builddir)/plot/src/librelacsplot.la \
    $(top_builddir)/datafile/src/librelacsdatafile.la \
    $(top_builddir)/options/src/librelacsoptions.la \
    $(top_builddir)/daq/src/librelacsdaq.la \
    $(top_builddir)/shapes/src/librelacsshapes.la \
    $(top_builddir)/numerics/src/librelacsnumerics.la \
    $(QT_LIBS) $(NIX_LIBS) $(GSL_LIBS)

$(libcalibrationattenuatorcheck_la_OBJECTS) : moc_attenuatorcheck.cc

libcalibrationattenuatorcheck_la_SOURCES = attenuatorcheck.cc

libcalibrationattenuatorcheck_la_includedir = $(pkgincludedir)/calibration

libcalibrationattenuatorcheck_la_include_HEADERS = $(HEADER_PATH)/attenuatorcheck.h



libcalibrationinputrangecheck_la_CPPFLAGS = \
    -I$(top_srcdir)/shapes/include \
    -I$(top_srcdir)/daq/include \
    -I$(top_srcdir)/numerics/include \
    -I$(top_srcdir)/options/include \
    -I$(top_srcdir)/datafile/include \
    -I$(top_srcdir)/plot/include \
    -I$(top_srcdir)/widgets/include \
    -I$(top_srcdir)/relacs/include \
    -I$(srcdir)/../include \
    $(QT_CPPFLAGS) $(NIX_CPPFLAGS)

libcalibrationinputrangecheck_la_LDFLAGS = \
    -module -avoid-version \
    $(QT_LDFLAGS) $(NIX_LDFLAGS)

libcalibrationinputrangecheck_la_LIBADD = \
    $(top_builddir)/relacs/src/librelacs.la \
    $(top_builddir)/widgets/src/librelacswidgets.la \
    $(top_builddir)/plot/src/librelacsplot.la \
    $(top_builddir)/datafile/src/librelacsdatafile.la \
    $(top_builddir)/options/src/librelacsoptions.la \
    $(top_builddir)/daq/src/librelacsdaq.la \
    $(top_builddir)/shapes/src/librelacsshapes.la \
    $(top_builddir)/numerics/src/librelacsnumerics.la \
    $(QT_LIBS) $(NIX_LIBS) $(GSL_LIBS)

$(libcalibrationinputrangecheck_la_OBJECTS) : moc_inputrangecheck.cc

libcalibrationinputrangecheck_la_SOURCES = inputrangecheck.cc

libcalibrationinputrangecheck_la_includedir = $(pkgincludedir)/calibration

libcalibrationinputrangecheck_la_include_HEADERS = $(HEADER_PATH)/inputrangecheck.h



check_PROGRAMS = \
    linktest_libcalibrationrestartdelay_la \
    linktest_libcalibrationstimulusdelay_la  \
    linktest_libcalibrationattenuatorcheck_la \
    linktest_libcalibrationinputrangecheck_la

linktest_libcalibrationrestartdelay_la_SOURCES = linktest.cc
linktest_libcalibrationrestartdelay_la_LDADD = libcalibrationrestartdelay.la

linktest_libcalibrationstimulusdelay_la_SOURCES = linktest.cc
linktest_libcalibrationstimulusdelay_la_LDADD = libcalibrationstimulusdelay.la


linktest_libcalibrationattenuatorcheck_la_SOURCES = linktest.cc
linktest_libcalibrationattenuatorcheck_la_LDADD = libcalibrationattenuatorcheck.la

linktest_libcalibrationinputrangecheck_la_SOURCES = linktest.cc
linktest_libcalibrationinputrangecheck_la_LDADD = libcalibrationinputrangecheck.la

TESTS = $(check_PROGRAMS)

