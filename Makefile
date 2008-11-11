# Makefile by aatos.heikkinen@cern.ch

# unset AHSYSTEM
d = ah09bProceedings

#tex       = latex
tex       = pdflatex

pdfviewer = firefox
#pdfviewer = gv
dviviewer = xdvi -allowshell -geometry 700x900+750+100 

all: 
	@echo :::document = $(d)
	@echo :::preparing latex ...
	@rm -f *.aux
	@$(tex) $(d); bibtex $(d); $(tex) $(d); $(tex) $(d); $(tex) $(d)
#	$(pdfviewer) $(d).pdf
ca:
	rm -f $(d).pdf $(d).ps *.out $(d)_*.eps 

pdf:
	@echo :: prepare ps and pdf from dvi
	@dvips -R0 $(d).dvi -o $(d).ps
	@ps2pdf $(d).ps

