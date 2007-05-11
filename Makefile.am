## Process this file with automake to produce Makefile.in

AM_CXXFLAGS = -ggdb3 -Wall -O2 -D_REENTRANT -I. -Woverloaded-virtual 
AM_CXXFLAGS += -Wctor-dtor-privacy -Wsign-promo -Wextra -Wfloat-equal -Wpointer-arith 
AM_CXXFLAGS += -Wcast-align -Wwrite-strings -Wconversion -Wsign-compare -Werror 
#-Wold-style-cast

BUILT_SOURCES = src/cppcsp_config.h
CLEANFILES = src/cppcsp_config.h


if X86
MYFLAGS = CPPCSP_X86
else
MYFLAGS = ____CPPCSP_NO_MORE_FLAGS
endif

VAL_SIZEOF_VOIDP = @NCCB_SIZEOF_VOIDP@

if MINGW

src/cppcsp_config.h: Makefile 
	echo -e '#ifndef SIZEOF_VOIDP\n#define SIZEOF_VOIDP $(VAL_SIZEOF_VOIDP)\n#endif' > src/cppcsp_config.h
	echo -e '#ifndef WIN32\n#define WIN32\n#endif\n#define CPPCSP_MINGW\n#define $(MYFLAGS)' >>$@

else

if BSD

src/cppcsp_config.h: Makefile 
	echo -e '#ifndef SIZEOF_VOIDP\n#define SIZEOF_VOIDP $(VAL_SIZEOF_VOIDP)\n#endif' > src/cppcsp_config.h
	echo -e '#define CPPCSP_POSIX\n#define CPPCSP_BSD\n#define CPPCSP_LONGJMP\n#define $(MYFLAGS)' >>$@

else

if LINUX

src/cppcsp_config.h: Makefile 
	echo -e '#ifndef SIZEOF_VOIDP\n#define SIZEOF_VOIDP $(VAL_SIZEOF_VOIDP)\n#endif' > src/cppcsp_config.h
	echo -e '#define CPPCSP_POSIX\n#define CPPCSP_LINUX\n#define CPPCSP_LONGJMP\n#define $(MYFLAGS)' >>$@

else

src/cppcsp_config.h: Makefile 
	echo -e '#ifndef SIZEOF_VOIDP\n#define SIZEOF_VOIDP $(VAL_SIZEOF_VOIDP)\n#endif' > src/cppcsp_config.h
	echo -e '#define CPPCSP_POSIX\n#define $(MYFLAGS)' >>$@

endif
endif
endif



    

libcppcsp2_a_SOURCES = src/cppcsp.cpp src/kernel.cpp src/process.cpp src/atomic.cpp src/alt.cpp

libcppcsp2_adir = $(includedir)/cppcsp
libcppcsp2_a_HEADERS = src/process.h src/kernel.h src/channel_ends.h src/barrier.h src/cppcsp.h src/run.h src/mutex.h src/alt.h src/time.h 
libcppcsp2_a_HEADERS += src/atomic.h src/atomic_impl.h src/mobile.h src/channel.h src/channel_buffers.h src/buffered_channel.h src/channel_factory.h
libcppcsp2_a_HEADERS += src/csprocess.h src/channel_base.h src/thread_local.h src/net_channels.h src/bucket.h
nodist_libcppcsp2_a_HEADERS = src/cppcsp_config.h


libcppcsp2_a_commondir = $(includedir)/cppcsp/common
libcppcsp2_a_common_HEADERS = src/common/basic.h src/common/barrier_bucket.h

libcppcsp2_a_DEPENDENCIES = src/cppcsp_config.h

LINKLIBS = 

lib_LIBRARIES = libcppcsp2.a 

CPPCSP = $(lib_LIBRARIES)

if MINGW
LINKLIBS += -lws2_32 -lwsock32 -lwinmm
else
#Not -lpthread -- GCC has a special -pthread option.
LINKLIBS += -pthread 
endif

libcppcsp2_a_CXXFLAGS = $(AM_CXXFLAGS) 




if BSD

LINKLIBS += -lc_r

else

if MINGW
else

#For the sched_yield function on non-BSD POSIXs:
LINKLIBS += -lrt

endif

endif





noinst_PROGRAMS = TestNorm TestPerf

test: src/cppcsp_config.h $(CPPCSP) TestNorm
	./TestNorm
	
#valgrind keeps reporting huge amounts of errors because of the stack switching, so leaving it out for now:
#	valgrind --tool=memcheck  --max-stackframe=-104675448 --leak-check=yes ./TestNorm
	

#Can't run valgrind on the performance test because valgrind bombs when we eat all the threads!	

testperf: src/cppcsp_config.h $(CPPCSP) TestPerf 
	./TestPerf
	
Shared_Test_Sources = test/test.h test/test.cpp test/time_test.cpp test/barrier_test.cpp test/run_test.cpp test/channel_test.cpp test/mutex_test.cpp test/alt_test.cpp test/buffered_channel_test.cpp test/alt_channel_test.cpp test/net_channel_test.cpp
	
TestNorm_DEPENDENCIES = src/cppcsp_config.h $(CPPCSP) 
TestNorm_SOURCES = test/test_normal.cpp $(Shared_Test_Sources)
TestNorm_LDADD = -L. -lcppcsp2 
TestNorm_LDFLAGS = $(LINKLIBS)


TestPerf_DEPENDENCIES = src/cppcsp_config.h $(CPPCSP)
TestPerf_SOURCES = test/test_perf.cpp $(Shared_Test_Sources)
TestPerf_LDADD = -L. -lcppcsp2 
TestPerf_LDFLAGS = $(LINKLIBS)

SUBDIRS =

EXTRA_DIST = cppcsp.dox cppcsp_internal.dox

docs: cppcsp.dox $(nobase_include_HEADERS)
	doxygen cppcsp.dox
        
docs-dist: docs
	tar -zcvf $(distdir)-docs.tar.gz docs/html/*
	zip -j $(distdir)-docs.zip docs/html/* 
	
         
            
