CXXFLAGS += -Wall -ggdb -MMD -MP -std=c++17 -Og -pthread -I popl
LDFLAGS += -ldl -lGL -lGLU -lGLX -lglfw -lepoxy -lportaudiocpp -lasound -lpthread

CXXFLAGS += $(shell pkg-config gstreamermm-1.0 --cflags)
LDFLAGS += $(shell pkg-config gstreamermm-1.0 --libs)

CXXFLAGS += $(shell pkg-config --cflags jsoncpp)
LDFLAGS += $(shell pkg-config --libs jsoncpp)

CXXFLAGS += $(shell pkg-config --cflags ftgl)
LDFLAGS += $(shell pkg-config --libs ftgl)

CXXFLAGS += $(shell pkg-config --cflags libpulse)
LDFLAGS += $(shell pkg-config --libs libpulse)
LDFLAGS += -lpulse-simple

FENESTRA_OBJS = \
  fenestra.o \
  plugins/ssr/SSRVideoStreamWriter.o

LIST_AUDIO_DEVICES_OBJS = \
  tools/list-portaudio-devices.o

PERFLOGVIEWER_OBJS = \
  tools/perflog-viewer.o

OBJS = $(FENESTRA_OBJS) $(LIST_AUDIO_DEVICES_OBJS) ${PERFLOGVIEWER_OBJS}

BIN = fenestra tools/list-portaudio-devices tools/perflog-viewer

all: $(BIN)

fenestra: $(FENESTRA_OBJS)

tools/list-portaudio-devices: $(LIST_AUDIO_DEVICES_OBJS)

tools/perflog-viewer: ${PERFLOGVIEWER_OBJS}

$(BIN):
	$(CXX) $^ -o $@ $(CPPFLAGS) $(LDFLAGS)

$(OBJS): Makefile

GENERATED_FILES += $(OBJS)

.PHONY: clean
clean:
	$(RM) $(GENERATED_FILES)

DEPFILES = $(patsubst %.o,%.d,$(OBJS))
-include $(DEPFILES)
