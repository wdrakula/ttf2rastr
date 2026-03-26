CXX ?= g++
CXXFLAGS ?= -std=c++11 -O2 -Wall -Wextra -pedantic

TARGET := font_preview
FONT_CPP ?= generated_font.h
FONT_SYMBOL ?= demo_font

.PHONY: all clean regenerate

all: $(TARGET)

$(TARGET): font_preview.cpp $(FONT_CPP)
	$(CXX) $(CXXFLAGS) -DFONT_DATA_FILE=\"$(FONT_CPP)\" -DFONT_SYMBOL_PREFIX=$(FONT_SYMBOL) -o $@ font_preview.cpp

regenerate:
	python main.py --config example_config.json

clean:
	rm -f $(TARGET) preview.ppm
