// -*- c++ -*-

#ifndef CHEP09TMVA_EVALUATE_H
#define CHEP09TMVA_EVALUATE_H

#include <TTree.h>
#include <TFile.h>
#include <TDirectory.h>
#include <TCut.h>
#include <TString.h>

#include <TMVA/TSpline1.h>

#include "config.h"

class MyEvaluate {
public:
  MyEvaluate(TFile *outputFile);
  virtual ~MyEvaluate();

  void setSignalTree(TTree *tree, double weight, const TCut& cut);
  void setBackgroundTree(TTree *tree, double weight, const TCut& cut);

  void calculateEventEfficiency(MyConfig& config);

  // This is for RootFinder
  static MyEvaluate *getThisBase() { return thisBase; }
private:
  // These are for RootFinder
  void resetThisBase() { thisBase = this; }
  static double iGetEffForRoot(double cut);
  double getEffForRoot(double cut);
  int getCutOrientation() { return fCutOrientation; }

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
  int rocBins;

  // These are for RootFinder
  static MyEvaluate *thisBase;
  TMVA::TSpline1 *sigEffSpline;
  double fXmin;
  double fXmax;
  int fCutOrientation;
};

#endif
