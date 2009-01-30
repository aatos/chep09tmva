#include "factory.h"

#include<algorithm>

#include <TMVA/MethodBase.h>
#include <TMVA/MsgLogger.h>

MyFactory::MyFactory(TString theJobName, TFile* theTargetFile, TString theOption):
  TMVA::Factory(theJobName, theTargetFile, theOption)
{}

void MyFactory::printEfficiency(MyConfig& config, long signalEventsAll, long signalEventsSelected,
                                long bkgEventsAll, long bkgEventsSelected,
                                long signalEntries, long bkgEntries) {
  using TMVA::MsgLogger;
  using TMVA::kINFO;
  std::vector<EffData> efficiencies;

  TTree *testTree = Data().GetTestTree();

  long signalEntriesSelected = Data().GetNEvtSigTrain() + Data().GetNEvtSigTest();
  long bkgEntriesSelected = Data().GetNEvtBkgdTrain() + Data().GetNEvtBkgdTest();
    
  double signalEventSelEff = double(signalEventsSelected)/double(signalEventsAll);
  double bkgEventSelEff = double(bkgEventsSelected)/double(bkgEventsAll);

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
          << "Evaluating all classifiers for signal jet efficiency @ 1e-5 OVERALL bkg jet efficiency" << Endl
          << Endl
          <<      "                                             signal   background" << Endl
          << Form("Event preselection efficiency              : %1.5f  %1.5f", signalEventSelEff, bkgEventSelEff) << Endl
          << Form("Jet   preselection efficiency              : %1.5f  %1.5f", signalJetSelEff, bkgJetSelEff) << Endl
          <<      "-------------------------------------------------------------" << Endl
          << Form("Overall jet preselection efficiency approx : %1.5f  %1.5f", signalPreSelEff, bkgPreSelEff) << Endl
          << Endl
          << Form("Thus the 1e-5 overall bkg jet efficiency corresponds %5e bkg jet efficiency in TMVA", refEff) << Endl
          << Endl
          << "Evaluation results ranked by best signal jet efficiency" << Endl
          << hLine << Endl
    //<< "MVA              Signal efficiency at bkg eff. (error):" << Endl
          << "MVA              Signal jet efficiency at 1e-5 overall bkg jet eff.:" << Endl
          << "Methods:         TMVA        with preselection" << Endl
          << hLine << Endl;

  for(std::map<std::string, std::string>::const_iterator iter = config.classifiers.begin(); iter != config.classifiers.end(); ++iter) {
    TMVA::IMethod *method_ = GetMethod(iter->first);
    TMVA::MethodBase *method = dynamic_cast<TMVA::MethodBase *>(method_);
    if(method) {
      EffData data;
      data.name = iter->first;
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
  fLogger << kINFO << hLine << Endl;
}
