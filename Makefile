

ifndef RPATH
RPATH = .
endif

CFLAGS = -Wall -std=c++1y -Wno-sign-compare -Wno-unused-variable -Wno-shift-count-overflow -Wno-tautological-constant-out-of-range-compare -Wno-mismatched-tags -ftemplate-depth=512

ifndef CC
CC = g++
endif
LD = $(CC)

ifndef RELEASE
CFLAGS += -Werror -Wimplicit-fallthrough
LDFLAGS += -fuse-ld=lld
endif


ifdef OSX
LDFLAGS += -Wl -L/usr/local/opt/openal-soft/lib
CFLAGS += -stdlib=libc++ -DOSX -mmacosx-version-min=10.7
CFLAGS += -I/usr/local/opt/openal-soft/include
else
LDFLAGS += -Wl,-rpath=$(RPATH)
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

ifdef SANITIZE_ADDRESS
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
BOOST_LIBS = -lboost_system -lboost_thread -lboost_chrono
OPENGL_LIBS = -framework OpenGL
else
BOOST_LIBS =
OPENGL_LIBS = -lGL
endif

LDFLAGS += -L/usr/local/lib

SRCS = $(shell ls -t *.cpp)

LIBS = -L/usr/lib/x86_64-linux-gnu $(OPENGL_LIBS) -lSDL2 -lopenal -lvorbis -lvorbisfile -lSDL2_image $(BOOST_LIBS) -lz -lpthread -lcurl ${LDFLAGS}


OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.cpp=.o))
DEPS = $(addprefix $(OBJDIR)/,$(SRCS:.cpp=.d))
DEPS += $(OBJDIR)/stdafx.h.d

##############################################################################

all:
	@$(MAKE) --no-print-directory info
	@$(MAKE) --no-print-directory compile

compile: gen_version $(NAME)

$(OBJDIR)/stdafx.h.gch: stdafx.h
	$(CC) -x c++-header $< -MMD $(CFLAGS) -o $@

ifndef OPT
PCH = $(OBJDIR)/stdafx.h.gch
PCHINC = -include-pch $(OBJDIR)/stdafx.h.gch
endif

$(OBJDIR)/%.o: %.cpp ${PCH}
	$(CC) -MMD $(CFLAGS) $(PCHINC) -c $< -o $@

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
	@$(CC) -v 2>&1 | head -n 2

ifdef RELEASE
gen_version:
	bash ./gen_version.sh
endif

parse_game:
	clang++ -DPARSE_GAME $(IPATH) -std=c++1y -g gzstream.cpp parse_game.cpp util.cpp debug.cpp saved_game_info.cpp file_path.cpp directory_path.cpp progress.cpp -o parse_game -lpthread -lz

clean:
	$(RM) $(OBJDIR)/*.o
	$(RM) $(OBJDIR)/*.d
	$(RM) log.out
	$(RM) $(OBJDIR)/test
	$(RM) $(OBJDIR)-opt/*.o
	$(RM) $(OBJDIR)-opt/*.d
	$(RM) $(NAME)
	$(RM) $(OBJDIR)/stdafx.h.*

-include $(DEPS)
