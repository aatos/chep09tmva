EXE := chep09tmva
FILES := $(wildcard *.cc)
OBJS := $(FILES:%.cc=%.o)

INC :=
ROOTVERSION := $(shell root-config --version)
ROOTINC := $(shell root-config --cflags)
ROOTLIBDIR := $(shell root-config --libdir)
ROOTLIBS := $(shell root-config --glibs)
ROOTFEATURES := $(shell root-config --features)
ROOTLIBS += $(shell test -e $(ROOTLIBDIR)/libMLP.so && echo -lMLP)
ROOTLIBS += $(shell test -e $(ROOTLIBDIR)/libMinuit.so && echo -lMinuit)
ROOTLIBS += $(shell test -e $(ROOTLIBDIR)/libTreePlayer.so && echo -lTreePlayer)
ROOTLIBS += $(shell test -e $(ROOTLIBDIR)/libTMVA.so && echo -lTMVA)

CXXFLAGS := -O2 -Wall -fPIC -D_REENTRANT $(ROOTINC) $(INC)
LDFLAGS := $(ROOTLIBS)

# Targets which do not produce a file with the same name
.PHONY: all test ah0 ah1 vk clean clean_all

all: versioninfo $(EXE)
$(EXE): $(OBJS)
	$(CXX) $(LDFLAGS) -O $^ -o $@
	@echo "Program $(EXE) has been compiled using ROOT version $(ROOTVERSION)"

versioninfo:
	@echo "Using ROOT version $(ROOTVERSION)"

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

gui: all
	root -l hiptmvagui.C

test: all
	root -l menu.C
ah0:
	root -l chep09tmva_aatos.C
	root -l TMVAGui.C
ah1: all
	./$(EXE) tmva-aatos.conf
	root -l TMVAGui.C

vk: all
	./$(EXE) tmva-veikko.conf
	root -l TMVAGui.C

clean:
	rm -f $(OBJS) $(EXE)

clean_all: clean
	rm -f TMVA.root
	rm -fR plots
	rm -fR weights
	rm scripts/*.so
