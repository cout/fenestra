CXXFLAGS += -Wall -g -MMD -MP -std=c++17
LDFLAGS += -ldl -lGL -lGLX -lglfw -lepoxy -lportaudiocpp -lasound

# CXXFLAGS += $(shell pkg-config gstreamer-1.0 --cflags)
# LDFLAGS += $(shell pkg-config gstreamer-1.0 --libs)

CXXFLAGS += $(shell pkg-config gstreamermm-1.0 --cflags)
LDFLAGS += $(shell pkg-config gstreamermm-1.0 --libs)

CXXFLAGS += $(shell pkg-config --cflags jsoncpp)
LDFLAGS += $(shell pkg-config --libs jsoncpp)

FENESTRA_OBJS = \
  fenestra.o

LIST_AUDIO_DEVICES_OBJS = \
  tools/list-portaudio-devices.o

OBJS = $(FENESTRA_OBJS) $(LIST_AUDIO_DEVICES_OBJS)

BIN = fenestra tools/list-portaudio-devices

all: $(BIN)

fenestra: $(FENESTRA_OBJS)

tools/list-portaudio-devices: $(LIST_AUDIO_DEVICES_OBJS)

$(BIN):
	$(CXX) $^ -o $@ $(LDFLAGS)

$(OBJS): Makefile

GENERATED_FILES += $(OBJS)

.PHONY: clean
clean:
	$(RM) $(GENERATED_FILES)

DEPFILES = $(patsubst %.o,%.d,$(OBJS))
-include $(DEPFILES)
