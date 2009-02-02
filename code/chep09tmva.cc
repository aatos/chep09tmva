#include<iostream>
#include<string>
#include<vector>
#include<map>
#include<cstring>

#include <TFile.h>
#include <TChain.h>
#include <TString.h>
#include <TSystem.h>
#include <TH1.h>

#include <TMVA/Config.h>

#include "config.h"
#include "factory.h"
#include "evaluate.h"

void print_usage(void);
void createChain(std::vector<std::string>& files, TChain *chain, const char *datasetName,
                 long& eventsAll, long& eventsSelected);

// See more usage examples about TMVA training in tmva/TMVA/examples/TMVAnalysis.C
int main(int argc, char **argv) {
  // Configuration
  TString outputfileName("TMVA.root");
  TString signalDataset("Pythia8_generatorLevel_HCh300");
  TString bkgDataset("Pythia8_generatorLevel_QCD_120_170");
  TString signalTreeName("TauID_");
  TString bkgTreeName("TauID_");
  signalTreeName += signalDataset;
  bkgTreeName += bkgDataset;

  double signalWeight     = 1.0;
  double backgroundWeight = 1.0;

  TMVA::gConfig().GetVariablePlotting().fNbinsXOfROCCurve = 200;

  // Parse command line arguments
  std::string confFile("tmva-common.conf");
  bool color = true;
  if(argc > 1) {
    confFile = argv[1];
    if(argc > 2) {
      if(std::strncmp(argv[2], "color=false", 12) == 0) {
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

  // Parse configuration
  if(!parseConf(confFile, config))
    return 1;
  if(config.signalTrainFiles.size() == 0) {
    std::cout << "SignalTrainFiles is empty!" << std::endl;
    return 1;
  }
  else if(config.bkgTrainFiles.size() == 0) {
    std::cout << "BackgroundTrainFiles is empty!" << std::endl;
    return 1;
  }

  // Print configuration
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
  TChain *signalTrainChain = new TChain(signalTreeName);
  TChain *bkgTrainChain = new TChain(bkgTreeName);
  TChain *signalTestChain = 0;
  TChain *bkgTestChain = 0;
  if(config.signalTestFiles.size() > 0)
    signalTestChain = new TChain(signalTreeName);
  if(config.bkgTestFiles.size() > 0)
    bkgTestChain = new TChain(bkgTreeName);

  long signalEntries = 0;
  long signalEventsAll = 0;
  long signalEventsSelected = 0;
  long bkgEntries = 0;
  long bkgEventsAll = 0;
  long bkgEventsSelected = 0;

  std::cout << "Files for signal training TChain" << std::endl;
  createChain(config.signalTrainFiles, signalTrainChain, signalDataset, signalEventsAll, signalEventsSelected);
  signalEntries = signalTrainChain->GetEntries();
  std::cout << "Training chain has " << signalEntries << " entries (jets)" << std::endl
            << signalEventsAll << " events were generated, of which " << signalEventsSelected << " events were selected as input" << std::endl
            << std::endl;

  if(signalTestChain) {
    signalEventsAll = 0;
    signalEventsSelected = 0;
    std::cout << "Files for signal testing TChain" << std::endl;
    createChain(config.signalTestFiles, signalTestChain, signalDataset, signalEventsAll, signalEventsSelected);
    signalEntries = signalTestChain->GetEntries();
    std::cout << "Testing chain has " << signalEntries << " entries (jets)" << std::endl
              << signalEventsAll << " events were generated, of which " << signalEventsSelected << " events were selected as input" << std::endl
              << std::endl;
  }
  else {
    std::cout << "!!! No files given for signal testing chain, training chain is split for training and testing !!!" << std::endl << std::endl;
  }
  
  std::cout << "Files for background training TChain" << std::endl;
  createChain(config.bkgTrainFiles, bkgTrainChain, bkgDataset, bkgEventsAll, bkgEventsSelected);
  bkgEntries = bkgTrainChain->GetEntries();
  std::cout << "Training Chain has " << bkgEntries << " entries (jets)" << std::endl
            << bkgEventsAll << " events were generated, of which " << bkgEventsSelected << " events were selected as input" << std::endl
            << std::endl;

  if(bkgTestChain) {
    bkgEventsAll = 0;
    bkgEventsSelected = 0;
    std::cout << "Files for background testing TChain" << std::endl;
    createChain(config.bkgTrainFiles, bkgTrainChain, bkgDataset, bkgEventsAll, bkgEventsSelected);
    bkgEntries = bkgTrainChain->GetEntries();
    std::cout << "Testing Chain has " << bkgEntries << " entries (jets)" << std::endl
              << bkgEventsAll << " events were generated, of which " << bkgEventsSelected << " events were selected as input" << std::endl
              << std::endl;
  }
  else {
    std::cout << "!!! No files given for background testing chain, training chain is split for training and testing !!!" << std::endl << std::endl;
  }

  // Create output file
  TFile *outputFile = TFile::Open(outputfileName, "RECREATE");

  // Initialize factory
  TString foptions = "!V:!Silent";
  if(color)
    foptions += ":Color";
  else
    foptions += ":!Color";
  TMVA::Factory *factory = new MyFactory("chep09tmva", outputFile, foptions);

  // Assign variables
  for(std::vector<std::string>::const_iterator iter = config.variables.begin();
      iter != config.variables.end(); ++iter) {
    factory->AddVariable(*iter, 'F');
  }

  // Register trees
  if(signalTestChain) {
    factory->AddSignalTree(signalTrainChain, signalWeight, TMVA::Types::kTraining);
    factory->AddSignalTree(signalTestChain, signalWeight, TMVA::Types::kTesting);
  }
  else {
    factory->AddSignalTree(signalTrainChain, signalWeight);
  }
  if(bkgTestChain) {
    factory->AddBackgroundTree(bkgTrainChain, backgroundWeight, TMVA::Types::kTraining);
    factory->AddBackgroundTree(bkgTestChain, backgroundWeight, TMVA::Types::kTesting);
  }
  else {
    factory->AddBackgroundTree(bkgTrainChain, backgroundWeight);
  }

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
    fac->printEfficiency(config, signalEventsAll, signalEventsSelected,
                         bkgEventsAll, bkgEventsSelected,
                         signalEntries, bkgEntries);
  }

  // Save output
  outputFile->Close();

  // Clean up
  delete factory;
  //inputFile->Close();

  MyEvaluate evaluate(TFile::Open(outputfileName, "UPDATE"));
  if(signalTestChain) 
    evaluate.setSignalTree(signalTestChain, signalWeight, signalCut_s, true);
  else
    evaluate.setSignalTree(signalTrainChain, signalWeight, signalCut_s, false);
  evaluate.setSignalEventNum(signalEventsAll, signalEventsSelected);
  if(bkgTestChain)
    evaluate.setBackgroundTree(bkgTestChain, backgroundWeight, bkgCut_s, true);
  else
    evaluate.setBackgroundTree(bkgTrainChain, backgroundWeight, bkgCut_s, false);
  evaluate.setBackgroundEventNum(bkgEventsAll, bkgEventsSelected);
  evaluate.calculateEventEfficiency(config);

  std::cout << "Created output file " << outputfileName << std::endl;
  //std::cout << "Launch TMVA GUI by 'root -l TMVAGui.C'" << std::endl;
}

void print_usage(void) {
  std::cout << "Usage: chep09tmva [config_file]" << std::endl
            << std::endl
            << "  config_file       path to configuration file (optional)" << std::endl
            << std::endl
            << "  The default value for config_file is tmva-common.conf" << std::endl;
}


void createChain(std::vector<std::string>& files, TChain *chain, const char *datasetName,
                 long& eventsAll, long& eventsSelected) {
  for(std::vector<std::string>::const_iterator iter = files.begin(); iter != files.end(); ++iter) {
    std::cout << *iter << std::endl;
    chain->AddFile(iter->c_str());

    TFile *file = TFile::Open(iter->c_str());
    TH1 *counter = dynamic_cast<TH1 *>(file->Get(Form("counter_main_%s", datasetName)));
    if(counter) {
      eventsAll += counter->GetBinContent(1);
      for(int bin=2; bin <= counter->GetNbinsX(); ++bin) {
        if(std::strncmp(counter->GetXaxis()->GetBinLabel(bin), "Passed E correction", 20) == 0) {
          eventsSelected += counter->GetBinContent(bin);
          break;
        }
      }
    }
    file->Close();
  }
}
