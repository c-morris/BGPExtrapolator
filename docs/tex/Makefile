# Compile Latex PDFs

TARGETS := develop extrapolator_design security_policies

# xelatex needed for fontspec, shell escape for minted
LATEX := xelatex
LATEXFLAGS := -shell-escape

TEXFILES := $(wildcard *.tex)
PDFS := $(addsuffix .pdf,$(TARGETS))

all: $(PDFS)

%.pdf: %.tex $(TEXFILES)
	$(LATEX) $(LATEXFLAGS) $*.tex
	bibtex $*
	$(LATEX) $(LATEXFLAGS) $*.tex
	$(LATEX) $(LATEXFLAGS) $*.tex

%.blg: %.bib 
	$(LATEX) $(LATEXFLAGS) $*.tex
	bibtex $*
	$(LATEX) $(LATEXFLAGS) $*.tex

%.toc: %.tex
	$(LATEX) $(LATEXFLAGS) $*.tex

.PHONY: clean
clean:
	rm -f $(PDFS) *.dvi *.aux *.ps *~ *.log *.out *.lot *.lof *.toc *.blg *.bbl url.sty || true

