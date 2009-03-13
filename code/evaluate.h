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
#include "output.h"
#include "timer.h"

struct EffResult {
  std::string name;
  double eff1;
  double eff2;
  double eff3;
  double eff4;
  double eff5;
  double eff6;
  double eff1err;
  double eff2err;
  double eff3err;
  double eff4err;
  double eff5err;
  double eff6err;
};


class MyEvaluate {
public:
  MyEvaluate(TFile *outputFile, int rocbins=100);
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

  void calculateEventEfficiency(MyConfig& config, MyOutput& output, std::vector<std::pair<std::string, TimerData> >& timer_vector);

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
  void printEffResults(std::vector<EffResult>& results, MyOutput& output, std::string column5, std::string column6, bool scaleBkg=false);

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
