

ifndef RPATH
RPATH = .
endif

CFLAGS = -Wall -std=c++11 -Wno-sign-compare -Wno-unused-variable -Wno-unused-function -Wfatal-errors -Wno-shift-count-overflow -Wno-tautological-constant-out-of-range-compare

CLANG = true

ifdef CLANG
CC = clang++
LD = clang++
CFLAGS += -Werror -DCLANG -ftemplate-depth=512
else
CC = g++
LD = g++
endif


ifdef OSX
LDFLAGS += -Wl
CFLAGS += -stdlib=libc++ -DOSX -mmacosx-version-min=10.7
CFLAGS += -DTEXT_SERIALIZATION
else
LDFLAGS = -Wl,-rpath=$(RPATH) -static-libstdc++
endif

LDFLAGS += -fuse-ld=gold

ifdef DATA_DIR
CFLAGS += -DDATA_DIR=\"$(DATA_DIR)\"
endif

ifdef USER_DIR
CFLAGS += -DUSER_DIR=\"$(USER_DIR)\"
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
BOOST_LIBS = -lboost_serialization -lboost_program_options -lboost_filesystem -lboost_system -lboost_thread
else
BOOST_LIBS = -lboost_serialization -lboost_program_options -lboost_filesystem -lboost_system
endif

SRCS = $(shell ls -t *.cpp)

LIBS = -L/usr/lib/x86_64-linux-gnu -lGL -lSDL2 -lcAudio -lSDL2_image $(BOOST_LIBS) -lz -lpthread -lcurl ${LDFLAGS}


OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.cpp=.o))
DEPS = $(addprefix $(OBJDIR)/,$(SRCS:.cpp=.d))

##############################################################################


all: gen_version $(NAME)

stdafx.h.gch: stdafx.h
	$(CC) -x c++-header $< -MMD $(CFLAGS) -o $@

ifndef OPT
PCH = stdafx.h.gch
ifdef CLANG
PCHINC = -include-pch stdafx.h.gch
endif
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

ifdef RELEASE
gen_version:
	bash ./gen_version.sh
endif

parse_game:
	clang++ -std=c++11 -g gzstream.cpp parse_game.cpp util.cpp debug.cpp saved_game_info.cpp -o parse_game -lboost_program_options -lpthread -lboost_serialization -DPARSE_GAME -lz

clean:
	$(RM) $(OBJDIR)/*.o
	$(RM) $(OBJDIR)/*.d
	$(RM) log.out
	$(RM) $(OBJDIR)/test
	$(RM) $(OBJDIR)-opt/*.o
	$(RM) $(OBJDIR)-opt/*.d
	$(RM) $(NAME)
	$(RM) stdafx.h.gch

-include $(DEPS)
