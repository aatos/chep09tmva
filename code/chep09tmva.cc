#include<iostream>
#include<string>
#include<vector>
#include<map>
#include<cstring>

#include <TFile.h>
#include <TChain.h>
#include <TString.h>
#include <TSystem.h>

#include <TMVA/Config.h>

#include "config.h"
#include "factory.h"

void print_usage(void);


// See more usage examples about TMVA training in tmva/TMVA/examples/TMVAnalysis.C
int main(int argc, char **argv) {
  // Configuration
  //TString inputfileName("mva-input.root");
  TString outputfileName("TMVA.root");

  double signalEventSelEff = 1;
  double backgroundEventSelEff = 1;

  double signalWeight     = 1.0;
  double backgroundWeight = 1.0;

  TMVA::gConfig().GetVariablePlotting().fNbinsXOfROCCurve = 200;

  // parse configuration
  std::string confFile("tmva-common.conf");
  bool color = true;
  if(argc > 1) {
    confFile = argv[1];
    if(argc > 2) {
      if(std::strcmp(argv[2], "color=false") == 0) {
        color = false;
      }
    }
  }
  if(confFile == "-help" || confFile == "--help" || confFile == "-h" || confFile == "-?") {
    print_usage();
    return 0;
  }

  MyConfig config;

  // Cuts
  TCut signalCut_s = "";
  TCut bkgCut_s = "";

  if(!parseConf(confFile, config))
    return 1;

  if(config.signalFiles.size() == 0) {
    std::cout << "SignalFiles is empty!" << std::endl;
    return 1;
  }
  else if(config.bkgFiles.size() == 0) {
    std::cout << "BackgroundFiles is empty!" << std::endl;
    return 1;
  }

  std::cout << "Variables:" << std::endl;
  for(std::vector<std::string>::const_iterator iter = config.variables.begin();
      iter != config.variables.end(); ++iter) {
    std::cout << *iter << std::endl;
  }
  std::cout << std::endl;

  std::cout << "Cuts (applied to signal)" << std::endl;
  for(std::vector<std::string>::const_iterator iter = config.signalCuts.begin();
      iter != config.signalCuts.end(); ++iter) {
    std::cout << *iter << std::endl;
    signalCut_s = signalCut_s && TCut(iter->c_str());
  }
  //std::cout << cut_s << std::endl;
  std::cout << std::endl;

  std::cout << "Cuts (applied to background)" << std::endl;
  for(std::vector<std::string>::const_iterator iter = config.bkgCuts.begin();
      iter != config.bkgCuts.end(); ++iter) {
    std::cout << *iter << std::endl;
    bkgCut_s = bkgCut_s && TCut(iter->c_str());
  }
  //std::cout << cut_s << std::endl;
  std::cout << std::endl;
  
  std::cout << "Trainer: " << config.trainer << std::endl << std::endl;
  std::cout << "Classifiers:" << std::endl;
  for(std::map<std::string, std::string>::const_iterator iter = config.classifiers.begin();
      iter != config.classifiers.end(); ++iter) {
    std::cout << iter->first << ": " << iter->second << std::endl;
  }
  std::cout << std::endl;

  // Input TChains
  TChain *signalChain = new TChain("TauID_Pythia8_generatorLevel_HCh300");
  TChain *bkgChain = new TChain("TauID_Pythia8_generatorLevel_QCD_120_170");

  long signalEntries = 0;
  long bkgEntries = 0;

  std::cout << "Files for signal TChain" << std::endl;
  for(std::vector<std::string>::const_iterator iter = config.signalFiles.begin();
      iter != config.signalFiles.end(); ++iter) {
    std::cout << *iter << std::endl;
    signalChain->AddFile(iter->c_str());
  }
  signalEntries = signalChain->GetEntries();
  std::cout << "Chain has " << signalEntries << " entries" << std::endl << std::endl;
  
  std::cout << "Files for background TChain" << std::endl;
  for(std::vector<std::string>::const_iterator iter = config.bkgFiles.begin();
      iter != config.bkgFiles.end(); ++iter) {
    std::cout << *iter << std::endl;
    bkgChain->AddFile(iter->c_str());
  }
  bkgEntries = bkgChain->GetEntries();
  std::cout << "Chain has " << bkgEntries << " entries" << std::endl << std::endl;

  // Check input file existence
  //TString pwd(getenv("LS_SUBCWD"));
  //TString ipath = pwd;
  //ipath += "/";
  //ipath += inputfileName;
  //TString ipath = inputfileName;
  //if(gSystem->AccessPathName(ipath)) {
  //  std::cout << "Input file " << ipath << " does not exist!" << std::endl;
  //  return 1;
  //}
  
  // Create output file
  TFile *outputFile = TFile::Open(outputfileName, "RECREATE");

  // Initialize factory
  TString foptions = "!V:!Silent";
  if(color)
    foptions += ":Color";
  else
    foptions += ":!Color";
  TMVA::Factory *factory = new MyFactory("trainTMVA", outputFile, foptions);

  // Assign variables
  for(std::vector<std::string>::const_iterator iter = config.variables.begin();
      iter != config.variables.end(); ++iter) {
    factory->AddVariable(*iter, 'F');
  }

  // Read input file
  //TFile *inputFile = TFile::Open(ipath);
  
  // Read signal and background trees, assign weights
  //TTree *signal     = dynamic_cast<TTree *>(inputFile->Get("TreeS"));
  //TTree *background = dynamic_cast<TTree *>(inputFile->Get("TreeB"));
  
  // Register tree
  factory->AddSignalTree(signalChain, signalWeight);
  factory->AddBackgroundTree(bkgChain, backgroundWeight);

  // Prepare training and testing
  //factory->PrepareTrainingAndTestTree(cut_s, cut_b, "NSigTrain=4000:NBkgTrain=230000:SplitMode=Random:NormMode=NumEvents:!V");
  factory->PrepareTrainingAndTestTree(signalCut_s, bkgCut_s, config.trainer);

  // Book MVA methods
  for(std::map<std::string, std::string>::const_iterator iter = config.classifiers.begin();
      iter != config.classifiers.end(); ++iter) {
    TMVA::Types::EMVA type = getType(iter->first);
    factory->BookMethod(type, iter->first, iter->second);
  }

  // Train classifiers
  factory->TrainAllMethods();

  // Evaluate all classifiers
  factory->TestAllMethods();

  // Compare classifier performance
  factory->EvaluateAllMethods();


  // MyFactory stuff
  MyFactory *fac = dynamic_cast<MyFactory* >(factory);
  if(fac) {
    //fac->calculateEventEfficiency(classifier_names);
    //fac->printEfficiency(classifier_names, signalEventSelEff, backgroundEventSelEff, signalEntries, bkgEntries);
  }

  // Save output
  outputFile->Close();

  std::cout << "Created output file " << outputfileName << std::endl;
  //std::cout << "Launch TMVA GUI by 'root -l TMVAGui.C'" << std::endl;

  // Clean up
  delete factory;
  //inputFile->Close();
}

void print_usage(void) {
  std::cout << "Usage: chep09tmva [config_file]" << std::endl
            << std::endl
            << "  config_file       path to configuration file (optional)" << std::endl
            << std::endl
            << "  The default value for config_file is tmva-common.conf" << std::endl;
}
