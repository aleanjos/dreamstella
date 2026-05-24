TARGET = DreamStella.elf
OBJDIR = obj
BUILD_DIR = ./build

EMUCORE     = ../emucore
COMMON      = ../common
UNIX_DIR    = ../unix

KOS_CPPFLAGS += -I$(KOS_PORTS)/include -I. -I$(EMUCORE) -I$(COMMON) \
                -DUNIX -DBS_LITTLE_ENDIAN -DNDEBUG -g0 -MMD -MP \
                -O2 -fomit-frame-pointer -ffast-math -m4-single-only \
                -fno-exceptions -fno-rtti -fno-threadsafe-statics -flto \
                -DTHUMB_SUPPORT

CORE_FILES = Main Disc GraphicsUtils SplashScreen RomsMenu OptionsMenu FSNode FSNodePOSIX MD5 Base Settings EventHandlerDC Serializer StateManager \
             System OSystemDC Cart Props PropsSet Random Console M6502 TIA TIATables FrameBuffer FrameBufferSoftDC \
             M6532 NullDev Control Switches Joystick Paddles TIASnd SoundDC Thumbulator
             
CART_FILES = Cart2K Cart3E Cart3F Cart4K Cart4KSC CartF4SC Cart4A50 CartF4 CartUA CartDPCPlus \
             CartF6 CartF8 CartF8SC CartFE CartE0 CartDPC CartMC CartE7 CartCV CartEF \
             CartEFSC Cart0840 CartSB CartDF CartBFSC CartDFSC CartF0 CartX07 CartFA CartFA2 \
             CartF6SC CartBF CartCTY CartAR

FILES_NAMES = $(CORE_FILES) $(CART_FILES)
OBJS = $(addprefix $(OBJDIR)/, $(addsuffix .o, $(FILES_NAMES)))
DEPS = $(OBJS:.o=.d)

include $(KOS_BASE)/Makefile.rules

KOS_LDFLAGS += -s -flto

.PHONY: all setup dist clean

all: setup dist

setup:
	@mkdir -p $(OBJDIR)
	@mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS) 
	$(KOS_CCPLUS) $(KOS_CPPFLAGS) $(KOS_LDFLAGS) -o $(BUILD_DIR)/$(TARGET) $(KOS_START) \
      $(OBJS) \
      -lstdc++ \
      $(KOS_LIBS)

$(OBJDIR)/%.o: %.cxx | setup
	$(KOS_CCPLUS) $(KOS_CPPFLAGS) $(KOS_CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(EMUCORE)/%.cxx | setup
	$(KOS_CCPLUS) $(KOS_CPPFLAGS) $(KOS_CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(COMMON)/%.cxx | setup
	$(KOS_CCPLUS) $(KOS_CPPFLAGS) $(KOS_CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(UNIX_DIR)/%.cxx | setup
	$(KOS_CCPLUS) $(KOS_CPPFLAGS) $(KOS_CFLAGS) -c $< -o $@

dist: $(TARGET)
	kos-objcopy -O binary $(BUILD_DIR)/$(TARGET) $(BUILD_DIR)/DreamStella.bin
	mkdir -p cd_root
	scramble $(BUILD_DIR)/DreamStella.bin cd_root/1ST_READ.bin
	mkisofs -C 0,11702 -V "DreamStella" -G IP.bin -r -J -l -o $(BUILD_DIR)/DreamStella.iso cd_root/
	cdi4dc $(BUILD_DIR)/DreamStella.iso $(BUILD_DIR)/DreamStella.cdi

clean:
	-rm -rf $(OBJDIR)
	-rm -rf $(BUILD_DIR)
	-rm -f cd_root/1ST_READ.bin

-include $(DEPS)
