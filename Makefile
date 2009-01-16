# Makefile by aatos.heikkinen@cern.ch

# Environment variables:
# USEVIEWER     Enable the PDF viewer
# PDFVIEWER     The PDF viewer program (e.g. xpdf)

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
ifdef USEVIEWER
	$(PDFVIEWER) $(d).pdf
endif

ca:
	rm -f $(d).pdf $(d).ps *.out $(d)_*.eps $(d).idx *.tar.gz

pdf:
	@echo :: prepare ps and pdf from dvi
	@dvips -R0 $(d).dvi -o $(d).ps
	@ps2pdf $(d).ps

view:
	$(PDFVIEWER) $(d).pdf

release:
	@echo "Building tarball... "
	@./tools/create-tarball.sh
	@echo "Done."

contribution:
	@echo "Preparing contribution tarball..."
	./tools/create-contribution.sh
	@echo "Done."

ahRelease:
	scp -r $(d).tar.gz miheikki@rock.helsinki.fi:public_html/system/refs/heikkinen/. 
	ssh -t -q -l miheikki rock.helsinki.fi 'chmod ugo+xr public_html/system/refs/heikkinen/ah09bProceedings.tar.gz'

