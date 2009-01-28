// -*- c++ -*-
#ifndef CHEP09TMVA_FACTORY_H
#define CHEP09TMVA_FACTORY_H

#include<string>
#include<vector>

#include <TString.h>
#include <TFile.h>

#include <TMVA/Factory.h>

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

  virtual void calculateEventEfficiency(std::vector<std::string> methods);
  virtual void printEfficiency(std::vector<std::string> methods, double signalEventSelEff, double bkgEventSelEff,
                               long signalEntries, long bkgEntries);

};

#endif
