// -*- c++ -*-
#ifndef CHEP09TMVA_FACTORY_H
#define CHEP09TMVA_FACTORY_H

#include <TString.h>
#include <TFile.h>

#include <TMVA/Factory.h>

#include "config.h"

class MyFactory: public TMVA::Factory {
private:
  struct EffData {
    std::string name;
    double eff;
    double err;
  };

  struct EffDataCompare: public std::binary_function<EffData, EffData, bool> {
    bool operator()(const EffData& a, const EffData& b) const {
      return a.eff > b.eff;
    }
  };

public:
  MyFactory(TString theJobName, TFile* theTargetFile, TString theOption = "");

  virtual void printEfficiency(MyConfig& config, long signalEventsAll, long signalEventsSelected,
                               long bkgEventsAll, long bkgEventsSelected,
                               long signalEntries, long bkgEntries);

};

#endif
