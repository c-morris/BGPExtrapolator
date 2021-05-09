# Convert latex documentation to markdown using pandoc
#
# The latex version is the "source" material other formats (markdown, pdf, etc)
# should be generated from.

SRC_DIR := ./tex/
SOURCE_FILES := tex
SOURCES := $(shell find $(SRC_DIR) -name "*.$(SOURCE_FILES)")

OUT_FILES := md
OUTPUTS := $(patsubst $(SRC_DIR)%.$(SOURCE_FILES), %.$(OUT_FILES), $(SOURCES))

PANDOC := pandoc
PDFLAGS := -t gfm


all: $(OUTPUTS)
	cp -r $(SRC_DIR)/media ./

%$(OUT_FILES): $(SRC_DIR)%$(SOURCE_FILES)
	$(PANDOC) $(PDFLAGS) -o $@ $<


.PHONY: pdf clean cleanall

pdf:
	make -C $(SRC_DIR)

cleanall: clean
	make -C $(SRC_DIR) clean

clean:
	rm -f *.md || true

