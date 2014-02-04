

CC = g++
LD = g++
CFLAGS = -Wfatal-errors -Werror -Wall -std=c++0x -Wno-sign-compare -Wno-unused-variable -Wl,-rpath=.

CMD:=$(shell rm -f zagadka)

ifdef DATA_DIR
	CFLAGS += -DDATA_DIR=\"$(DATA_DIR)\"
endif

ifdef OPT
GFLAG = -O3 -DRELEASE
else
GFLAG = -g
endif

ifndef OPTFLAGS
	OPTFLAGS = -static-libstdc++ ${GFLAG}
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

SRCS = time_queue.cpp level.cpp model.cpp square.cpp util.cpp monster.cpp  square_factory.cpp  view.cpp creature.cpp message_buffer.cpp item_factory.cpp item.cpp inventory.cpp debug.cpp player.cpp window_view.cpp field_of_view.cpp view_object.cpp creature_factory.cpp quest.cpp shortest_path.cpp effect.cpp equipment.cpp level_maker.cpp monster_ai.cpp attack.cpp attack.cpp tribe.cpp name_generator.cpp event.cpp location.cpp skill.cpp fire.cpp ranged_weapon.cpp action.cpp map_layout.cpp trigger.cpp map_memory.cpp view_index.cpp pantheon.cpp enemy_check.cpp collective.cpp collective_action.cpp task.cpp markov_chain.cpp controller.cpp village_control.cpp poison_gas.cpp minion_equipment.cpp statistics.cpp options.cpp

LIBS = -L/usr/lib/x86_64-linux-gnu -lsfml-graphics -lsfml-window -lsfml-system ${LDFLAGS}

ifdef debug
	CFLAGS += -g
	OBJDIR := ${addsuffix -d,$(OBJDIR)}
	NAME := ${addsuffix -d,$(NAME)}
else
	CFLAGS += $(OPTFLAGS)
endif


OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.cpp=.o))
DEPS = $(addprefix $(OBJDIR)/,$(SRCS:.cpp=.d))

##############################################################################


all: $(OBJDIR) $(NAME)
	ctags --c++-kinds=+p --fields=+iaS --extra=+q *.h

$(OBJDIR):
	mkdir $(OBJDIR)

stdafx.h.gch: stdafx.h
	$(CC) -MMD $(CFLAGS) -c $< -o $@

ifndef OPT
PCH = stdafx.h.gch
endif

$(OBJDIR)/%.o: %.cpp ${PCH}
	$(CC) -MMD $(CFLAGS) -c $< -o $@

$(NAME): $(OBJS) $(OBJDIR)/main.o
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS)

test: $(OBJS) $(OBJDIR)/test.o
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS)

monkey: $(OBJS) $(OBJDIR)/monkey_test.o
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	$(RM) $(OBJDIR)/*.o
	$(RM) $(OBJDIR)/*.d
	$(RM) log13*
	$(RM) log.out
	$(RM) $(OBJDIR)/test
	$(RM) $(OBJDIR)-opt/*.o
	$(RM) $(OBJDIR)-opt/*.d
	$(RM) $(NAME)
	$(RM) stdafx.h.gch

-include $(DEPS)
