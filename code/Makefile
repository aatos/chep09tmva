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

clean:
	rm -f $(OBJS) $(EXE)

go:
	root chep09tmva_aatos.C

