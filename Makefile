.DEFAULT_GOAL := all
.SUFFIXES:

# Portable logic tests and dependency preparation do not require devkitARM.
NEEDS_TOOLCHAIN := $(if $(MAKECMDGOALS),$(filter-out host-test core-test gameplay-test world-test assets-test pipeline-test deps clean,$(MAKECMDGOALS)),all)
ifneq ($(strip $(NEEDS_TOOLCHAIN)),)
ifeq ($(strip $(DEVKITARM)),)
$(error Set DEVKITPRO and DEVKITARM, e.g. /opt/devkitpro and /opt/devkitpro/devkitARM)
endif
include $(DEVKITARM)/3ds_rules
endif

TARGET := 3ds-craft
BUILD := build
TOPDIR ?= $(CURDIR)
ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS := -g -O2 -Wall -Wextra -mword-relocations -ffunction-sections -fdata-sections $(ARCH) $(INCLUDE) -D__3DS__
CXXFLAGS := $(CFLAGS) -std=gnu++17 -fno-rtti -fexceptions
ASFLAGS := -g $(ARCH)
LDFLAGS = -specs=3dsx.specs -g $(ARCH) -Wl,--gc-sections,-Map,$(notdir $*.map)
LIBS := -lcitro2d -lcitro3d -lpng -lz -lctru -lm
LIBDIRS := $(CTRULIB) $(DEVKITPRO)/portlibs/3ds
APP_TITLE := 3DS Craft
APP_DESCRIPTION := New 3DS voxel engine template
APP_AUTHOR := 3DS Craft contributors

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH := $(CURDIR)/source $(CURDIR)/shaders
export DEPSDIR := $(CURDIR)/$(BUILD)
export LD := $(CXX)
export OFILES := $(notdir $(patsubst %.cpp,%.o,$(wildcard source/*.cpp))) world.shbin.o
export INCLUDE := -I$(CURDIR)/include -I$(CURDIR)/third_party -I$(CURDIR)/$(BUILD) $(foreach dir,$(LIBDIRS),-I$(dir)/include)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export _3DSXDEPS := $(OUTPUT).smdh
export _3DSXFLAGS := --smdh=$(OUTPUT).smdh
export APP_TITLE APP_DESCRIPTION APP_AUTHOR

.PHONY: all clean deps host-test cia
all: deps
	@mkdir -p $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(TOPDIR)/Makefile

deps:
	@bash scripts/fetch-deps.sh

HOST_FLAGS := -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -Iinclude
.PHONY: core-test gameplay-test world-test assets-test pipeline-test
host-test: core-test gameplay-test world-test assets-test pipeline-test

core-test:
	@mkdir -p build-host
	$(HOST_CXX) $(HOST_FLAGS) source/Core.cpp source/Village.cpp source/ModelMesh.cpp tests/core_tests.cpp -o build-host/core-tests
	./build-host/core-tests

gameplay-test:
	@mkdir -p build-host
	$(HOST_CXX) $(HOST_FLAGS) source/Core.cpp source/Village.cpp source/ModelMesh.cpp source/BlockStore.cpp source/Gameplay.cpp source/Survival.cpp source/Crafting.cpp source/Animals.cpp tests/gameplay_tests.cpp -o build-host/gameplay-tests
	./build-host/gameplay-tests

world-test:
	@mkdir -p build-host
	$(HOST_CXX) $(HOST_FLAGS) -Itests/platform -pthread source/Core.cpp source/Village.cpp source/ModelMesh.cpp source/BlockStore.cpp source/World.cpp tests/world_tests.cpp -o build-host/world-tests
	./build-host/world-tests

assets-test: deps
	@mkdir -p build-host
	$(HOST_CXX) $(HOST_FLAGS) -Ithird_party source/Assets.cpp tests/assets_tests.cpp -lpng -lz -o build-host/assets-tests
	./build-host/assets-tests build-host/fixtures
pipeline-test: deps
	@mkdir -p build-host
	$(HOST_CXX) $(HOST_FLAGS) -Ithird_party source/Core.cpp source/Village.cpp source/ModelMesh.cpp source/BlockStore.cpp source/Gameplay.cpp source/Survival.cpp source/Assets.cpp source/FileSystem.cpp source/AssetArchive.cpp source/AssetCatalog.cpp source/WorldStore.cpp source/Menu.cpp source/Crafting.cpp tests/pipeline_tests.cpp -lpng -lz -o build-host/pipeline-tests
	./build-host/pipeline-tests build-host/pipeline-fixtures

HOST_CXX ?= g++

# makerom is a separate optional dependency. The test title ID in the RSF is local-only.
cia: all
	makerom -f cia -target t -rsf packaging/new3ds.rsf -elf $(TARGET).elf -icon $(TARGET).smdh -o $(TARGET).cia

clean:
	rm -rf build build-host $(TARGET).elf $(TARGET).3dsx $(TARGET).smdh $(TARGET).cia
else
.PHONY: all
all: $(OUTPUT).3dsx
$(OUTPUT).3dsx: $(OUTPUT).elf $(_3DSXDEPS)
$(OUTPUT).elf: $(OFILES)
$(filter-out world.shbin.o,$(OFILES)): world_shbin.h
world.shbin.o world_shbin.h: world.shbin
	$(bin2o)
-include $(DEPSDIR)/*.d
endif
