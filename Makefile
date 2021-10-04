CXXFLAGS += -Wall -g -MMD -MP -std=c++17
LDFLAGS += -ldl -lGL -lGLX -lglfw -lepoxy -lportaudiocpp -lportaudio -lasound

CXXFLAGS += $(shell pkg-config --cflags jsoncpp)
LDFLAGS += $(shell pkg-config --libs jsoncpp)

all: fenestra

OBJS = \
  fenestra.o

fenestra: $(OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS)

GENERATED_FILES += $(OBJS)

.PHONY: clean
clean:
	$(RM) $(GENERATED_FILES)

DEPFILES = $(patsubst %.o,%.d,$(OBJS))
-include $(DEPFILES)
