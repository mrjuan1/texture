O := texture
SRCS := $(O).c
OBJS := $(SRCS:%.c=%.o)

TEXTURE := albedo

CC := $(CROSS_COMPILE)gcc
STRIP := $(CROSS_COMPILE)strip
GDB := $(CROSS_COMPILE)gdb

CFLAGS_COMMON := -std=c2x -m64

CFLAGS_DEBUG := $(CFLAGS_COMMON)
CFLAGS_DEBUG += -pedantic -Wall -Wextra
CFLAGS_DEBUG += -g3 -O0

CFLAGS_RELEASE := $(CFLAGS_COMMON)
CFLAGS_RELEASE += -Ofast
CFLAGS_RELEASE += -ftree-vectorize -ffast-math -funroll-loops

LDFLAGS_COMMON := -lSDL2 -lSDL2_image -lGL
LDFLAGS_COMMON += -L. -Wl,-rpath,. -letcpak

all: $(O) $(TEXTURE).png $(TEXTURE).bin

clean:
	@rm -Rfv $(O) *.o *.bin *.pvr

distclean: clean
	@rm -Rfv $(shell cat .gitignore)

$(O): $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS_DEBUG) $(CFLAGS)

run: $(O) $(TEXTURE).png
	./$^

debug: $(O) $(TEXTURE).png
	$(GDB) -ex run --batch --args ./$^

release: $(SRCS)
	$(CC) $^ -o $(O) $(CFLAGS_RELEASE) $(CFLAGS) $(LDFLAGS)
	$(STRIP) -s $(O)

dist: release
	@mkdir -pv dist
	@cp -v $(O) dist

$(TEXTURE).png:
	@if [ ! -e $@ ]; then cp -v ../$@ .; fi

$(TEXTURE).bin: $(O) $(TEXTURE).png
	./$^

help:
	@echo "all - Build executable and convert/pack $(TEXTURE)"
	@echo "clean - Remove executable, object(s) and all converted/packed texture(s)"
	@echo "distclean - Remove everything in .gitignore"
	@echo "$(O) - Build $(O)"
	@echo "<object>.o - Build <object>.o"
	@echo "run - Run executable"
	@echo "debug - Run executable with gdb"
	@echo "release - Build optimised executable"
	@echo "dist - Package optimised executable and all local dependencies in a dist directory"
	@echo "$(TEXTURE).png - Copy PNG texture from parent directory to this directory"
	@echo "$(TEXTURE).bin - Convert and pack PNG texture"
