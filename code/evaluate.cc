#include "evaluate.h"

#include <TTreeFormula.h>

#include <TMVA/Reader.h>

MyEvaluate::MyEvaluate(TFile *file):
  outputFile(file),
  top(0),
  histoBins(10000) {

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

  TMVA::Reader *reader = new TMVA::Reader("!Color:Silent");
  std::vector<TTreeFormula *> treeFormulas;
  std::vector<float *> readerVariables;
  int nvariables = config.variables.size();
  for(std::vector<std::string>::const_iterator var = config.variables.begin(); var != config.variables.end(); ++var) {
    TTreeFormula *ttf = new TTreeFormula(Form("Formula_%s", var->c_str()), var->c_str(), signal.tree);
    treeFormulas.push_back(ttf);

    float *rvar = new float(0);
    reader->AddVariable(var->c_str(), rvar);
    readerVariables.push_back(rvar);
  }

  TTree *mvaOutput = new TTree("mvaOutput", "mvaOutput");
  mvaOutput->SetDirectory(0);
  int type = -1;
  mvaOutput->Branch("type", &type, "type/I");

  std::map<std::string, float *> mvaValues;
  for(std::map<std::string, std::string>::const_iterator method = config.classifiers.begin(); method != config.classifiers.end(); ++method) {
    reader->BookMVA(method->first.c_str(), Form("weights/chep09tmva_%s.weights.txt", method->first.c_str()));

    float *mva = new float(0);
    mvaValues.insert(std::make_pair(method->first, mva));
    mvaOutput->Branch(method->first.c_str(), mva, Form("%s/F", method->first.c_str()));
  }

  TTreeFormula *sigCut = new TTreeFormula("SigCut", signal.preCut, signal.tree);
  if(sigCut->GetNcodes() == 0) {
    std::cout << "ERROR: Signal cut '" << signal.preCut << "' does not depend on any TTree variable in TTree "
              << signal.tree->GetName() << std::endl;
    return;
  }

  int prevEvent = -1;
  int prevRun = -1;
  int currentEvent = 0;
  int currentRun = 0;
  signal.tree->SetBranchStatus("evt", 1);
  signal.tree->SetBranchStatus("run", 1);
  signal.tree->SetBranchAddress("evt", &currentEvent);
  signal.tree->SetBranchAddress("run", &currentRun);

  Long64_t entries = signal.tree->GetEntries();
  Long64_t passed = 0;
  for(Long64_t ientry = 0; ientry < entries; ++ientry) {
    signal.tree->GetEntry(ientry);

    Int_t ndata = sigCut->GetNdata();
    Float_t cutVal = sigCut->EvalInstance(0);
    if(cutVal < 0.5) {
      prevEvent = currentEvent;
      prevRun = currentRun;
      continue;
    }
    ++passed;

    for(int i=0; i<nvariables; ++i) {
      *(readerVariables[i]) = treeFormulas[i]->EvalInstance(0);
    }

    type = 1;
    for(std::map<std::string, float *>::const_iterator iter = mvaValues.begin(); iter != mvaValues.end(); ++iter) {
      *(iter->second) = std::max(float(reader->EvaluateMVA(iter->first.c_str())), *(iter->second));
    }
    if(currentEvent != prevEvent || currentRun != prevRun) {
      mvaOutput->Fill();
      for(std::map<std::string, float *>::const_iterator iter = mvaValues.begin(); iter != mvaValues.end(); ++iter) {
        *(iter->second) = -9999;
      }
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

  std::cout << "Passed " << passed << "/" << entries << "=" << double(passed)/entries << " in preselection" << std::endl;

  top->cd();
  for(std::map<std::string, float *>::iterator iter = mvaValues.begin(); iter != mvaValues.end(); ++iter) {
    double xmin, xmax;
    xmin = mvaOutput->GetMinimum(iter->first.c_str());
    xmax = mvaOutput->GetMaximum(iter->first.c_str());
    mvaOutput->Draw(Form("%s >>MyMVA_%s_S(%d,%f,%f)", iter->first.c_str(), iter->first.c_str(), histoBins, xmin, xmax),
                    "type == 1", "goff");
    TH1 *histo = mvaOutput->GetHistogram();
    histo->SetDirectory(top);

    delete iter->second;
  }

  delete mvaOutput;

  for(int i=0; i<nvariables; ++i) {
    delete treeFormulas[i];
    delete readerVariables[i];
  }

  delete sigCut;
  delete reader;
}
