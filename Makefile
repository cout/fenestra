CXXFLAGS += -Wall -g -MMD -MP -std=c++17
LDFLAGS += -ldl -lGL -lGLX -lglfw -lepoxy -lportaudiocpp -lportaudio

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
