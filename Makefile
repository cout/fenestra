REQUIRED_PACKAGES = jsoncpp ftgl
OPTIONAL_PACKAGES = alsa portaudiocpp gstreamermm-1.0 libpulse
PACKAGES = $(REQUIRED_PACKAGES) $(OPTIONAL_PACKAGES)

CXXFLAGS += -Wall -ggdb -MMD -MP -std=c++17 -Og -pthread -I popl
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

FENESTRA_OBJS = \
  fenestra.o \
  plugins/ssr/SSRVideoStreamWriter.o

PERFLOGVIEWER_OBJS = \
  tools/perflog-viewer.o

OBJS = $(FENESTRA_OBJS) $(PERFLOGVIEWER_OBJS)

BIN = fenestra tools/perflog-viewer
ICONS = fenestra.png perflog-viewer.png

all: $(BIN) $(ICONS)

fenestra: $(FENESTRA_OBJS)

tools/perflog-viewer: $(PERFLOGVIEWER_OBJS)

ifeq ($(call is_installed,portaudiocpp),yes)

OBJS += LIST_PORTAUDIO_DEVICES_OBJS
BIN += tools/list-portaudio-devices

tools/list-portaudio-devices: $(LIST_PORTAUDIO_DEVICES_OBJS)

endif

$(BIN):
	$(CXX) $^ -o $@ $(CPPFLAGS) $(LDFLAGS)

fenestra.png: fenestra.svg
perflog-viewer.png: perflog-viewer.svg

$(ICONS):
	inkscape $^ --export-png $@

$(OBJS): Makefile

GENERATED_FILES += $(OBJS)

.PHONY: clean
clean:
	$(RM) $(GENERATED_FILES)

DEPFILES = $(patsubst %.o,%.d,$(OBJS))
-include $(DEPFILES)
