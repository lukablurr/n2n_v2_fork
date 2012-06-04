# image conversions (.dia to .eps, .eps to .pdf)

DIA = dia
INKSCAPE = inkscape
EPSTOPDF = epstopdf

DIA_FILES = supernode-bootstrap.dia
SVG_FILES = 
EPS_FILES = $(DIA_FILES:.dia=.eps) snm-hdr.eps snm-req.eps snm-rsp.eps snm-adv.eps
PDF_FILES = $(EPS_FILES:.eps=.pdf)

all: $(PDF_FILES)

%.pdf: %.eps
	$(EPSTOPDF) $<

%.eps: %.dia
	$(DIA) -t eps-pango -e $@ $<

%.eps: %.svg
	$(INKSCAPE) -E $@ $<
