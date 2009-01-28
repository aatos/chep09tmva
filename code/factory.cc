#include "factory.h"

#include<algorithm>

#include <TMVA/MethodBase.h>
#include <TMVA/MsgLogger.h>

MyFactory::MyFactory(TString theJobName, TFile* theTargetFile, TString theOption):
  TMVA::Factory(theJobName, theTargetFile, theOption)
{}

void MyFactory::calculateEventEfficiency(std::vector<std::string> methods) {
  TTree *testTree = Data().GetTestTree();

  Int_t theType = 0;
  Float_t theWeight = 0;

  std::map<std::string, Float_t *> values;
  for(std::vector<std::string>::const_iterator meth = methods.begin(); meth != methods.end(); ++meth) {
    Float_t *val = new Float_t(0);
    testTree->SetBranchAddress(Form("MVA_%s", meth->c_str()), val);
    values.insert(std::make_pair(*meth, val));
  }

  testTree->SetBranchAddress("type", &theType);
  testTree->SetBranchAddress("weight", &theWeight);

  Long64_t prevSignalEvent = -1;
  Long64_t prevBkgEvent = -1;
  Long64_t entries = testTree->GetEntries();
  for(int ievt = 0; ievt<entries; ++ievt) {
    testTree->GetEntry(ievt);

    if(theType == 0) {
    }
    else {
    }

    std::cout << theType << " " << theWeight;
    for(std::map<std::string, Float_t *>::const_iterator iter = values.begin(); iter != values.end(); ++iter) {
      std::cout << "; " << iter->first << ": " << *(iter->second);
    }

    std::cout << std::endl;
  }
    
  for(std::map<std::string, Float_t *>::iterator iter = values.begin(); iter != values.end(); ++iter) {
    delete iter->second;
    iter->second = 0;
  }
}

void MyFactory::printEfficiency(std::vector<std::string> methods, double signalEventSelEff, double bkgEventSelEff,
                                long signalEntries, long bkgEntries) {
  using TMVA::MsgLogger;
  using TMVA::kINFO;
  std::vector<EffData> efficiencies;

  TTree *testTree = Data().GetTestTree();

  long signalEntriesSelected = Data().GetNEvtSigTrain() + Data().GetNEvtSigTest();
  long bkgEntriesSelected = Data().GetNEvtBkgdTrain() + Data().GetNEvtBkgdTest();
    
  double signalJetSelEff = double(signalEntriesSelected)/double(signalEntries);
  double bkgJetSelEff = double(bkgEntriesSelected)/double(bkgEntries);

  double signalPreSelEff = signalEventSelEff*signalJetSelEff;
  double bkgPreSelEff = bkgEventSelEff*bkgJetSelEff;

  double refEff = 1e-5/bkgPreSelEff;

  TString hLine = "-----------------------------------------------------------------------------";

  fLogger << kINFO 
          << Endl
          << Endl
          << " !!! WARNING:  The numbers below are pleriminary, and should not be used !!!" << Endl
          << Endl
          << "Evaluating all classifiers for signal efficiency @ 1e-5 OVERALL bkg efficiency" << Endl
          << Endl
          <<      "                                  signal   background" << Endl
          << Form("Event preselection efficiency   : %1.5f  %1.5f", signalEventSelEff, bkgEventSelEff) << Endl
          << Form("Jet preselection efficiency     : %1.5f  %1.5f", signalJetSelEff, bkgJetSelEff) << Endl
          <<      "-----------------------------------------------------" << Endl
          << Form("Overall preselection efficiency : %1.5f  %1.5f", signalPreSelEff, bkgPreSelEff) << Endl
          << Endl
          << Form("Thus the 1e-5 overall bkg efficiency corresponds %5e bkg efficiency in TMVA", refEff) << Endl
          << Endl
          << "Evaluation results ranked by best signal efficiency" << Endl
          << hLine << Endl
    //<< "MVA              Signal efficiency at bkg eff. (error):" << Endl
          << "MVA              Signal efficiency at 1e-5 overall bkg eff.:" << Endl
          << "Methods:         TMVA       with preselection" << Endl
          << hLine << Endl;

  for(std::vector<std::string>::const_iterator method_name = methods.begin(); method_name != methods.end(); ++method_name) {
    TMVA::IMethod *method_ = GetMethod(*method_name);
    TMVA::MethodBase *method = dynamic_cast<TMVA::MethodBase *>(method_);
    if(method) {
      EffData data;
      data.name = *method_name;
      data.eff = method->GetEfficiency(Form("Efficiency:%e", refEff), testTree, data.err);
      efficiencies.push_back(data);
    }
  }

  std::sort(efficiencies.begin(), efficiencies.end(), EffDataCompare());

  for(std::vector<EffData>::const_iterator data = efficiencies.begin(); data != efficiencies.end(); ++data) {
    double eff = data->eff*signalPreSelEff;
    //fLogger << kINFO << Form("%-15s: %1.3f(%02i)", data->name.c_str(), eff, int(data->err*1000)) << Endl;
    fLogger << kINFO << Form("%-15s: %1.5f     %1.5f", data->name.c_str(), data->eff, eff) << Endl;
  }
}
