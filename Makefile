# Makefile by aatos.heikkinen@cern.ch

# Environment variables:
# PDFVIEWER     The PDF viewer program (e.g. xpdf)
# NOVIEWER     Disable the PDF viewer entirely
#              (useful for slow SSH connections)

# unset AHSYSTEM
d = ah09bProceedings

#tex       = latex
tex       = pdflatex

ifndef PDFVIEWER
PDFVIEWER = firefox
endif
#pdfviewer = gv
dviviewer = xdvi -allowshell -geometry 700x900+750+100 

all: 
	@echo :::document = $(d)
	@echo :::preparing latex ...
	@rm -f *.aux
	@$(tex) $(d); bibtex $(d); $(tex) $(d); $(tex) $(d); $(tex) $(d)
ifndef NOVIEWER
	$(PDFVIEWER) $(d).pdf
endif

ca:
	rm -f $(d).pdf $(d).ps *.out $(d)_*.eps 

pdf:
	@echo :: prepare ps and pdf from dvi
	@dvips -R0 $(d).dvi -o $(d).ps
	@ps2pdf $(d).ps

view:
	$(PDFVIEWER) $(d).pdf
