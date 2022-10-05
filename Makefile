PRGNAME     = gens-gs-ii

# define regarding OS, which compiler to use
EXESUFFIX = 
TOOLCHAIN = 
CC          = gcc
CCP         = g++
LD          = gcc

# add SDL dependencies
#SDL_LIB     = /usr/lib
#SDL_INCLUDE = /usr/include/SDL

# change compilation / linking flag options
F_OPTS		= -DHOME_SUPPORT
CC_OPTS		= -O2 -fomit-frame-pointer -fdata-sections -ffunction-sections $(F_OPTS)
CFLAGS		= -I$(SDL_INCLUDE) $(CC_OPTS) -I./extlib -Winline
CFLAGS    	+= -I./src -I./src/libcompat -I./src/libgens -I./src/libgens/Cartridge -I/usr/include/SDL2 -I./extlib/popt -I./extlib/minizip -I./extlib/lzma -I./src/libgens/m68000
CFLAGS   	+= -I./src/libgens/cpu -I./src/libgens/cz80 -I./src/libgens/Data/32X -I./src/libgens/Effects
CFLAGS   	+= -I./src/libgens/EmuContext -I./src/libgens/IO -I./src/libgens/macros -I./src/libgens/Save -I./src/libgens/sound
CFLAGS   	+= -I./src/libgens/Util -I./src/libgens/Vdp -I./src/libgenscd -I./src/libgensfile -I./src/libgenskeys -I./src/libgenstext -I./src/libzomg -I./src/gens-sdl

CXXFLAGS	= $(CFLAGS) -std=gnu++11
CFLAGS	    += -std=gnu11
LDFLAGS     = -lSDL2 -ldl -lm -Wl,--as-needed -Wl,--gc-sections -flto -lstdc++ -lpng16 -lz

# Files to be compiled
SRCDIR    = ./src ./src/libcompat ./src/libgens ./src/libgens/Cartridge ./extlib ./extlib/popt ./extlib/minizip ./extlib/lzma ./src/libgens/m68000
SRCDIR   += ./src/libgens/cpu ./src/libgens/cz80 ./src/libgens/Data/32X ./src/libgens/Effects
SRCDIR   += ./src/libgens/EmuContext ./src/libgens/IO ./src/libgens/macros ./src/libgens/Save ./src/libgens/sound
SRCDIR   += ./src/libgens/Util ./src/libgens/Vdp ./src/libgenscd ./src/libgensfile ./src/libgenskeys ./src/libgenstext ./src/libzomg ./src/gens-sdl

VPATH     = $(SRCDIR)
SRC_C   = $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.c))
SRC_CP   = $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.cpp))
OBJ_C   = $(notdir $(patsubst %.c, %.o, $(SRC_C)))
OBJ_CP   = $(notdir $(patsubst %.cpp, %.o, $(SRC_CP)))
OBJS     = $(OBJ_C) $(OBJ_CP)

# Rules to make executable
$(PRGNAME)$(EXESUFFIX): $(OBJS)  
	$(LD) $(CFLAGS) -o $(PRGNAME)$(EXESUFFIX) $^ $(LDFLAGS)

$(OBJ_C) : %.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_CP) : %.o : %.cpp
	$(CCP) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(PRGNAME)$(EXESUFFIX) *.o
