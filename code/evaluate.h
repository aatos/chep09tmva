// -*- c++ -*-

#ifndef CHEP09TMVA_EVALUATE_H
#define CHEP09TMVA_EVALUATE_H

#include <TTree.h>
#include <TFile.h>
#include <TDirectory.h>
#include <TCut.h>
#include <TString.h>

#include <TMVA/TSpline1.h>
#include <TMVA/MsgLogger.h>

#include "config.h"

struct EffResult {
  std::string name;
  double eff1;
  double eff2;
  double eff3;
  double eff4;
  double eff5;
  double eff1err;
  double eff2err;
  double eff3err;
  double eff4err;
  double eff5err;
};


class MyEvaluate {
public:
  MyEvaluate(TFile *outputFile);
  virtual ~MyEvaluate();

  void setSignalTree(TTree *tree, double weight, const TCut& cut, bool isTestTree, long testEntries);
  void setBackgroundTree(TTree *tree, double weight, const TCut& cut, bool isTestTree, long testEntries);

  void setSignalEventNum(long all, long selected) {
    signalEventsAll = all;
    signalEventsPreSelected = selected;
  }
  void setBackgroundEventNum(long all, long selected) {
    bkgEventsAll = all;
    bkgEventsPreSelected = selected;
  }

  void calculateEventEfficiency(MyConfig& config);

  // This is for RootFinder
  static MyEvaluate *getThisBase() { return thisBase; }
private:
  // These are for RootFinder
  void resetThisBase() { thisBase = this; }
  static double iGetEffForRoot(double cut);
  double getEffForRoot(double cut);
  int getCutOrientation() { return fCutOrientation; }

  double getSignalEfficiency(double bkgEff, TMVA::TSpline1 *effSpl, TGraph *gr);
  double getSignalEfficiencyError(double nevents_s, double eff_s);
  void printEffResults(std::vector<EffResult>& results, bool scaleBkg=false);

  struct Data {
    TTree *tree;
    double weight;
    TString preCut;
    bool isTestTree;
    long testEntries;
  };
  Data signal;
  Data background;

  TFile *outputFile;
  TDirectory *top;

  int histoBins, rocBins;

  long signalEventsAll, signalEventsPreSelected;
  long bkgEventsAll, bkgEventsPreSelected;

  // These are for RootFinder
  static MyEvaluate *thisBase;
  TMVA::TSpline1 *sigEffSpline;
  double fXmin;
  double fXmax;
  int fCutOrientation;
  TMVA::MsgLogger fLogger;
};

#endif
