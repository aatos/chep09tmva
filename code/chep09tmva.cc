#include<iostream>
#include<iomanip>
#include<string>
#include<vector>
#include<map>
#include<cstring>
#include<cstdlib>
#include<algorithm>

#include <TFile.h>
#include <TChain.h>
#include <TString.h>
#include <TSystem.h>
#include <TH1.h>
#include <TList.h>
#include <TKey.h>

#include "TROOT.h"

#include <TMVA/Config.h>

#include "config.h"
#include "factory.h"
#include "evaluate.h"
#include "output.h"
#include "timer.h"

void print_usage(void);
void createChain(std::vector<std::string>& files, TChain *chain, const char *datasetName,
                 long& eventsAll, long& eventsSelected);

bool sniffChainName(std::vector<std::string>& files, const std::string& treeName,
                    std::string& chainName, std::string& dataset);


// See more usage examples about TMVA training in tmva/TMVA/examples/TMVAnalysis.C
int main(int argc, char **argv) {
  std::cout <<"Using ROOT version "<< gROOT->GetVersion() << std::endl;

  TStopwatch timer_total;
  timer_total.Start();
  TStopwatch timer;
  TimerData timer_data;
  timer_data.realtime = 0;
  timer_data.cputime = 0;
  std::vector<std::pair<std::string, TimerData> > timer_vector;
  std::vector<std::pair<std::string, TimerData> > timer_vector_eval;
  timer.Start();

  // Configuration
  TString outputfileName("TMVA.root");
  MyOutput csvOutput("tmva.csvoutput.txt");
  std::string treeName("TauID_");

  double signalWeight     = 1.0;
  double backgroundWeight = 1.0;

  // A number this high in here could be a problem for Cuts, because a
  // classifier is trained for each signal efficiency bin
  int rocbins = 100000;

  TMVA::gConfig().GetVariablePlotting().fNbinsXOfROCCurve = rocbins;

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
    csvOutput.addMethod(iter->first);
  }
  std::cout << std::endl;

  // Input TChains
  std::string signalTrainTreeName, signalTrainDataset, signalTestTreeName, signalTestDataset,
    bkgTrainTreeName, bkgTrainDataset, bkgTestTreeName, bkgTestDataset;

  if(!sniffChainName(config.signalTrainFiles, treeName, signalTrainTreeName, signalTrainDataset)) {
    std::cout << "Unable to sniff the chain name for signal training tree" << std::endl;
    return 1;
  }
  if(!sniffChainName(config.bkgTrainFiles, treeName, bkgTrainTreeName, bkgTrainDataset)) {
    std::cout << "Unable to sniff the chain name for background training tree" << std::endl;
    return 1;
  }

  TChain *signalTrainChain = new TChain(signalTrainTreeName.c_str());
  TChain *bkgTrainChain = new TChain(bkgTrainTreeName.c_str());
  TChain *signalTestChain = 0;
  TChain *bkgTestChain = 0;
  if(config.signalTestFiles.size() > 0) {
    if(!sniffChainName(config.signalTrainFiles, treeName, signalTestTreeName, signalTestDataset)) {
      std::cout << "Unable to sniff the chain name for signal test tree" << std::endl;
      return 1;
    }
    signalTestChain = new TChain(signalTestTreeName.c_str());
  }
  if(config.bkgTestFiles.size() > 0) {
    if(!sniffChainName(config.bkgTrainFiles, treeName, bkgTestTreeName, bkgTestDataset)) {
      std::cout << "Unable to sniff the chain name for background test tree" << std::endl;
      return 1;
    }
    bkgTestChain = new TChain(bkgTestTreeName.c_str());
  }

  long signalEntries = 0;
  long signalEventsAll = 0;
  long signalEventsSelected = 0;
  long bkgEntries = 0;
  long bkgEventsAll = 0;
  long bkgEventsSelected = 0;

  std::cout << "Files for signal training TChain (name: " << signalTrainTreeName << ")" << std::endl;
  createChain(config.signalTrainFiles, signalTrainChain, signalTrainDataset.c_str(), signalEventsAll, signalEventsSelected);
  signalEntries = signalTrainChain->GetEntries();
  std::cout << "Training chain has " << signalEntries << " entries (jets)" << std::endl
            << signalEventsAll << " events were generated, of which " << signalEventsSelected << " events were selected as input" << std::endl
            << std::endl;

  if(signalTestChain) {
    signalEventsAll = 0;
    signalEventsSelected = 0;
    std::cout << "Files for signal testing TChain (name: " << signalTestTreeName << ")" << std::endl;
    createChain(config.signalTestFiles, signalTestChain, signalTestDataset.c_str(), signalEventsAll, signalEventsSelected);
    signalEntries = signalTestChain->GetEntries();
    std::cout << "Testing chain has " << signalEntries << " entries (jets)" << std::endl
              << signalEventsAll << " events were generated, of which " << signalEventsSelected << " events were selected as input" << std::endl
              << std::endl;
  }
  else {
    std::cout << "!!! No files given for signal testing chain, training chain is split for training and testing !!!" << std::endl << std::endl;
  }
  
  std::cout << "Files for background training TChain (name: " << bkgTrainTreeName << ")" << std::endl;
  createChain(config.bkgTrainFiles, bkgTrainChain, bkgTrainDataset.c_str(), bkgEventsAll, bkgEventsSelected);
  bkgEntries = bkgTrainChain->GetEntries();
  std::cout << "Training Chain has " << bkgEntries << " entries (jets)" << std::endl
            << bkgEventsAll << " events were generated, of which " << bkgEventsSelected << " events were selected as input" << std::endl
            << std::endl;

  if(bkgTestChain) {
    bkgEventsAll = 0;
    bkgEventsSelected = 0;
    std::cout << "Files for background testing TChain (name: " << bkgTestTreeName << ")" << std::endl;
    createChain(config.bkgTestFiles, bkgTestChain, bkgTestDataset.c_str(), bkgEventsAll, bkgEventsSelected);
    bkgEntries = bkgTestChain->GetEntries();
    std::cout << "Testing Chain has " << bkgEntries << " entries (jets)" << std::endl
              << bkgEventsAll << " events were generated, of which " << bkgEventsSelected << " events were selected as input" << std::endl
              << std::endl;
  }
  else {
    std::cout << "!!! No files given for background testing chain, training chain is split for training and testing !!!" << std::endl << std::endl;
  }

  // Create output file
  TFile *outputFile = TFile::Open(outputfileName, "RECREATE");

  timer.Stop();
  timer_data.realtime = timer.RealTime();
  timer_data.cputime = timer.CpuTime();
  timer_vector.push_back(std::make_pair("Initialization (ours)", timer_data));
  timer.Reset();


  // Initialize factory
  timer.Start();
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

  timer.Stop();
  timer_data.realtime = timer.RealTime();
  timer_data.cputime = timer.CpuTime();
  timer_vector.push_back(std::make_pair("Initialization (TMVA)", timer_data));
  timer.Reset();

  // Train classifiers
  timer.Start();
  factory->TrainAllMethods();
  timer.Stop();
  timer_data.realtime = timer.RealTime();
  timer_data.cputime = timer.CpuTime();
  timer_vector.push_back(std::make_pair("Training", timer_data));
  timer.Reset();

  // Evaluate all classifiers
  timer.Start();
  factory->TestAllMethods();
  timer.Stop();
  timer_data.realtime = timer.RealTime();
  timer_data.cputime = timer.CpuTime();
  timer_vector.push_back(std::make_pair("Testing", timer_data));
  timer.Reset();

  // Compare classifier performance
  timer.Start();
  factory->EvaluateAllMethods();
  timer.Stop();
  timer_data.realtime = timer.RealTime();
  timer_data.cputime = timer.CpuTime();
  timer_vector.push_back(std::make_pair("Evaluation", timer_data));
  timer.Reset();

  // MyFactory stuff 
  MyFactory *fac = dynamic_cast<MyFactory* >(factory);
  if(fac && std::find(config.reports.begin(), config.reports.end(), "JetEfficiencies") != config.reports.end()) {
    timer.Start();
    fac->printEfficiency(config, csvOutput, signalEventsAll, signalEventsSelected,
                         bkgEventsAll, bkgEventsSelected,
                         signalEntries, bkgEntries);
    timer.Stop();
    timer_data.realtime = timer.RealTime();
    timer_data.cputime = timer.CpuTime();
    timer_vector.push_back(std::make_pair("Jet efficiencies", timer_data));
    timer.Reset();
  }
  
  // Save output
  outputFile->Close();

  // Clean up
  delete factory;
  //inputFile->Close();

  bool doEvaluate = false;
  for(std::vector<std::string>::const_iterator iter = config.reports.begin(); iter != config.reports.end(); ++iter) {
    if(iter->find("Event") != std::string::npos) {
      doEvaluate = true;
      break;
    }
  }
  if(doEvaluate) {
    timer.Start();
    long signalTestEntries = 0;
    long bkgTestEntries = 0;

    size_t start = 0;
    size_t stop = 0;
    while(stop != std::string::npos) {
      stop = config.trainer.find_first_of(':', start+1);
      std::string conf = config.trainer.substr(start, stop-start);
      if(conf.compare(0, 9, "NSigTest=") == 0) {
        signalTestEntries = std::strtol(conf.substr(9, std::string::npos).c_str(), 0, 10);
      }
      else if(conf.compare(0, 9, "NBkgTest=") == 0) {
        bkgTestEntries = std::strtol(conf.substr(9, std::string::npos).c_str(), 0, 10);
      }
      start = stop+1;
    }

    MyEvaluate evaluate(TFile::Open(outputfileName, "UPDATE"), rocbins);
    if(signalTestChain) 
      evaluate.setSignalTree(signalTestChain, signalWeight, signalCut_s, true, signalTestEntries);
    else
      evaluate.setSignalTree(signalTrainChain, signalWeight, signalCut_s, false, signalTestEntries);
    evaluate.setSignalEventNum(signalEventsAll, signalEventsSelected);
    if(bkgTestChain)
      evaluate.setBackgroundTree(bkgTestChain, backgroundWeight, bkgCut_s, true, bkgTestEntries);
    else
      evaluate.setBackgroundTree(bkgTrainChain, backgroundWeight, bkgCut_s, false, bkgTestEntries);
    evaluate.setBackgroundEventNum(bkgEventsAll, bkgEventsSelected);

    evaluate.calculateEventEfficiency(config, csvOutput, timer_vector_eval);
    timer.Stop();
    timer_data.realtime = timer.RealTime();
    timer_data.cputime = timer.CpuTime();
    timer_vector.push_back(std::make_pair("Event efficiencies", timer_data));
    timer.Reset();
  }

  if(std::find(config.reports.begin(), config.reports.end(), "CsvOutput") != config.reports.end()) {
    csvOutput.writeFile();
  }

  if(std::find(config.reports.begin(), config.reports.end(), "Timer") != config.reports.end()) {
    std::cout << "Elapsed                   CPU time  real time" << std::endl;
    for(std::vector<std::pair<std::string, TimerData> >::const_iterator iter = timer_vector.begin(); iter != timer_vector.end(); ++iter) {
      std::cout << "  " << std::setw(22) << std::left << iter->first << ": " 
                << std::setw(9) << std::setprecision(4) << iter->second.cputime << " " 
                << std::setw(9) << std::setprecision(4) << iter->second.realtime << " "
                << "seconds" << std::endl;
    }
    timer_total.Stop();
    std::cout << "  " << "--------------------------------------------------" << std::endl
              << "  " << std::setw(22) << std::left << "Total" << ": " 
              << std::setw(9) << std::setprecision(4) << timer_total.CpuTime() << " " 
              << std::setw(9) << std::setprecision(4) << timer_total.RealTime() << " "
              << "seconds" << std::endl;

    std::cout << std::endl
              << "Event evaluation details  CPU time  real time" << std::endl;
    for(std::vector<std::pair<std::string, TimerData> >::const_iterator iter = timer_vector_eval.begin(); iter != timer_vector_eval.end(); ++iter) {
      std::cout << "  " << std::setw(22) << std::left << iter->first << ": " 
                << std::setw(9) << std::setprecision(4) << iter->second.cputime << " " 
                << std::setw(9) << std::setprecision(4) << iter->second.realtime << " "
                << "seconds" << std::endl;
    }
  }

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
      eventsAll += long(counter->GetBinContent(1));
      for(int bin=2; bin <= counter->GetNbinsX(); ++bin) {
        if(std::strncmp(counter->GetXaxis()->GetBinLabel(bin), "Passed E correction", 20) == 0) {
          eventsSelected += long(counter->GetBinContent(bin));
          break;
        }
      }
    }
    file->Close();
  }
}

bool sniffChainName(std::vector<std::string>& files, const std::string& treeName,
                    std::string& chainName, std::string& dataset) {
  std::string ret("");
  if(files.size() < 1)
    return false;

  TFile *file = TFile::Open(files[0].c_str());
  TList *keyList = file->GetListOfKeys();
  TIter next(keyList);
  TKey *key = 0;
  while((key = dynamic_cast<TKey *>(next()))) {
    if(std::strncmp(key->GetClassName(), "TTree", 5) == 0) {
      std::string name(key->GetName());
      if(name.find(treeName) == 0) {
        ret = name;
        break;
      }
    }
  }
  file->Close();
  chainName = ret;
  dataset = ret.substr(treeName.length(), std::string::npos);

  return true;
}
