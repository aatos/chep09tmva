#include "evaluate.h"

#include<algorithm>

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

struct EffResultCompare: public std::binary_function<EffResult, EffResult, bool> {
  bool operator()(const EffResult& a, const EffResult& b) const {
    return a.eff1 > b.eff1;
  }
};

MyEvaluate *MyEvaluate::thisBase = 0;

MyEvaluate::MyEvaluate(TFile *file):
  outputFile(file),
  top(0),
  histoBins(10000),
  rocBins(100),
  fLogger(std::string("MyEvaluate")) {

  top = file->mkdir("MyEvaluate");  
}

MyEvaluate::~MyEvaluate() {
  outputFile->Write();
  outputFile->Close();
}

void MyEvaluate::setSignalTree(TTree *tree, double weight, const TCut& cut) {
  signal.tree = tree;
  signal.weight = weight;
  signal.preCut = cut.GetTitle();
}

void MyEvaluate::setBackgroundTree(TTree *tree, double weight, const TCut& cut) {
  background.tree = tree;
  background.weight = weight;
  background.preCut = cut.GetTitle();
}

void MyEvaluate::calculateEventEfficiency(MyConfig& config) {

  /*
  std::map<std::string, Float_t *> values;
  for(std::vector<std::string>::const_iterator var = config.variables.begin(); var != config.variables.end(); ++var) {
    Float_t *val = new Float_t(0);
    testTree->SetBranchAddress(Form("MVA_%s", meth->c_str()), val);
    values.insert(std::make_pair(*meth, val));
  }
  */

  fLogger << kINFO << Endl
          << Endl
          << "Testing classifiers in order to obtain signal/background efficiencies for EVENTS" << Endl
          << "(not jets, which are the input to TMVA)" << Endl;

  std::map<std::string, int> cutOrientation;

  // Look for the cut orientation for each classifier
  for(std::map<std::string, std::string>::const_iterator method = config.classifiers.begin(); method != config.classifiers.end(); ++method) {
    std::string mvaType = stripType(method->first);
    TString format("");
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
  TMVA::Reader *reader = new TMVA::Reader("!Color:Silent");
  fLogger << kINFO << "2-0" << Endl;
  std::vector<float *> readerVariables;
  int nvariables = config.variables.size();

  fLogger << kINFO << "2-1" << Endl;

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

  std::map<std::string, float *> mvaValues;
  for(std::map<std::string, std::string>::const_iterator method = config.classifiers.begin(); method != config.classifiers.end(); ++method) {
    const char *mvaName = method->first.c_str();
    reader->BookMVA(mvaName, Form("weights/chep09tmva_%s.weights.txt", mvaName));

    float *mva = new float(0);
    mvaValues.insert(std::make_pair(method->first, mva));
    mvaOutput->Branch(mvaName, mva, Form("%s/F", mvaName));
  }

  // Compute MVA outputs for all entries (=jets) in the input TTrees
  // (for both signal and background)
  Long64_t njets[2] = {0, 0};
  Long64_t njets_passed[2] = {0, 0};
  Long64_t nevents_passed[2] = {0, 0};
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
          if(cutOrientation[iter->first] > 0) 
            *(iter->second) = std::max(float(reader->EvaluateMVA(iter->first.c_str())), *(iter->second)); 
          else
            *(iter->second) = std::min(float(reader->EvaluateMVA(iter->first.c_str())), *(iter->second)); 
        }
        /*
        std::cout << "Entry " << ientry << " has same event and run numbers as the previous entry: event "
                  << currentEvent << "/" << prevEvent
                  << " run " << currentRun << "/" << prevRun << std::endl;
        */
      }

      /*
        std::cout << "cutVal " << cutVal << "; ";
        for(std::map<std::string, TTreeFormula *>::iterator iter = vars.begin(); iter != vars.end(); ++iter) {
        std::cout << iter->first << " " << iter->second->EvalInstance(0) << "; ";
        }
        std::cout << std::endl;
      */

      prevEvent = currentEvent;
      prevRun = currentRun;
    }
    // Filling of the results of the last entry/event
    mvaOutput->Fill();
    ++nevents_passed[type];

    for(int i=0; i<nvariables; ++i) {
      delete treeFormulas[i];
    }
  }

  TMVA::gConfig().SetSilent(kFALSE);

  fLogger << kINFO << Endl;
  for(type=0; type < 2; ++type) {
    fLogger << kINFO << (type?"Signal":"Background") << " preselection: passed " << njets_passed[type] << "/" << njets[type]
            << "=" << double(njets_passed[type])/njets[type] << " jets; " << nevents_passed[type] << " events" << Endl;
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

  // Renormalize efficiency histograms, compute ROC and signal
  // efficiencies at various background efficiency levels
  std::vector<EffResult> efficiencies;
  for(std::map<std::string, MvaData>::iterator iter = dataMap.begin(); iter != dataMap.end(); ++iter) {
    const char *mvaName = iter->first.c_str();
    MvaData& data = iter->second;

    // Renormalize efficiency histograms to maximum
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
    TMVA::TSpline1 *splEff = new TMVA::TSpline1("effBvsSspl", effBvsSgr);

    EffResult res;
    res.name = iter->first;
    res.eff1 = getSignalEfficiency(0.1, splEff);
    res.eff2 = getSignalEfficiency(0.01, splEff);
    res.eff3 = getSignalEfficiency(1e-3, splEff);
    res.eff4 = getSignalEfficiency(1e-4, splEff);
    res.eff5 = getSignalEfficiency(1e-5, splEff);

    res.eff1err = getSignalEfficiencyError(nevents_passed[1], res.eff1);
    res.eff2err = getSignalEfficiencyError(nevents_passed[1], res.eff2);
    res.eff3err = getSignalEfficiencyError(nevents_passed[1], res.eff3);
    res.eff4err = getSignalEfficiencyError(nevents_passed[1], res.eff4);
    res.eff5err = getSignalEfficiencyError(nevents_passed[1], res.eff5);

    efficiencies.push_back(res);

    sigEffSpline=0;
    delete splEff;
    delete effBvsSgr;
    delete splS;
    delete splB;
    delete grS;
    delete grB;
  }

  std::sort(efficiencies.begin(), efficiencies.end(), EffResultCompare());
  TString hLine = "-----------------------------------------------------------------------------";
  fLogger << kINFO << Endl
    //      << "Evaluating all classifiers for signal efficiency @ 1e-5 OVERALL bkg efficiency" << Endl
    //      << Endl
    //      <<      "                                  signal   background" << Endl
    //      << Form("Event preselection efficiency   : %1.5f  %1.5f", signalEventSelEff, bkgEventSelEff) << Endl
    //      << Form("Jet preselection efficiency     : %1.5f  %1.5f", signalJetSelEff, bkgJetSelEff) << Endl
    //      <<      "-----------------------------------------------------" << Endl
    //      << Form("Overall preselection efficiency : %1.5f  %1.5f", signalPreSelEff, bkgPreSelEff) << Endl
    //      << Endl
    //      << Form("Thus the 1e-5 overall bkg efficiency corresponds %5e bkg efficiency in TMVA", refEff) << Endl
    //      << Endl
          << "Evaluation results ranked by best signal efficiency" << Endl
          << hLine << Endl
    //<< "MVA              Signal efficiency at bkg eff. (error):" << Endl
          << "MVA              Signal event efficiency at bkg event efficiency (error):" << Endl
          << "Methods:         @B=1e-5      @B=1e-4      @B=1e-3      @B=0.01      @B=0.1" << Endl
          << hLine << Endl;
  for(std::vector<EffResult>::const_iterator iter = efficiencies.begin(); iter != efficiencies.end(); ++iter) {
    fLogger << kINFO << Form("%-15s: %1.4f(%03d)  %1.4f(%03d)  %1.4f(%03d)  %1.4f(%03d)  %1.4f(%03d)",
                             iter->name.c_str(),
                             iter->eff5, int(iter->eff5err*1e4), iter->eff4, int(iter->eff4err*1e4),
                             iter->eff3, int(iter->eff3err*1e4), iter->eff2, int(iter->eff2err*1e4), 
                             iter->eff1, int(iter->eff1err*1e4)) << Endl;
  }

  fLogger << kINFO << hLine << Endl;

  fLogger << kWARNING << Endl
          << " !!!  This module is still experimental  !!!" << Endl
          << Endl;

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

double MyEvaluate::getSignalEfficiency(double bkgEff, TMVA::TSpline1 *effSpl) {
  double effS = 0;
  double effS_ = 0;
  double effB_ = 0;
  for(int bin=1; bin <= histoBins; ++bin) {
    // get corresponding signal and background efficiencies
    effS = (bin-0.5)/double(histoBins);
    double effB = effSpl->Eval(effS);

    //std::cout << effS << " " << effB << std::endl;

    // find signal efficiency that corresponds to required background efficiency
    if((effB-bkgEff)*(effB_-bkgEff) <= 0)
      break;

    effS_ = effS;
    effB_ = effB;
  }

  // take mean between bin above and bin below
  effS = 0.5*(effS + effS_);

  return effS;
}

double MyEvaluate::getSignalEfficiencyError(double nevents_s, double eff_s) {
  double err = 0;
  if(nevents_s > 0)
    err = TMath::Sqrt(eff_s*(1.0-eff_s)/nevents_s);
  return err;
}
