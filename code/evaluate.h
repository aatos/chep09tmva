// -*- c++ -*-

#ifndef CHEP09TMVA_EVALUATE_H
#define CHEP09TMVA_EVALUATE_H

#include <TTree.h>
#include <TFile.h>
#include <TDirectory.h>
#include <TCut.h>
#include <TString.h>

#include "config.h"

class MyEvaluate {
public:
  MyEvaluate(TFile *outputFile);
  virtual ~MyEvaluate();

  void setSignalTree(TTree *tree, double weight, const TCut& cut);
  void setBackgroundTree(TTree *tree, double weight, const TCut& cut);

  void calculateEventEfficiency(MyConfig& config);
private:
  struct Data {
    TTree *tree;
    double weight;
    TString preCut;
  };

  Data signal;
  Data background;

  TFile *outputFile;
  TDirectory *top;

  int histoBins;
};

#endif
