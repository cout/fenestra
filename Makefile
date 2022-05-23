# === Config ===

REQUIRED_PACKAGES = jsoncpp ftgl libzstd
OPTIONAL_PACKAGES = alsa portaudiocpp gstreamermm-1.0 libpulse
PACKAGES = $(REQUIRED_PACKAGES) $(OPTIONAL_PACKAGES)

CXXFLAGS += -Wall -ggdb -MMD -MP -std=c++17 -Og -pthread -I src
LDFLAGS += -ldl -lGL -lGLU -lGLX -lglfw -lSOIL -lepoxy -lpthread

package_exists = $(shell pkg-config --short-errors --exists $(1) && echo $(1))
have_definition = $(shell echo "-DHAVE_$(1)" | tr a-z A-Z | sed -e 's/-[0-9.]\+$$//')
installed_packages = $(foreach package,$(PACKAGES),$(call package_exists,$(package)))
is_installed = $(if $(filter $(1),$(installed_packages)),yes)

$(foreach package,$(REQUIRED_PACKAGES), \
  $(if $(call is_installed,$(package)),, \
  $(error Error: package $(package) is required)))

CXXFLAGS += $(foreach package,$(installed_packages),$(call have_definition,$(package)))
CXXFLAGS += $(shell pkg-config --cflags $(installed_packages))
LDFLAGS += $(shell pkg-config --libs $(installed_packages))

all:

# === Fenestra ===

FENESTRA_OBJS = \
  src/fenestra/fenestra.o \
  src/fenestra/plugins/KeyHandler.o \
  src/fenestra/plugins/Savefile.o \
  src/fenestra/plugins/ssr/SSRVideoStreamWriter.o

OBJS += $(FENESTRA_OBJS)
BIN += fenestra
ICONS += icons/fenestra.png

fenestra: $(FENESTRA_OBJS)

icons/fenestra.png: icons/fenestra.svg

# === Perflog viewer ===

PERFLOGVIEWER_OBJS = \
  tools/perflog-viewer.o

OBJS += $(PERFLOGVIEWER_OBJS)
BIN += tools/perflog-viewer
ICONS += icons/perflog-viewer.png

tools/perflog-viewer: $(PERFLOGVIEWER_OBJS)

icons/perflog-viewer.png: icons/perflog-viewer.svg

# === Perflog2csv ===

PERFLOG2CSV_OBJS = \
  tools/perflog2csv.o

OBJS += $(PERFLOG2CSV_OBJS)

BIN += tools/perflog2csv

tools/perflog2csv: $(PERFLOG2CSV_OBJS)

# === List portaudio devices ===

ifeq ($(call is_installed,portaudiocpp),yes)

LIST_PORTAUDIO_DEVICES_OBJS = \
  tools/list-portaudio-devices.cpp

OBJS += $(LIST_PORTAUDIO_DEVICES_OBJS)
BIN += tools/list-portaudio-devices

tools/list-portaudio-devices: $(LIST_PORTAUDIO_DEVICES_OBJS)

endif

# === Common ===

all: $(BIN) $(ICONS)

$(BIN):
	$(CXX) $^ -o $@ $(CPPFLAGS) $(LDFLAGS)

$(ICONS):
	inkscape $^ --export-png $@

$(OBJS): Makefile

GENERATED_FILES += $(OBJS)

.PHONY: clean
clean:
	$(RM) $(GENERATED_FILES)

DEPFILES = $(patsubst %.o,%.d,$(OBJS))
-include $(DEPFILES)
