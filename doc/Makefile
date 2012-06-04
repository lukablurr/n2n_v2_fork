BASENAME = multiple-supernodes
PDF = $(addsuffix .pdf, $(BASENAME))
DVI = $(addsuffix .dvi, $(BASENAME))
TEX = $(addsuffix .tex, $(BASENAME))
BIB = $(addsuffix .bib, $(BASENAME))
LATEX = latex
PDFLATEX = pdflatex
BIBTEX = bibtex

.PHONY: all clean

all: $(PDF)

$(DVI): $(TEX) $(BIB) src/
	$(LATEX) $<
	$(BIBTEX) $(BASENAME)
	# Twice, so TOC is also updated
	$(LATEX) $<
	$(LATEX) $<

$(PDF): $(TEX) $(BIB) src/ 
	make --directory=img/
	$(PDFLATEX) $<
	$(BIBTEX) $(BASENAME)
	# Twice, so TOC is also updated
	$(PDFLATEX) $<
	$(PDFLATEX) $<

clean:
	-rm -f *~ *.aux *.log *.blg *.bbl *.out
