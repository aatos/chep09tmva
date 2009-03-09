#include "evaluate.h"

#include<algorithm>
//#include<iomanip>

#include <TTreeFormula.h>
#include <TMath.h>

#include <TMVA/Reader.h>
#include <TMVA/Tools.h>
#include <TMVA/RootFinder.h>
#include <TMVA/Config.h>

using TMVA::MsgLogger;
using TMVA::kINFO;
using TMVA::kWARNING;
using TMVA::kERROR;
using TMVA::kFATAL;

struct MvaData {
  TH1 *histoS;
  TH1 *histoB;
  TH1 *effS;
  TH1 *effB;
};

struct CutData {
  std::string name;
  TH1 *signalEff;
  TH1 *bkgEff;
  int rocBins;
  char *hasBeenFilled;
};

struct EffResultCompare: public std::binary_function<EffResult, EffResult, bool> {
  bool operator()(const EffResult& a, const EffResult& b) const {
    return a.eff5 > b.eff5;
  }
};

struct EffPairCompare: public std::binary_function<std::pair<double, double>, std::pair<double, double>, bool> {
  bool operator()(const std::pair<double, double>& a, const std::pair<double, double>& b) const {
    return a.first < b.first;
  }
};

TString hLine = "-----------------------------------------------------------------------------";

MyEvaluate *MyEvaluate::thisBase = 0;

MyEvaluate::MyEvaluate(TFile *file, int rbins):
  outputFile(file),
  top(0),
  histoBins(200000),
  rocBins(rbins),
  fLogger(std::string("MyEvaluate")) {

  top = file->mkdir("MyEvaluate");
}

MyEvaluate::~MyEvaluate() {
  outputFile->Write();
  outputFile->Close();
}

void MyEvaluate::setSignalTree(TTree *tree, double weight, const TCut& cut, bool isTest, long testEntries) {
  signal.tree = tree;
  signal.weight = weight;
  signal.preCut = cut.GetTitle();
  signal.isTestTree = isTest;
  signal.testEntries = testEntries;
}

void MyEvaluate::setBackgroundTree(TTree *tree, double weight, const TCut& cut, bool isTest, long testEntries) {
  background.tree = tree;
  background.weight = weight;
  background.preCut = cut.GetTitle();
  background.isTestTree = isTest;
  background.testEntries = testEntries;
}

void MyEvaluate::calculateEventEfficiency(MyConfig& config, MyOutput& csvOutput) {
  fLogger << kINFO << Endl
          << Endl
          << "Testing classifiers in order to obtain signal/background efficiencies for EVENTS" << Endl
          << "(not jets, which are the input to TMVA)" << Endl;

  if(!signal.isTestTree) {
    fLogger << kWARNING << Endl
            << "Signal tree has been (partially) used for training" << Endl
            << "  => signal event efficiencies below are biased" << Endl
            << Endl;
  }
  if(!signal.isTestTree) {
    fLogger << kWARNING << Endl
            << "Background tree has been (partially) used for training" << Endl
            << "  => background efficiencies below are biased" << Endl
            << Endl;
  }

  std::map<std::string, int> cutOrientation;

  // Look for the cut orientation for each classifier
  // Separate classifiers to cuts and non-cuts classes
  top->cd();
  std::vector<std::string> classifiersNoCuts;
  //std::vector<std::string> classifiersCuts;
  std::vector<CutData> cutData;
  for(std::map<std::string, std::string>::const_iterator method = config.classifiers.begin(); method != config.classifiers.end(); ++method) {
    std::string mvaType = stripType(method->first);
    const char *mvaName = method->first.c_str();
    TString format("");
    if(mvaType == "Cuts") {
      //classifiersCuts.push_back(method->first);
      format.Form("Method_Cuts/%s/MVA_%s_effBvsS", mvaName, mvaName);
      TH1 *histo = dynamic_cast<TH1 *>(outputFile->Get(format));
      if(!histo) {
        fLogger << kERROR << "Unable to find histogram " << format << Endl;
        return;
      }

      CutData data;
      data.name = method->first;
      data.rocBins = histo->GetNbinsX();
      data.signalEff = new TH1F(Form("MyMVA_%s_Seff", mvaName), "Signal event eff at signal jet eff", data.rocBins, 0, 1);
      data.bkgEff = new TH1F(Form("MyMVA_%s_Beff", mvaName), "Bkg event eff at signal jet eff", data.rocBins, 0, 1);
      data.hasBeenFilled = new char[data.rocBins];
      memset(data.hasBeenFilled, 0, data.rocBins);
      cutData.push_back(data);
    }
    else {
      classifiersNoCuts.push_back(method->first);

      format.Form("Method_%s/%s/MVA_%s_S", mvaType.c_str(), method->first.c_str(), method->first.c_str());
      TH1 *histoS = dynamic_cast<TH1 *>(outputFile->Get(format));
      if(!histoS) {
        fLogger << kERROR << "Unable to find histogram " << format << Endl;
        return;
      }
      format.Form("Method_%s/%s/MVA_%s_B", mvaType.c_str(), method->first.c_str(), method->first.c_str());
      TH1 *histoB = dynamic_cast<TH1 *>(outputFile->Get(format));
      if(!histoB) {
        fLogger << kERROR << "Unable to find histogram " << format << Endl;
        return;
      }

      cutOrientation[method->first] = (histoS->GetMean() > histoB->GetMean()) ? +1 : -1;
    }
  }  

  if(classifiersNoCuts.size() == 0 && cutData.size() == 0) {
    fLogger << kWARNING << "No classifiers left, not evaluating." << Endl;
    return;
  }

  // Create TTreeFormula objects corresponding to the preselection cuts
  TTreeFormula *sigCut = 0;
  if(signal.preCut.Length() > 0) {
    sigCut = new TTreeFormula("SigCut", signal.preCut, signal.tree);
    if(sigCut->GetNcodes() == 0) {
      fLogger << kERROR << "Signal cut '" << signal.preCut << "' does not depend on any TTree variable in TTree "
              << signal.tree->GetName() << Endl;
      return;
    }
  }
  TTreeFormula *bkgCut = 0;
  if(background.preCut.Length() > 0) {
    bkgCut = new TTreeFormula("BkgCut", background.preCut, background.tree);
    if(bkgCut->GetNcodes() == 0) {
      fLogger << kERROR << "Background cut '" << background.preCut << "' does not depend on any TTree variable in TTree "
              << background.tree->GetName() << Endl;
      delete sigCut;
      return;
    }
  }


  // Initialize TMVA Reader
  TMVA::Reader *reader = new TMVA::Reader("Silent");
  std::vector<float *> readerVariables;
  int nvariables = config.variables.size();

  for(std::vector<std::string>::const_iterator var = config.variables.begin(); var != config.variables.end(); ++var) {
    float *rvar = new float(0);
    reader->AddVariable(var->c_str(), rvar);
    readerVariables.push_back(rvar);
  }


  // Initialize temporary TTree for MVA outputs
  int type = -1;
  TTree *mvaOutput = new TTree("mvaOutput", "mvaOutput");
  mvaOutput->SetDirectory(0);
  mvaOutput->Branch("type", &type, "type/I");

  // Book MVA methods on reader and set branches for MVA values
  std::map<std::string, float *> mvaValues;
  for(std::vector<std::string>::const_iterator method = classifiersNoCuts.begin(); method != classifiersNoCuts.end(); ++method) {
    const char *mvaName = method->c_str();
    reader->BookMVA(mvaName, Form("weights/chep09tmva_%s.weights.txt", mvaName));

    float *mva = new float(0);
    mvaValues.insert(std::make_pair(*method, mva));
    mvaOutput->Branch(mvaName, mva, Form("%s/F", mvaName));
  }
  for(std::vector<CutData>::iterator iter = cutData.begin(); iter != cutData.end(); ++iter) {
    const char *mvaName = iter->name.c_str();
    reader->BookMVA(mvaName, Form("weights/chep09tmva_%s.weights.txt", mvaName));
  }

  // Compute MVA outputs for all entries (=jets) in the input TTrees
  // (for both signal and background)
  long njets[2] = {0, 0};
  long njets_passed[2] = {0, 0};
  long nevents_passed[2] = {0, 0};
  for(type=0; type<2; ++type) {
    TTree *tree = type ? signal.tree : background.tree;

    // Create TTreeFormula objects corresponding to the variables
    // given to the classifiers
    std::vector<TTreeFormula *> treeFormulas;
    for(std::vector<std::string>::const_iterator var = config.variables.begin(); var != config.variables.end(); ++var) {
      TTreeFormula *ttf = new TTreeFormula(Form("Formula_%s", var->c_str()), var->c_str(), tree);
      treeFormulas.push_back(ttf);
    }

    TTreeFormula *preCut = type ? sigCut : bkgCut;

    int prevEvent = -1;
    int prevRun = -1;
    int currentEvent = 0;
    int currentRun = 0;
    bool fillStarted = false;
    tree->SetBranchStatus("evt", 1);
    tree->SetBranchStatus("run", 1);
    tree->SetBranchAddress("evt", &currentEvent);
    tree->SetBranchAddress("run", &currentRun);

    // Iterate over background/signal TTree
    njets[type] = tree->GetEntries();
    long confMaxEntries = type ? signal.testEntries : background.testEntries;
    if(confMaxEntries <= 0)
      confMaxEntries = njets[type];
    for(Long64_t ientry = 0; ientry < njets[type]; ++ientry) {
      tree->GetEntry(ientry);

      // Check if the entry passes the preselection cut
      if(preCut) {
        //Int_t ndata = preCut->GetNdata();
        Float_t cutVal = preCut->EvalInstance(0);
        if(cutVal < 0.5) {
          //std::cout << "Taking out entry " << ientry << " in event " << currentEvent << ", run " << currentRun << std::endl;
          continue;
        }
      }
      ++njets_passed[type];


      // Get variable values
      for(int i=0; i<nvariables; ++i) {
        *(readerVariables[i]) = treeFormulas[i]->EvalInstance(0);
      }

      /**
       * Compute MVA output, and fill tree if necessary
       *
       * We're computing the efficiencies in per-event basis, so for
       * each event we want one entry in the mvaOutput tree. The
       * mvaOutput tree is filled for entry n when processing entry
       * n+1. For this reason, at the first iteration the mvaOutput
       * tree is not filled, and the tree is filled explicitly after
       * the entry loop has finished.
       *
       * Why the filling of mvaOutput is done like this? Because we
       * check if the jet (in entry n) is unique in an event in entry
       * n+1 (this also holds for checking if event has changed).
       */
      if(currentEvent != prevEvent || currentRun != prevRun) {
        if(fillStarted) {
          mvaOutput->Fill();
          ++nevents_passed[type];
        }
        else
          fillStarted = true;

        for(std::map<std::string, float *>::const_iterator iter = mvaValues.begin(); iter != mvaValues.end(); ++iter) {
          *(iter->second) = float(reader->EvaluateMVA(iter->first.c_str())), *(iter->second);
        }
      }
      else {
        for(std::map<std::string, float *>::const_iterator iter = mvaValues.begin(); iter != mvaValues.end(); ++iter) {
          float mvaVal = reader->EvaluateMVA(iter->first.c_str());
          if(cutOrientation[iter->first] > 0) 
            *(iter->second) = std::max(mvaVal, *(iter->second)); 
          else
            *(iter->second) = std::min(mvaVal, *(iter->second)); 
        }
      }

      /**
       * Compute Cuts efficiencies for *both* signal and background 
       */
      if(currentEvent != prevEvent || currentRun != prevRun) {
        for(std::vector<CutData>::iterator iter = cutData.begin(); iter != cutData.end(); ++iter) {
          memset(iter->hasBeenFilled, 0, iter->rocBins);
        }
      }
      for(std::vector<CutData>::iterator iter = cutData.begin(); iter != cutData.end(); ++iter) {
        const char *cutName = iter->name.c_str();
        TH1 *histo = type ? iter->signalEff : iter->bkgEff;
        for(int bin=0; bin < iter->rocBins; ++bin) {
          float pass = reader->EvaluateMVA(cutName, float(bin)/iter->rocBins);
          if(pass > 0.5 && iter->hasBeenFilled[bin] == 0) {
            histo->AddBinContent(bin+1, 1);
            iter->hasBeenFilled[bin] = 1;
          }
        }
      }

      prevEvent = currentEvent;
      prevRun = currentRun;
      if(njets_passed[type] >= confMaxEntries)
        break;
    }
    // Filling of the results of the last entry/event
    mvaOutput->Fill();
    ++nevents_passed[type];

    for(int i=0; i<nvariables; ++i) {
      delete treeFormulas[i];
    }
  }

  TMVA::gConfig().SetSilent(kFALSE);
  double signalEventPreSeleEff = double(signalEventsPreSelected)/double(signalEventsAll);
  double bkgEventPreSeleEff = double(bkgEventsPreSelected)/double(bkgEventsAll);

  double signalEventTmvaSeleEff = double(nevents_passed[1])/double(signalEventsPreSelected);
  double bkgEventTmvaSeleEff = double(nevents_passed[0])/double(bkgEventsPreSelected);

  double signalEventOverallEff = double(nevents_passed[1])/double(signalEventsAll);
  double bkgEventOverallEff = double(nevents_passed[0])/double(bkgEventsAll);

  double signalJetTmvaSeleEff = double(njets_passed[1])/double(njets[1]);
  double bkgJetTmvaSeleEff = double(njets_passed[0])/double(njets[0]);

  if(std::find(config.reports.begin(), config.reports.end(), "EventPreSelections") != config.reports.end()) {
    fLogger << kINFO << Endl
            << hLine << Endl
            <<      "             |             Events             |               Jets    " << Endl
            <<      "             |   Signal   eff       Bkg   eff |   Signal  eff       Bkg  eff" << Endl
            << hLine << Endl
            << Form("Generated    : %8ld        %8ld       |      N/A            N/A", signalEventsAll, bkgEventsAll) << Endl
            << Form("Event presel : %8ld %5.3f  %8ld %5.3f | %8ld       %8ld",
                    signalEventsPreSelected, signalEventPreSeleEff,
                    bkgEventsPreSelected, bkgEventPreSeleEff,
                    njets[1], njets[0]) << Endl
            << Form("TMVA  presel : %8ld %5.3f  %8ld %5.3f | %8ld %4.2f  %8ld %4.2f",
                    nevents_passed[1], signalEventTmvaSeleEff,
                    nevents_passed[0], bkgEventTmvaSeleEff,
                    njets_passed[1], signalJetTmvaSeleEff,
                    njets_passed[0], bkgJetTmvaSeleEff) << Endl
            << Form("Total presel :          %5.3f           %5.3f |", signalEventOverallEff, bkgEventOverallEff) << Endl
            << hLine << Endl;
    
    fLogger << kINFO << Endl
            << Form("Thus 1e-5 overall bkg event efficiency corresponds to %1.1e bkg event",  1e-5/bkgEventOverallEff) << Endl
            << "efficiency by TMVA." << Endl;
  }

  // Create efficiency histograms
  top->cd();
  std::map<std::string, MvaData> dataMap;
  for(std::map<std::string, float *>::iterator iter = mvaValues.begin(); iter != mvaValues.end(); ++iter) {
    MvaData data;
    const char *mvaName = iter->first.c_str();
    double xmin = mvaOutput->GetMinimum(mvaName);
    double xmax = mvaOutput->GetMaximum(mvaName);

    data.histoS = new TH1F(Form("MyMVA_%s_S", mvaName), "MVA distribution for signal events", histoBins, xmin, xmax);
    data.histoB = new TH1F(Form("MyMVA_%s_B", mvaName), "MVA distribution for background events", histoBins, xmin, xmax);
    data.effS = new TH1F(Form("MyMVA_%s_effS", mvaName), "MVA efficiency for signal events", histoBins, xmin, xmax);
    data.effB = new TH1F(Form("MyMVA_%s_effB", mvaName), "MVA efficiency for background events", histoBins, xmin, xmax);

    dataMap[iter->first] = data;
  }

  // Fill efficiency histograms
  Long64_t entries = mvaOutput->GetEntries();
  for(Long64_t ientry=0; ientry < entries; ++ientry) {
    mvaOutput->GetEntry(ientry);

    for(std::map<std::string, float *>::iterator iter = mvaValues.begin(); iter != mvaValues.end(); ++iter) {
      MvaData& data = dataMap[iter->first];
      float mvaValue = *(iter->second);

      TH1* histo = type ? data.histoS : data.histoB;
      TH1* eff = type ? data.effS : data.effB;

      histo->Fill(mvaValue);
      
      int sign = cutOrientation[iter->first];
      int maxbin = histo->GetXaxis()->FindBin(mvaValue);
      if(sign > 0) {
        if(maxbin > histoBins)
          continue;
        else if(maxbin < 1)
          maxbin = 1;

        for(int bin=1; bin <= maxbin; ++bin) {
          eff->AddBinContent(bin, 1);
        }
      }
      else {
        if(maxbin < 1)
          continue;
        else if(maxbin > histoBins)
          maxbin = histoBins;
        for(int bin=maxbin+1; bin <= histoBins; ++bin) {
          eff->AddBinContent(bin, 1);
        }
      }
    }
  }

  // Normalize efficiency histograms, compute ROC and signal
  // efficiencies at various background efficiency levels
  std::vector<EffResult> efficiencies;
  std::vector<EffResult> efficienciesBkgScaled;
  std::vector<EffResult> efficienciesAllScaled;
  for(std::map<std::string, MvaData>::iterator iter = dataMap.begin(); iter != dataMap.end(); ++iter) {
    const char *mvaName = iter->first.c_str();
    MvaData& data = iter->second;

    // Normalize efficiency histograms to maximum
    float maximum = data.effS->GetMaximum();
    data.effS->Scale(1.0 / (maximum > 0 ? maximum : 1));
    maximum = data.effB->GetMaximum();
    data.effB->Scale(1.0 / (maximum > 0 ? maximum : 1));

    // Compute ROC
    TH1 *effBvsS = new TH1F(Form("MyMVA_%s_effBvsS", mvaName), "B efficiency vs. S", rocBins, 0, 1);
    effBvsS->SetXTitle("signal eff");
    effBvsS->SetYTitle("backgr eff");
    TH1 *rejBvsS = new TH1F(Form("MyMVA_%s_rejBvsS", mvaName), "B rejection vs. S", rocBins, 0, 1);
    rejBvsS->SetXTitle("signal eff");
    rejBvsS->SetYTitle("backgr rejection (1-eff)");
    
    TGraph *grS = new TGraph(data.effS);
    TGraph *grB = new TGraph(data.effB);
    TMVA::TSpline1 *splS = new TMVA::TSpline1("spline_signal", grS);
    TMVA::TSpline1 *splB = new TMVA::TSpline1("spline_background", grB);

    TMVA::gTools().CheckSplines(data.effS, splS);
    TMVA::gTools().CheckSplines(data.effB, splB);

    fXmin = data.effS->GetXaxis()->GetXmin();
    fXmax = data.effS->GetXaxis()->GetXmax();
    fCutOrientation = cutOrientation[iter->first];
    sigEffSpline = splS;
    resetThisBase();
    TMVA::RootFinder rootFinder(&iGetEffForRoot, fXmin, fXmax);
    for(int bin=1; bin <= rocBins; ++bin) {
      double seff = effBvsS->GetBinCenter(bin);
      double cut = rootFinder.Root(seff);
      double beff = splB->Eval(cut);

      effBvsS->SetBinContent(bin, beff);
      rejBvsS->SetBinContent(bin, 1-beff);
    }

    TGraph *effBvsSgr = new TGraph(effBvsS);
    effBvsSgr->SetMaximum(effBvsS->GetMaximum());
    effBvsSgr->SetMinimum(effBvsS->GetMinimum());
    TMVA::TSpline1 *splEff = new TMVA::TSpline1("effBvsSspl", effBvsSgr);

    EffResult res;
    res.name = iter->first;
    res.eff1 = getSignalEfficiency(0.1 , splEff, effBvsSgr);
    res.eff2 = getSignalEfficiency(0.01, splEff, effBvsSgr);
    res.eff3 = getSignalEfficiency(1e-3, splEff, effBvsSgr);
    res.eff4 = getSignalEfficiency(1e-4, splEff, effBvsSgr);
    res.eff5 = getSignalEfficiency(1e-5, splEff, effBvsSgr);
    res.eff6 = getSignalEfficiency(1e-6, splEff, effBvsSgr);
    res.eff1err = getSignalEfficiencyError(nevents_passed[1], res.eff1);
    res.eff2err = getSignalEfficiencyError(nevents_passed[1], res.eff2);
    res.eff3err = getSignalEfficiencyError(nevents_passed[1], res.eff3);
    res.eff4err = getSignalEfficiencyError(nevents_passed[1], res.eff4);
    res.eff5err = getSignalEfficiencyError(nevents_passed[1], res.eff5);
    res.eff6err = getSignalEfficiencyError(nevents_passed[1], res.eff6);
    efficiencies.push_back(res);

    res.eff1 = getSignalEfficiency(0.1 /bkgEventOverallEff, splEff, effBvsSgr);
    res.eff2 = getSignalEfficiency(0.01/bkgEventOverallEff, splEff, effBvsSgr);
    res.eff3 = getSignalEfficiency(1e-3/bkgEventOverallEff, splEff, effBvsSgr);
    res.eff4 = getSignalEfficiency(1e-4/bkgEventOverallEff, splEff, effBvsSgr);
    res.eff5 = getSignalEfficiency(1e-5/bkgEventOverallEff, splEff, effBvsSgr);
    res.eff6 = getSignalEfficiency(1e-6/bkgEventOverallEff, splEff, effBvsSgr);
    res.eff1err = getSignalEfficiencyError(nevents_passed[1], res.eff1);
    res.eff2err = getSignalEfficiencyError(nevents_passed[1], res.eff2);
    res.eff3err = getSignalEfficiencyError(nevents_passed[1], res.eff3);
    res.eff4err = getSignalEfficiencyError(nevents_passed[1], res.eff4);
    res.eff5err = getSignalEfficiencyError(nevents_passed[1], res.eff5);
    res.eff6err = getSignalEfficiencyError(nevents_passed[1], res.eff6);
    efficienciesBkgScaled.push_back(res);

    res.eff1 *= signalEventOverallEff;
    res.eff2 *= signalEventOverallEff;
    res.eff3 *= signalEventOverallEff;
    res.eff4 *= signalEventOverallEff;
    res.eff5 *= signalEventOverallEff;
    res.eff6 *= signalEventOverallEff;
    res.eff1err = getSignalEfficiencyError(nevents_passed[1], res.eff1);
    res.eff2err = getSignalEfficiencyError(nevents_passed[1], res.eff2);
    res.eff3err = getSignalEfficiencyError(nevents_passed[1], res.eff3);
    res.eff4err = getSignalEfficiencyError(nevents_passed[1], res.eff4);
    res.eff5err = getSignalEfficiencyError(nevents_passed[1], res.eff5);
    res.eff6err = getSignalEfficiencyError(nevents_passed[1], res.eff6);
    efficienciesAllScaled.push_back(res);

    sigEffSpline=0;
    delete splEff;
    delete effBvsSgr;
    delete splS;
    delete splB;
    delete grS;
    delete grB;
  }

  // Cuts
  for(std::vector<CutData>::iterator iter = cutData.begin(); iter != cutData.end(); ++iter) {
    // Normalize histograms
    iter->signalEff->Scale(nevents_passed[1] > 0 ? (1.0/nevents_passed[1]) : 1.0);
    iter->bkgEff->Scale(   nevents_passed[0] > 0 ? (1.0/nevents_passed[0]) : 1.0);

    const char *cutName = iter->name.c_str();
    std::vector<std::pair<double, double> > effBvsS;
    for(int bin=1; bin <= iter->rocBins; ++bin) {
      double effS = iter->signalEff->GetBinContent(bin);
      double effB = iter->bkgEff->GetBinContent(bin);
      if(effS > 1e-10)
        effBvsS.push_back(std::make_pair(effS, effB));
    }
    std::sort(effBvsS.begin(), effBvsS.end(), EffPairCompare());
    double *effS = new double[effBvsS.size()];
    double *effB = new double[effBvsS.size()];
    double *rejB = new double[effBvsS.size()];
    double maxEffB = -9999;
    double minEffB = 9999;
    for(unsigned int i=0; i<effBvsS.size(); ++i) {
      effS[i] = effBvsS[i].first;
      effB[i] = effBvsS[i].second;
      rejB[i] = 1.0-effB[i];
      maxEffB = std::max(maxEffB, effB[i]);
      minEffB = std::min(minEffB, effB[i]);
    }
    TGraph *grEffBvsS = new TGraph(effBvsS.size(), effS, effB);
    grEffBvsS->SetMaximum(maxEffB);
    grEffBvsS->SetMinimum(minEffB);
    grEffBvsS->SetName(Form("MyMVA_%s_effBvsS", cutName));
    grEffBvsS->SetTitle("B efficiency vs. S");
    grEffBvsS->Write();

    TGraph *grRejBvsS = new TGraph(effBvsS.size(), effS, rejB);
    grRejBvsS->SetName(Form("MyMVA_%s_rejBvsS", cutName));
    grRejBvsS->SetTitle("B rejection vs. S");
    grRejBvsS->Write();

    TMVA::TSpline1 *splEff = new TMVA::TSpline1("effBvsSspl", grEffBvsS);
    EffResult res;
    res.name = iter->name;
    res.eff1 = getSignalEfficiency(0.1 , splEff, grEffBvsS);
    res.eff2 = getSignalEfficiency(0.01, splEff, grEffBvsS);
    res.eff3 = getSignalEfficiency(1e-3, splEff, grEffBvsS);
    res.eff4 = getSignalEfficiency(1e-4, splEff, grEffBvsS);
    res.eff5 = getSignalEfficiency(1e-5, splEff, grEffBvsS);
    res.eff6 = getSignalEfficiency(1e-6, splEff, grEffBvsS);
    res.eff1err = getSignalEfficiencyError(nevents_passed[1], res.eff1);
    res.eff2err = getSignalEfficiencyError(nevents_passed[1], res.eff2);
    res.eff3err = getSignalEfficiencyError(nevents_passed[1], res.eff3);
    res.eff4err = getSignalEfficiencyError(nevents_passed[1], res.eff4);
    res.eff5err = getSignalEfficiencyError(nevents_passed[1], res.eff5);
    res.eff6err = getSignalEfficiencyError(nevents_passed[1], res.eff6);
    efficiencies.push_back(res);

    res.eff1 = getSignalEfficiency(0.1 /bkgEventOverallEff, splEff, grEffBvsS);
    res.eff2 = getSignalEfficiency(0.01/bkgEventOverallEff, splEff, grEffBvsS);
    res.eff3 = getSignalEfficiency(1e-3/bkgEventOverallEff, splEff, grEffBvsS);
    res.eff4 = getSignalEfficiency(1e-4/bkgEventOverallEff, splEff, grEffBvsS);
    res.eff5 = getSignalEfficiency(1e-5/bkgEventOverallEff, splEff, grEffBvsS);
    res.eff6 = getSignalEfficiency(1e-6/bkgEventOverallEff, splEff, grEffBvsS);
    res.eff1err = getSignalEfficiencyError(nevents_passed[1], res.eff1);
    res.eff2err = getSignalEfficiencyError(nevents_passed[1], res.eff2);
    res.eff3err = getSignalEfficiencyError(nevents_passed[1], res.eff3);
    res.eff4err = getSignalEfficiencyError(nevents_passed[1], res.eff4);
    res.eff5err = getSignalEfficiencyError(nevents_passed[1], res.eff5);
    res.eff6err = getSignalEfficiencyError(nevents_passed[1], res.eff6);
    efficienciesBkgScaled.push_back(res);

    res.eff1 *= signalEventOverallEff;
    res.eff2 *= signalEventOverallEff;
    res.eff3 *= signalEventOverallEff;
    res.eff4 *= signalEventOverallEff;
    res.eff5 *= signalEventOverallEff;
    res.eff6 *= signalEventOverallEff;
    res.eff1err = getSignalEfficiencyError(nevents_passed[1], res.eff1);
    res.eff2err = getSignalEfficiencyError(nevents_passed[1], res.eff2);
    res.eff3err = getSignalEfficiencyError(nevents_passed[1], res.eff3);
    res.eff4err = getSignalEfficiencyError(nevents_passed[1], res.eff4);
    res.eff5err = getSignalEfficiencyError(nevents_passed[1], res.eff5);
    res.eff6err = getSignalEfficiencyError(nevents_passed[1], res.eff6);
    efficienciesAllScaled.push_back(res);

    sigEffSpline=0;
    delete splEff;
    delete effS;
    delete effB;
    delete rejB;
  }


  std::sort(efficiencies.begin(), efficiencies.end(), EffResultCompare());
  std::sort(efficienciesBkgScaled.begin(), efficienciesBkgScaled.end(), EffResultCompare());
  std::sort(efficienciesAllScaled.begin(), efficienciesAllScaled.end(), EffResultCompare());

  if(std::find(config.reports.begin(), config.reports.end(), "EventEfficienciesTMVA") != config.reports.end()) {
    fLogger << kINFO << Endl
            << "For comparison with the jet efficiency numbers reported by TMVA" << Endl
            << "Signal and background event efficiencies have NOT been scaled with preselections" << Endl;
    printEffResults(efficiencies, csvOutput, "eventEffTmva_5", "eventEffTmva_6");
    csvOutput.setComment("eventEffTmva_5", "signal event efficiency at 1e-5 bkg event efficiency, both as given by TMVA (i.e. no scaling with preselection efficiencies");
    csvOutput.setComment("eventEffTmva_6", "signal event efficiency at 1e-6 bkg event efficiency, both as given by TMVA (i.e. no scaling with preselection efficiencies");
  }
  if(std::find(config.reports.begin(), config.reports.end(), "EventEfficienciesBkgScaled") != config.reports.end()) {
    fLogger << kINFO << Endl
            << "Background event efficiency has been scaled with the preselection bkg event" << Endl
            << "efficiency (signal event efficiency is as given by TMVA)" << Endl;
    printEffResults(efficienciesBkgScaled, csvOutput, "eventEffBkgScaled_5", "eventEffBkgScaled_6", true);
    csvOutput.setComment("eventEffBkgScaled_5", "signal event efficiency at 1e-5 bkg event efficiency, bkg efficiency scaled with preselection efficiency, signal efficiency as given by TMVA");
    csvOutput.setComment("eventEffBkgScaled_6", "signal event efficiency at 1e-6 bkg event efficiency, bkg efficiency scaled with preselection efficiency, signal efficiency as given by TMVA");
  }
  if(std::find(config.reports.begin(), config.reports.end(), "EventEfficienciesAllScaled") != config.reports.end()) {
    fLogger << kINFO << Endl 
            << "Signal and background event efficiencies have been scaled with the preselection" << Endl
            << "event efficiencies" << Endl;
    printEffResults(efficienciesAllScaled, csvOutput, "eventEffScaled_5", "eventEffScaled_6", true);
    csvOutput.setComment("eventEffScaled_5", "signal event efficiency at 1e-5 bkg event efficiency, both scaled with preselection efficiencies");
    csvOutput.setComment("eventEffScaled_6", "signal event efficiency at 1e-6 bkg event efficiency, both scaled with preselection efficiencies");
  }

  fLogger << kWARNING << " !!!  This module is still experimental  !!!" << Endl;

  for(std::map<std::string, float *>::iterator iter = mvaValues.begin(); iter != mvaValues.end(); ++iter) {
    delete iter->second;
  }

  delete mvaOutput;  

  for(int i=0; i<nvariables; ++i) {
    delete readerVariables[i];
  }

  delete sigCut;
  delete bkgCut;
  delete reader;
}

double MyEvaluate::iGetEffForRoot(double cut) {
  return MyEvaluate::getThisBase()->getEffForRoot(cut);
}

double MyEvaluate::getEffForRoot(double cut) {
  double retval = sigEffSpline->Eval(cut);

  // This is directly from TMVAs MethodBase.cxx
   // caution: here we take some "forbidden" action to hide a problem:
   // in some cases, in particular for likelihood, the binned efficiency distributions
   // do not equal 1, at xmin, and 0 at xmax; of course, in principle we have the
   // unbinned information available in the trees, but the unbinned minimization is
   // too slow, and we don't need to do a precision measurement here. Hence, we force
   // this property.
  Double_t eps = 1.0e-5;
  if      (cut-fXmin < eps) retval = (getCutOrientation() > 0) ? 1.0 : 0.0;
  else if (fXmax-cut < eps) retval = (getCutOrientation() > 0) ? 0.0 : 1.0;

  return retval;
}

double MyEvaluate::getSignalEfficiency(double bkgEff, TMVA::TSpline1 *effSpl, TGraph *gr) {
  double effS = 0;
  double effB = 0;
  double effS_ = 0;
  double effB_ = 0;
  double Smin=0, Smax=0, temp=0;
  bool found = false;

  gr->GetPoint(0, Smin, temp);
  gr->GetPoint(gr->GetN()-1, Smax, effB_);
  //std::cout << xmin << " " << xmax << std::endl;
  //std::cout << "gr max " << std::setprecision(10) << gr->GetMaximum() << " gr min " << std::setprecision(10) << gr->GetMinimum() << std::endl;

  // If requested bkg efficiency is greater than exists in the effBvsS
  // curve, all signal events are accepted for that bkg eff and thus
  // the signal efficiency is 1
  if(bkgEff > gr->GetMaximum())
    return 1.0;

  // If requested bkg efficiency is less than exists in the effBvsS
  // curve, all signal events are rejected for that bkg eff and thus
  // the signal efficiency is 0
  if(bkgEff < gr->GetMinimum())
    return 0.0;

  for(int bin=histoBins; bin >= 1; --bin) {
  //for(int bin=1; bin <=histoBins; ++bin) {
    // get corresponding signal and background efficiencies
    effS = (bin-0.5)/double(histoBins);
    if(effS < Smin || effS > Smax) // skip effS which are not in the range of the spline
      continue;
    effB = effSpl->Eval(effS);
    
    //std::cout << effS << " " << effB << "   " << (effB-bkgEff)*(effB_-bkgEff) << std::endl;

    // found signal efficiency that corresponds to required background efficiency
    if((effB-bkgEff)*(effB_-bkgEff) <= 0) {
      found = true;
      break;
    }

    effS_ = effS;
    effB_ = effB;
  }

  //std::cout << "bkgEff " << bkgEff << " effB " << effB << " effB_ " << effB_ << " effS " << effS << " effS_ " << effS_ << " found " << found << std::endl;

  // Take mean between bin above and bin below
  if(found)
    effS = 0.5*(effS + effS_);
  // If the intersection point is not found, the background efficiency
  // is always less than the given level (bkgEff argument) => the
  // signal efficiency at given background efficiency is 1
  else
    effS = 1.0;

  return effS;
}

double MyEvaluate::getSignalEfficiencyError(double nevents_s, double eff_s) {
  double err = 0;
  if(nevents_s > 0)
    err = TMath::Sqrt(eff_s*(1.0-eff_s)/nevents_s);
  return err;
}

void MyEvaluate::printEffResults(std::vector<EffResult>& results, MyOutput& csvOutput, std::string column5, std::string column6, bool scaleBkg) {
  fLogger << kINFO 
          << "Evaluation results ranked by best signal efficiency at 1e-5" << Endl
          << hLine << Endl
          << "MVA              Signal event efficiency at bkg event efficiency (error):" << Endl
    //<< "Methods:         @B=1e-5      @B=1e-4      @B=1e-3      @B=0.01      " << (scaleBkg?"":"@B=0.1") <<  Endl
          << "Methods:         @B=1e-6      @B=1e-5      @B=1e-4      @B=1e-3      @B=0.01      " << (scaleBkg?"":"@B=0.1") <<  Endl
          << hLine << Endl;

  for(std::vector<EffResult>::const_iterator iter = results.begin(); iter != results.end(); ++iter) {
    fLogger << kINFO << Form("%-15s: %1.4f(%03d)  %1.4f(%03d)  %1.4f(%03d)  %1.4f(%03d)  %1.4f(%03d)",
                             iter->name.c_str(),
                             iter->eff6, int(iter->eff6err*1e4),
                             iter->eff5, int(iter->eff5err*1e4), iter->eff4, int(iter->eff4err*1e4),
                             iter->eff3, int(iter->eff3err*1e4), iter->eff2, int(iter->eff2err*1e4))
            << (scaleBkg?"":Form("  %1.4f(%03d)", iter->eff1, int(iter->eff1err*1e4)))
            << Endl;
    csvOutput.addResult(iter->name, column5, iter->eff5);
    csvOutput.addResult(iter->name, column6, iter->eff6);
  }

  fLogger << kINFO << hLine << Endl;
}
