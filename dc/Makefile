TARGET = DreamStella.elf
OBJ_DIR = ./obj
BUILD_DIR = ./build
CDROOT_DIR = ./cd_root
ROMDISK_DIR = ./romdisk

EMUCORE_DIR     = ../emucore
COMMON_DIR      = ../common
UNIX_DIR    = ../unix

KOS_CPPFLAGS += -I$(KOS_PORTS)/include -I. -I$(EMUCORE_DIR) -I$(COMMON_DIR) \
                -DUNIX -DBS_LITTLE_ENDIAN -MMD -MP \
                -O2 -fomit-frame-pointer -ffast-math -m4-single-only \
                -fno-exceptions -fno-rtti -fno-threadsafe-statics \
                -fno-strict-aliasing -std=c++17 -fpermissive \
                -DTHUMB_SUPPORT

CORE_FILES = Main Disc GraphicsUtils SplashScreen RomsMenu OptionsMenu FSNode FSNodePOSIX MD5 Base Settings EventHandlerDC Serializer StateManager \
             System OSystemDC Cart Props PropsSet Random Console M6502 TIA TIATables FrameBuffer FrameBufferSoftDC \
             M6532 NullDev Control Switches Joystick Paddles TIASnd SoundDC Thumbulator
             
CART_FILES = Cart2K Cart3E Cart3F Cart4K Cart4KSC CartF4SC Cart4A50 CartF4 CartUA CartDPCPlus \
             CartF6 CartF8 CartF8SC CartFE CartE0 CartDPC CartMC CartE7 CartCV CartEF \
             CartEFSC Cart0840 CartSB CartDF CartBFSC CartDFSC CartF0 CartX07 CartFA CartFA2 \
             CartF6SC CartBF CartCTY CartAR

FILES_NAMES = $(CORE_FILES) $(CART_FILES)
OBJS = $(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(FILES_NAMES)))
DEPS = $(OBJS:.o=.d)

ROMDISK_IMG = $(BUILD_DIR)/romdisk.img
ROMDISK_OBJ = $(OBJ_DIR)/romdisk.o

include $(KOS_BASE)/Makefile.rules

.PHONY: all setup dist clean dev

all: KOS_CPPFLAGS += -DNDEBUG -g0
all: KOS_LDFLAGS += -s

dist: KOS_CPPFLAGS += -DNDEBUG -g0
dist: KOS_LDFLAGS += -s

debug: KOS_CPPFLAGS += -DDEBUG_MODE -g

all: setup dist

debug: setup $(TARGET)

setup:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(CDROOT_DIR)
	@mkdir -p $(ROMDISK_DIR)

$(TARGET): $(ROMDISK_OBJ) $(OBJS) 
	$(KOS_CCPLUS) $(KOS_CPPFLAGS) $(KOS_LDFLAGS) -o $(BUILD_DIR)/$(TARGET) $(KOS_START) \
	$(OBJS) $(ROMDISK_OBJ) \
	-lstdc++ \
	$(KOS_LIBS)

$(ROMDISK_IMG):
	$(KOS_GENROMFS) -f $(ROMDISK_IMG) -d $(ROMDISK_DIR) -v

$(ROMDISK_OBJ): $(ROMDISK_IMG)
	$(KOS_BASE)/utils/bin2o/bin2o $(ROMDISK_IMG) romdisk $(ROMDISK_OBJ)

$(OBJ_DIR)/%.o: %.cxx | setup
	$(KOS_CCPLUS) $(KOS_CPPFLAGS) $(KOS_CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(EMUCORE_DIR)/%.cxx | setup
	$(KOS_CCPLUS) $(KOS_CPPFLAGS) $(KOS_CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(COMMON_DIR)/%.cxx | setup
	$(KOS_CCPLUS) $(KOS_CPPFLAGS) $(KOS_CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(UNIX_DIR)/%.cxx | setup
	$(KOS_CCPLUS) $(KOS_CPPFLAGS) $(KOS_CFLAGS) -c $< -o $@

dist: $(TARGET)
	@mkdcdisc -e $(BUILD_DIR)/$(TARGET) -D $(CDROOT_DIR) -o $(BUILD_DIR)/DreamStella.cdi -V "DreamStella" --disable-data-track-padding --allow-overwrite

clean:
	-rm -rf $(OBJ_DIR)/*
	-rm -rf $(BUILD_DIR)/*

-include $(DEPS)