

ifndef RPATH
RPATH = .
endif

CFLAGS = -Wall -std=c++11 -Wno-sign-compare -Wno-unused-variable -Wno-unused-function -Wfatal-errors -Wno-shift-count-overflow -Wno-tautological-constant-out-of-range-compare

ifdef CLANG
CC = clang++
LD = clang++
CFLAGS += -Werror -DCLANG -ftemplate-depth=512
else
CC = g++
LD = g++
endif

ifdef OSX
LDFLAGS = -Wl
CFLAGS += -stdlib=libc++ -DOSX -m32 -mmacosx-version-min=10.7
CFLAGS += -DTEXT_SERIALIZATION
else
LDFLAGS = -Wl,-rpath=$(RPATH) -static-libstdc++
endif

CMD:=$(shell rm -f zagadka)

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

SRCS = time_queue.cpp level.cpp model.cpp square.cpp util.cpp monster.cpp square_factory.cpp view.cpp creature.cpp item_factory.cpp item.cpp inventory.cpp debug.cpp player.cpp window_view.cpp field_of_view.cpp view_object.cpp creature_factory.cpp shortest_path.cpp effect.cpp equipment.cpp level_maker.cpp monster_ai.cpp attack.cpp tribe.cpp name_generator.cpp event.cpp location.cpp skill.cpp fire.cpp ranged_weapon.cpp map_layout.cpp trigger.cpp map_memory.cpp view_index.cpp collective.cpp player_control.cpp task.cpp controller.cpp village_control.cpp poison_gas.cpp minion_equipment.cpp statistics.cpp options.cpp renderer.cpp tile.cpp map_gui.cpp gui_elem.cpp item_attributes.cpp creature_attributes.cpp serialization.cpp unique_entity.cpp entity_set.cpp gender.cpp main.cpp gzstream.cpp singleton.cpp technology.cpp encyclopedia.cpp input_queue.cpp minimap_gui.cpp music.cpp test.cpp sectors.cpp vision.cpp animation.cpp clock.cpp square_type.cpp creature_action.cpp collective_control.cpp renderable.cpp bucket_map.cpp task_map.cpp movement_type.cpp collective_builder.cpp player_message.cpp minion_task_map.cpp gui_builder.cpp known_tiles.cpp collective_teams.cpp progress_meter.cpp entity_name.cpp collective_config.cpp spell.cpp spell_map.cpp spectator.cpp visibility_map.cpp model_builder.cpp file_sharing.cpp stack_printer.cpp highscores.cpp main_loop.cpp construction_map.cpp level_builder.cpp move_info.cpp movement_set.cpp position.cpp stair_key.cpp position_map.cpp collective_attack.cpp territory.cpp drag_and_drop.cpp sound.cpp sound_library.cpp campaign.cpp game.cpp sunlight_info.cpp creature_listener.cpp event_generator.cpp

LIBS = -L/usr/lib/x86_64-linux-gnu -lsfml-audio -lsfml-graphics -lsfml-window -lsfml-system $(BOOST_LIBS) -lz -lpthread -lcurl ${LDFLAGS}


OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.cpp=.o))
DEPS = $(addprefix $(OBJDIR)/,$(SRCS:.cpp=.d))

##############################################################################


all: check_serial gen_version $(NAME)

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
