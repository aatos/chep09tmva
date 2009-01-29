#include "evaluate.h"

#include <TTreeFormula.h>

#include <TMVA/Reader.h>

struct MvaData {
  TH1 *histoS;
  TH1 *histoB;
  TH1 *effS;
  TH1 *effB;
};


MyEvaluate::MyEvaluate(TFile *file):
  outputFile(file),
  top(0),
  histoBins(10000),
  rocBins(100) {

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

  // Create TTreeFormula objects corresponding to the preselection cuts
  TTreeFormula *sigCut = new TTreeFormula("SigCut", signal.preCut, signal.tree);
  if(sigCut->GetNcodes() == 0) {
    std::cout << "ERROR: Signal cut '" << signal.preCut << "' does not depend on any TTree variable in TTree "
              << signal.tree->GetName() << std::endl;
    return;
  }
  TTreeFormula *bkgCut = new TTreeFormula("BkgCut", background.preCut, background.tree);
  if(bkgCut->GetNcodes() == 0) {
    std::cout << "ERROR: Background cut '" << background.preCut << "' does not depend on any TTree variable in TTree "
              << background.tree->GetName() << std::endl;
    delete sigCut;
    return;
  }

  // Initialize TMVA Reader
  TMVA::Reader *reader = new TMVA::Reader("!Color:Silent");
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
  Long64_t entries[2] = {0, 0};
  Long64_t passed[2] = {0, 0};
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
    entries[type] = tree->GetEntries();
    for(Long64_t ientry = 0; ientry < entries[type]; ++ientry) {
      tree->GetEntry(ientry);

      // Check if the entry passes the preselection cut
      //Int_t ndata = preCut->GetNdata();
      Float_t cutVal = preCut->EvalInstance(0);
      if(cutVal < 0.5) {
        //std::cout << "Taking out entry " << ientry << " in event " << currentEvent << ", run " << currentRun << std::endl;
        continue;
      }
      ++passed[type];

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
        if(fillStarted)
          mvaOutput->Fill();
        else
          fillStarted = true;

        for(std::map<std::string, float *>::const_iterator iter = mvaValues.begin(); iter != mvaValues.end(); ++iter) {
          *(iter->second) = float(reader->EvaluateMVA(iter->first.c_str())), *(iter->second);
        }
      }
      else {
        for(std::map<std::string, float *>::const_iterator iter = mvaValues.begin(); iter != mvaValues.end(); ++iter) {
          *(iter->second) = std::max(float(reader->EvaluateMVA(iter->first.c_str())), *(iter->second)); 
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

    std::cout << "Passed " << passed[type] << "/" << entries[type] << "=" << double(passed[type])/entries[type] 
              << " jets in preselection of " << (type?"signal":"background") << std::endl;

    for(int i=0; i<nvariables; ++i) {
      delete treeFormulas[i];
    }
  }

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


  top->cd();
  Long64_t nentries = mvaOutput->GetEntries();
  for(Long64_t ientry=0; ientry < nentries; ++ientry) {
    mvaOutput->GetEntry(ientry);

    for(std::map<std::string, float *>::iterator iter = mvaValues.begin(); iter != mvaValues.end(); ++iter) {
      MvaData& data = dataMap[iter->first];
      float mvaValue = *(iter->second);

      TH1* histo = type ? data.histoS : data.histoB;
      TH1* eff = type ? data.effS : data.effB;

      histo->Fill(mvaValue);
    }
  }

  /*
    for(std::map<std::string, float *>::iterator iter = mvaValues.begin(); iter != mvaValues.end(); ++iter) {
      const char *mvaName = iter->first.c_str();
      const char *sb = type ? "S" : "B";
      mvaOutput->Draw(Form("%s >>MyMVA_%s_%s(%d,%f,%f)", mvaName, mvaName, sb, histoBins, xmin, xmax),
                      Form("type == %d", type), "goff");
      TH1 *histo = mvaOutput->GetHistogram();
      histo->SetDirectory(top);

      TH1 *eff = new TH1F(Form("MyMVA_%s_eff%s", mvaName, sb, Form("%s efficiency", sb)), histoBins, xmin, xmax);
      for(Long64_t ientry=0; ientry < 

              TH1 *histo_s = dynamic_cast<TH1 *>(top->Get(Form("MyMVA_%s_S", iter->first.c_str())));
              TH1 *histo_b = dynamic_cast<TH1 *>(top->Get(Form("MyMVA_%s_B", iter->first.c_str())));

              TH1 *effBvsS = new TH1F(Form("MyMVA_%s_effBvsS", "B efficiency vs. S", rocBins, 0, 1));
              TH1 *rejBvsS = new TH1F(Form("MyMVA_%s_rejBvsS", "B rejection vs. S", rocBins, 0, 1));

    }
  */
  
  for(std::map<std::string, float *>::iterator iter = mvaValues.begin(); iter != mvaValues.end(); ++iter) {
    delete iter->second;
  }

  delete mvaOutput;  

  for(int i=0; i<nvariables; ++i) {
    delete readerVariables[i];
  }

  delete sigCut;
  delete reader;
}
