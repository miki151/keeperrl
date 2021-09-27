

CFLAGS += -Wall -std=c++1y -Wno-sign-compare -Wno-unused-variable -Wno-shift-count-overflow -Wno-tautological-constant-out-of-range-compare -Wno-mismatched-tags -ftemplate-depth=512 -Wno-implicit-conversion-floating-point-to-bool -Wno-string-conversion -Wno-bool-conversion -ftemplate-backtrace-limit=0 -Wunused-function -I/usr/include/SDL2

# Remove if you wish to build KeeperRL without steamworks integration.
ifndef NO_STEAMWORKS
STEAMWORKS = true
endif

ifndef GCC
GCC = clang++
endif
LD = $(GCC)

DEBUG_LD=lld

ifndef RELEASE
CFLAGS += -Werror -Wimplicit-fallthrough
LDFLAGS += -fuse-ld=$(DEBUG_LD)
endif

ifdef OSX
LDFLAGS += -Wl -L/home/michal/Downloads/osxcross/libs/lib -Wl,-rpath .
CFLAGS += -stdlib=libc++ -DOSX -mmacosx-version-min=10.7
CFLAGS += -I/home/michal/Downloads/osxcross/libs/include -I/home/michal/Downloads/osxcross/target/macports/pkgs/opt/local/include/
else
ifndef NO_RPATH
LDFLAGS += -Wl,-rpath=$(or $(RPATH),.)
endif
endif

ifndef RELEASE
LDFLAGS += -Wl,--gdb-index
endif

ifdef DATA_DIR
CFLAGS += -DDATA_DIR=\"$(DATA_DIR)\"
endif

ifdef USER_DIR
CFLAGS += -DUSER_DIR=\"$(USER_DIR)\"
endif

ifdef ENABLE_LOCAL_USER_DIR
CFLAGS += -DENABLE_LOCAL_USER_DIR
endif

ifdef RELEASE
CFLAGS += -DRELEASE
else
CFLAGS += -g
endif

ifdef SANITIZE
CFLAGS += -fsanitize=address
endif

ifdef DBG
CFLAGS += -g
endif

ifdef OPT
CFLAGS += -O3
endif

ifdef PROF
CFLAGS += -pg
endif

ifdef DEBUG_STL
CFLAGS += -DDEBUG_STL
endif

ifdef TEXT_SERIALIZATION
CFLAGS += -DTEXT_SERIALIZATION
endif

ifdef STEAMWORKS
include Makefile-steam
endif

ifdef OPT
OBJDIR = obj-opt
else
OBJDIR = obj
endif

NAME = keeper

ROOT = ./
TOROOT = ./../
IPATH = -I. -I./extern

CFLAGS += $(IPATH)

ifdef OSX
#BOOST_LIBS = -lboost_system -lboost_thread -lboost_chrono
OPENGL_LIBS = -framework OpenGL -framework SDL2 -framework OpenAL -framework SDL2_image -F/home/michal/Downloads/osxcross/libs
else
OPENGL_LIBS = -lGL -lSDL2 -lopenal -lSDL2_image
LDFLAGS += -L/usr/local/lib -L/usr/lib/x86_64-linux-gnu -lopenal -lssl
endif


ifdef STEAMWORKS
SRCS = $(shell ls -t *.cpp)
else
SRCS = $(shell ls -t *.cpp | grep -v steam_.*.cpp)
endif
SRCS += $(shell ls -t extern/*.cpp)

LIBS +=  -lvorbis -lvorbisfile -ltheoradec  -logg $(OPENGL_LIBS) \
	   $(BOOST_LIBS) -lz -lpthread -lcurl ${LDFLAGS} $(STEAM_LIBS)

ifdef EASY_PROFILER
LIBS += libeasy_profiler.so
CFLAGS += -DEASY_PROFILER
endif

OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.cpp=.o))
DEPS = $(addprefix $(OBJDIR)/,$(SRCS:.cpp=.d))
DEPS += $(OBJDIR)/stdafx.h.d

##############################################################################

all:
	@$(MAKE) --no-print-directory info
	@$(MAKE) --no-print-directory compile

compile: gen_version $(NAME)

$(OBJDIR)/stdafx.h.gch: stdafx.h
	$(GCC) -x c++-header $< -MMD $(CFLAGS) -o $@

ifndef RELEASE
PCH = $(OBJDIR)/stdafx.h.gch
PCHINC = -include-pch $(OBJDIR)/stdafx.h.gch
endif

$(OBJDIR)/%.o: %.cpp ${PCH}
	$(GCC) -MMD $(CFLAGS) $(PCHINC) -c $< -o $@

$(NAME): $(OBJS)
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS)

test: $(OBJS) $(OBJDIR)/test.o
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS)

check_serial:
	bash ./check_serial.sh

run: $(NAME)
	./keeper ${RUN_FLAGS} &

run_gdb: $(NAME)
	./run.sh ${RUN_FLAGS}
info:
	@$(GCC) -v 2>&1 | head -n 2

.PHONY:gen_version
gen_version:
ifdef RELEASE
	bash ./gen_version.sh
endif

parse_game:
	clang++ -DPARSE_GAME $(IPATH) -std=c++1y -g gzstream.cpp parse_game.cpp util.cpp debug.cpp saved_game_info.cpp file_path.cpp directory_path.cpp progress.cpp content_id.cpp view_id.cpp color.cpp -o parse_game -lpthread -lz

clean:
	$(RM) $(OBJDIR)/*.o
	$(RM) $(OBJDIR)/*.d
	$(RM) $(OBJDIR)/*.gch
	$(RM) $(OBJDIR)/extern/*.o
	$(RM) $(OBJDIR)/extern/*.d
	$(RM) $(OBJDIR)/extern/*.gch
	$(RM) log.out
	$(RM) $(OBJDIR)/test
	$(RM) $(OBJDIR)-opt/*.o
	$(RM) $(OBJDIR)-opt/*.d
	$(RM) $(OBJDIR)-opt/*.gch
	$(RM) $(OBJDIR)-opt/extern/*.o
	$(RM) $(OBJDIR)-opt/extern/*.d
	$(RM) $(OBJDIR)-opt/extern/*.gch
	$(RM) $(NAME)
	$(RM) unity.cpp
	$(RM) $(OBJDIR)/stdafx.h.*

# Recreates dependency files, in case they got outdated
BASE_SRCS=$(basename $(SRCS))
depends: $(PCH)
	@echo $(BASE_SRCS) | tr '\n' ' ' | xargs -P16 -t -d' ' -I '{}' $(GCC) $(CFLAGS) $(PCHINC) \
		'{}.cpp' -MM -MF $(OBJDIR)/'{}'.d -MT $(OBJDIR)/'{}'.o -E > /dev/null

-include $(DEPS)
