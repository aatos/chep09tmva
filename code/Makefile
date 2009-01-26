EXE := chep09tmva
FILES := $(wildcard *.cc)
OBJS := $(FILES:%.cc=%.o)

INC :=
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

$(EXE): $(OBJS)
	$(CXX) $(LDFLAGS) -O $^ -o $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

ah0:
	root -l chep09tmva_aatos.C
	root -l TMVAGui.C
ah1:
	make
	./chep09tmva tmva-aatos.conf
	root -l TMVAGui.C

clean:
	rm -f $(OBJS) $(EXE)

clean_all:
	@make clean
	rm -f TMVA.root
	rm -fR plots
	rm -fR weights
	rm scripts/*.so
