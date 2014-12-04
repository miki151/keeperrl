

ifdef CLANG
CC = clang++
LD = clang++
else
CC = g++
LD = g++
endif

CFLAGS = -Werror -Wall -std=c++0x -Wno-sign-compare -Wno-unused-variable -Wfatal-errors -Wno-shift-count-overflow -Wno-tautological-constant-out-of-range-compare

ifdef OSX
LDFLAGS = -Wl
CFLAGS += -DOSX
else
LDFLAGS = -Wl,-rpath=. -static-libstdc++
endif

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

ifdef SERIAL_DEBUG
GFLAG += -DSERIALIZATION_DEBUG
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

ifdef OSX
BOOST_LIBS = -lboost_serialization -lboost_program_options -lboost_system -lboost_thread-mt
else
BOOST_LIBS = -lboost_serialization -lboost_program_options
endif

SRCS = time_queue.cpp level.cpp model.cpp square.cpp util.cpp monster.cpp  square_factory.cpp view.cpp creature.cpp item_factory.cpp item.cpp inventory.cpp debug.cpp player.cpp window_view.cpp field_of_view.cpp view_object.cpp creature_factory.cpp quest.cpp shortest_path.cpp effect.cpp equipment.cpp level_maker.cpp monster_ai.cpp attack.cpp tribe.cpp name_generator.cpp event.cpp location.cpp skill.cpp fire.cpp ranged_weapon.cpp map_layout.cpp trigger.cpp map_memory.cpp view_index.cpp pantheon.cpp enemy_check.cpp collective.cpp player_control.cpp task.cpp controller.cpp village_control.cpp poison_gas.cpp minion_equipment.cpp statistics.cpp options.cpp renderer.cpp tile.cpp map_gui.cpp gui_elem.cpp item_attributes.cpp creature_attributes.cpp serialization.cpp unique_entity.cpp entity_set.cpp gender.cpp main.cpp gzstream.cpp singleton.cpp technology.cpp encyclopedia.cpp input_queue.cpp window_renderer.cpp minimap_gui.cpp music.cpp test.cpp sectors.cpp vision.cpp animation.cpp clock.cpp square_type.cpp creature_action.cpp collective_control.cpp script_context.cpp renderable.cpp bucket_map.cpp task_map.cpp movement_type.cpp collective_builder.cpp player_message.cpp minion_task_map.cpp gui_builder.cpp known_tiles.cpp collective_teams.cpp progress_meter.cpp collective_config.cpp extern/scriptbuilder.cpp extern/scripthelper.cpp extern/scriptstdstring.cpp

LIBS = -L/usr/lib/x86_64-linux-gnu -lsfml-audio -lsfml-graphics -lsfml-window -lsfml-system $(BOOST_LIBS) -lz -langelscript -lpthread ${LDFLAGS}

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


all: $(NAME)

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
