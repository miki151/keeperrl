

ifdef CLANG
CC = clang++
LD = clang++
else
CC = g++
LD = g++
endif

CFLAGS = -Werror -Wall -std=c++0x -Wno-sign-compare -Wno-unused-variable -Wfatal-errors
LDFLAGS = -Wl,-rpath=. -static-libstdc++

CMD:=$(shell rm -f zagadka)

ifdef DATA_DIR
	CFLAGS += -DDATA_DIR=\"$(DATA_DIR)\"
endif

ifdef RELEASE
GFLAG = -DRELEASE
else
GFLAG = -g
endif

ifdef DBG
GFLAG += -g
endif

ifdef OPT
GFLAG += -O3
endif

ifdef PROF
GFLAG += -pg
endif

ifdef DEBUG_STL
GFLAG += -DDEBUG_STL
endif

ifndef OPTFLAGS
	OPTFLAGS = ${GFLAG}
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

SRCS = time_queue.cpp level.cpp model.cpp square.cpp util.cpp monster.cpp  square_factory.cpp  view.cpp creature.cpp message_buffer.cpp item_factory.cpp item.cpp inventory.cpp debug.cpp player.cpp window_view.cpp field_of_view.cpp view_object.cpp creature_factory.cpp quest.cpp shortest_path.cpp effect.cpp equipment.cpp level_maker.cpp monster_ai.cpp attack.cpp tribe.cpp name_generator.cpp event.cpp location.cpp skill.cpp fire.cpp ranged_weapon.cpp map_layout.cpp trigger.cpp map_memory.cpp view_index.cpp pantheon.cpp enemy_check.cpp collective.cpp player_control.cpp task.cpp markov_chain.cpp controller.cpp village_control.cpp poison_gas.cpp minion_equipment.cpp statistics.cpp options.cpp renderer.cpp tile.cpp map_gui.cpp gui_elem.cpp item_attributes.cpp creature_attributes.cpp serialization.cpp unique_entity.cpp entity_set.cpp gender.cpp main.cpp gzstream.cpp singleton.cpp technology.cpp encyclopedia.cpp creature_view.cpp input_queue.cpp user_input.cpp window_renderer.cpp texture_renderer.cpp minimap_gui.cpp music.cpp test.cpp sectors.cpp vision.cpp animation.cpp clock.cpp square_type.cpp creature_action.cpp collective_control.cpp script_context.cpp renderable.cpp extern/scriptbuilder.cpp extern/scripthelper.cpp extern/scriptstdstring.cpp

LIBS = -L/usr/lib/x86_64-linux-gnu -lsfml-audio -lsfml-graphics -lsfml-window -lsfml-system -lboost_serialization -lz -langelscript -lpthread ${LDFLAGS}

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


all: $(OBJDIR) $(OBJDIR)/extern $(NAME)

$(OBJDIR):
	mkdir $(OBJDIR)
$(OBJDIR)/extern:
	mkdir $(OBJDIR)/extern

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

$(NAME): $(OBJS) $(OBJDIR)/main.o
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS)

test: $(OBJS) $(OBJDIR)/test.o
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS)

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
