bin_PROGRAMS=timelatch
timelatch_SOURCES=AsioIOServiceKeep.cpp SerialComm.cpp GLog.cpp timelatch.cpp

if DEBUG
  AM_CFLAGS = -g3 -O0 -Wall -DNDEBUG
  AM_CXXFLAGS = -g3 -O0 -Wall -DNDEBUG
else
  AM_CFLAGS = -O3 -Wall
  AM_CXXFLAGS = -O3 -Wall
endif

timelatch_LDFLAGS = -L/usr/local/lib
BOOST_LIBS = -lboost_thread-mt -lboost_chrono-mt
timelatch_LDADD = ${BOOST_LIBS}
