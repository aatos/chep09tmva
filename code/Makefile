EXE := chep09tmva
FILES := $(wildcard *.cc)
OBJS := $(FILES:%.cc=%.o)

INC :=
ROOTINC := $(shell root-config --cflags)
ROOTLIBDIR := $(shell root-config --libdir)
ROOTLIBS := $(shell root-config --glibs)

ROOTLIBS += $(shell test -e $$ROOTSYS/lib/libMLP.so && echo -lMLP)
ROOTLIBS += $(shell test -e $$ROOTSYS/lib/libMinuit.so && echo -lMinuit)
ROOTLIBS += $(shell test -e $$ROOTSYS/lib/libTreePlayer.so && echo -lTreePlayer)
ROOTLIBS += $(shell test -e $$ROOTSYS/lib/libTMVA.so && echo -lTMVA)

CXXFLAGS := -O2 -Wall -fPIC -D_REENTRANT $(ROOTINC) $(INC)
LDFLAGS := $(ROOTLIBS)

$(EXE): $(OBJS)
	$(CXX) $(LDFLAGS) -O $^ -o $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(EXE)
