# Makefile by aatos.heikkinen@cern.ch

# Environment variables:
# USEVIEWER     Enable the PDF viewer
# PDFVIEWER     The PDF viewer program (e.g. xpdf)

# unset AHSYSTEM

d1 = chep09tmva
#d = jpcsSample
d = ah09bProceedings
e = ah09aTalk

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
	@$(tex) $(d) && bibtex $(d) && $(tex) $(d) && $(tex) $(d) && $(tex) $(d)
ifdef USEVIEWER
	$(PDFVIEWER) $(d).pdf
endif

ca:
	rm -f *.pdf *.ps *.out $(d)_*.eps $(d).idx *.tar.gz *.nav *.snm

pdf:
	@echo :: prepare ps and pdf from dvi
	@dvips -R0 $(d).dvi -o $(d).ps
	@ps2pdf $(d).ps

paper:
	@echo :::CHEP09 TMVA paper = $(d1)
	@rm -f *.aux
	@latex $(d1) && latex $(d1) 
#	@$(tex) $(d1) && $(tex) $(d1) 
	@dvips -R0 $(d1).dvi -o $(d1).ps
	@ps2pdf $(d1).ps
ifdef USEVIEWER
	$(PDFVIEWER) $(d1).pdf
endif

view:
	$(PDFVIEWER) $(d).pdf

release:
	@echo "Building tarball... "
	@./tools/create-tarball.sh
	@echo "Done."

contribution:
	@echo "Preparing contribution tarball..."
	@./tools/create-contribution.sh
	@echo "Done."

ahRelease:
	scp -r $(d).tar.gz miheikki@rock.helsinki.fi:public_html/system/refs/heikkinen/. 
	ssh -t -q -l miheikki rock.helsinki.fi 'chmod ugo+xr public_html/system/refs/heikkinen/ah09bProceedings.tar.gz'

ahAddPekka:
	git remote add pekka git://github.com/kaitanie/chep09tmva.git

ahGetPekka:
	git fetch pekka
	git merge pekka/master

talk: 
	@echo :::document = $(e)
	@echo :::preparing latex ...
	@rm -f *.aux
	@$(tex) $(e); bibtex $(e); $(tex) $(e); $(tex) $(e); $(tex) $(e)
ifdef USEVIEWER
	$(PDFVIEWER) $(e).pdf
endif
