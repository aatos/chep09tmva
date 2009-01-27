#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<cstring>
#include<algorithm>

#include <TFile.h>
#include <TChain.h>
#include <TString.h>
#include <TSystem.h>

#include <TMVA/Config.h>
#include <TMVA/Factory.h>
#include <TMVA/MethodBase.h>
#include <TMVA/MsgLogger.h>

bool parseConf(std::string filename, std::vector<std::string>& variables,
               std::vector<std::string>& signalCuts, std::vector<std::string>& bkgCuts,
               std::vector<std::string>& signalFiles, std::vector<std::string>& bkgFiles,
               std::string& trainer,
               std::vector<std::pair<std::string, std::string> >& classifiers);
std::vector<std::string> preprocessLine(std::string& line);
TMVA::Types::EMVA getType(std::string desc);
void print_usage(void);

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
  MyFactory(TString theJobName, TFile* theTargetFile, TString theOption = ""):
    TMVA::Factory(theJobName, theTargetFile, theOption)
  {}

  virtual void printEfficiency(std::vector<std::string> methods, double signalEventSelEff, double bkgEventSelEff,
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

};

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

  std::vector<std::string> variables;
  std::vector<std::string> signalCuts;
  std::vector<std::string> bkgCuts;
  std::vector<std::string> signalFiles;
  std::vector<std::string> bkgFiles;
  std::vector<std::pair<std::string, std::string> > classifiers;
  std::vector<std::string> classifier_names;
  std::string trainer;

  // Cuts
  TCut signalCut_s = "";
  TCut bkgCut_s = "";

  if(!parseConf(confFile, variables, signalCuts, bkgCuts, signalFiles, bkgFiles, trainer, classifiers))
    return 1;

  if(signalFiles.size() == 0) {
    std::cout << "SignalFiles is empty!" << std::endl;
    return 1;
  }
  else if(bkgFiles.size() == 0) {
    std::cout << "BackgroundFiles is empty!" << std::endl;
    return 1;
  }

  std::cout << "Variables:" << std::endl;
  for(std::vector<std::string>::const_iterator iter = variables.begin();
      iter != variables.end(); ++iter) {
    std::cout << *iter << std::endl;
  }
  std::cout << std::endl;

  std::cout << "Cuts (applied to signal)" << std::endl;
  for(std::vector<std::string>::const_iterator iter = signalCuts.begin();
      iter != signalCuts.end(); ++iter) {
    std::cout << *iter << std::endl;
    signalCut_s = signalCut_s && TCut(iter->c_str());
  }
  //std::cout << cut_s << std::endl;
  std::cout << std::endl;

  std::cout << "Cuts (applied to background)" << std::endl;
  for(std::vector<std::string>::const_iterator iter = bkgCuts.begin();
      iter != bkgCuts.end(); ++iter) {
    std::cout << *iter << std::endl;
    bkgCut_s = bkgCut_s && TCut(iter->c_str());
  }
  //std::cout << cut_s << std::endl;
  std::cout << std::endl;
  
  std::cout << "Trainer: " << trainer << std::endl << std::endl;
  std::cout << "Classifiers:" << std::endl;
  for(std::vector<std::pair<std::string, std::string> >::const_iterator iter = classifiers.begin();
      iter != classifiers.end(); ++iter) {
    std::cout << iter->first << ": " << iter->second << std::endl;
    classifier_names.push_back(iter->first);
  }
  std::cout << std::endl;

  // Input TChains
  TChain *signalChain = new TChain("TauID_Pythia8_generatorLevel_HCh300");
  TChain *bkgChain = new TChain("TauID_Pythia8_generatorLevel_QCD_120_170");

  long signalEntries = 0;
  long bkgEntries = 0;

  std::cout << "Files for signal TChain" << std::endl;
  for(std::vector<std::string>::const_iterator iter = signalFiles.begin();
      iter != signalFiles.end(); ++iter) {
    std::cout << *iter << std::endl;
    signalChain->AddFile(iter->c_str());
  }
  signalEntries = signalChain->GetEntries();
  std::cout << "Chain has " << signalEntries << " entries" << std::endl << std::endl;
  
  std::cout << "Files for background TChain" << std::endl;
  for(std::vector<std::string>::const_iterator iter = bkgFiles.begin();
      iter != bkgFiles.end(); ++iter) {
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
  for(std::vector<std::string>::const_iterator iter = variables.begin();
      iter != variables.end(); ++iter) {
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
  factory->PrepareTrainingAndTestTree(signalCut_s, bkgCut_s, trainer);

  // Book MVA methods
  for(std::vector<std::pair<std::string, std::string> >::const_iterator iter = classifiers.begin();
      iter != classifiers.end(); ++iter) {
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
    fac->printEfficiency(classifier_names, signalEventSelEff, backgroundEventSelEff, signalEntries, bkgEntries);
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

TMVA::Types::EMVA getType(std::string desc) {
  std::string type("");
  size_t t = desc.find("_");
  if(t < desc.length())
    type = desc.substr(0, t);
  else
    type = desc;

  if(type == "Cuts")
    return TMVA::Types::kCuts;
  else if(type == "Likelihood")
    return TMVA::Types::kLikelihood;
  else if(type == "PDERS")
    return TMVA::Types::kPDERS;
  else if(type == "HMatrix")
    return TMVA::Types::kHMatrix;
  else if(type == "Fisher")
    return TMVA::Types::kFisher;
  else if(type == "KNN")
    return TMVA::Types::kKNN;
  else if(type == "CFMlpANN")
    return TMVA::Types::kCFMlpANN;
  else if(type == "TMlpANN")
    return TMVA::Types::kTMlpANN;
  else if(type == "MLP")
    return TMVA::Types::kMLP;
  else if(type == "BDT")
    return TMVA::Types::kBDT;
  else if(type == "RuleFit")
    return TMVA::Types::kRuleFit;
  else if(type == "SVM")
    return TMVA::Types::kSVM;
  else if(type == "FDA")
    return TMVA::Types::kFDA;
  else {
    std::cout << "Unkown classifer type " << type << std::endl;
    exit(1);
  }
}

bool parseConf(std::string filename, std::vector<std::string>& variables,
               std::vector<std::string>& signalCuts, std::vector<std::string>& bkgCuts, 
               std::vector<std::string>& signalFiles, std::vector<std::string>& bkgFiles,
               std::string& trainer,
               std::vector<std::pair<std::string, std::string> >& classifiers) {
  variables.clear();
  classifiers.clear();
  trainer = "";

  std::ifstream file(filename.c_str());
  if(!file) {
    std::cout << "File " << filename << " does not exist" << std::endl;
    return false;
  }

  std::string line;

  enum mode_t {kNone, kVar, kSignalCut, kBkgCut, kSignalFiles, kBkgFiles, kTrain, kClass};
  mode_t mode = kNone;
  int lineno = 0;

  while(! file.eof()) {
    std::getline(file, line);
    ++lineno;

    std::vector<std::string> parsed = preprocessLine(line);
    if(parsed.size() == 0) continue;

    if(parsed[0] == "include") {
      if(mode != kNone) {
        std::cout << "Include should be the first directive in the config file" << std::endl;
        return false;
      }

      if(parsed.size() != 2) {
        std::cout << "Include should be used like 'include file.conf' instead of '" << line << "'" << std::endl;
        return false;
      }
      if(!parseConf(parsed[1], variables, signalCuts, bkgCuts, signalFiles, bkgFiles, trainer, classifiers))
        return false;
    }

    if(line == "Variables:") {
      mode = kVar;
      variables.clear();
    }
    else if(line == "SignalCuts:") {
      mode = kSignalCut;
      signalCuts.clear();
    }
    else if(line == "BackgroundCuts:") {
      mode = kBkgCut;
      bkgCuts.clear();
    }
    else if(line == "SignalFiles:") {
      mode = kSignalFiles;
      signalFiles.clear();
    }
    else if(line == "BackgroundFiles:") {
      mode = kBkgFiles;
      bkgFiles.clear();
    }
    else if(line == "Trainer:") {
      mode = kTrain;
      trainer = "";
    }
    else if(line == "Classifiers:") {
      mode = kClass;
      classifiers.clear();
    }
    else if(mode == kVar) {
      if(parsed.size() != 1) {
        std::cout << "Parse error at line " << lineno << ": \"" << line << "\"" << std::endl;
        std::cout << "Expected only one string at line" << std::endl;
        return false;
      }
      variables.push_back(parsed[0]);
    }
    else if(mode == kSignalCut) {
      if(parsed.size() > 0) {
        std::string s(parsed[0]);
        for(std::vector<std::string>::const_iterator iter = parsed.begin()+1; iter != parsed.end(); ++iter) {
          s += " ";
          s += *iter;
        }
        signalCuts.push_back(s);
      }
    }
    else if(mode == kBkgCut) {
      if(parsed.size() > 0) {
        std::string s(parsed[0]);
        for(std::vector<std::string>::const_iterator iter = parsed.begin()+1; iter != parsed.end(); ++iter) {
          s += " ";
          s += *iter;
        }
        bkgCuts.push_back(s);
      }
    }
    else if(mode == kSignalFiles) {
      if(parsed.size() != 1) {
        std::cout << "Parse error at line " << lineno << ": \"" << line << "\"" << std::endl;
        std::cout << "Expected only one string at line" << std::endl;
        return false;
      }
      signalFiles.push_back(parsed[0]);
    }
    else if(mode == kBkgFiles) {
      if(parsed.size() != 1) {
        std::cout << "Parse error at line " << lineno << ": \"" << line << "\"" << std::endl;
        std::cout << "Expected only one string at line" << std::endl;
        return false;
      }
      bkgFiles.push_back(parsed[0]);
    }
    else if(mode == kTrain) {
      if(parsed.size() != 1) {
        std::cout << "Parse error at line " << lineno << ": \"" << line << "\"" << std::endl;
        std::cout << "Expected only one string at line" << std::endl;
        return false;
      }
      trainer = parsed[0];
    }
    else if(mode == kClass) {
      if(parsed.size() != 2) {
        std::cout << "Parse error at line " << lineno << ": \"" << line << "\"" << std::endl;
        std::cout << "Expected two strings at line" << std::endl;
        return false;
      }
      classifiers.push_back(std::make_pair(parsed[0], parsed[1]));
    }
  }
  file.close();
  
  return true;
}

std::vector<std::string> preprocessLine(std::string& line) {
  std::vector<std::string> ret;

  std::string temp("");

  // preprocess comments
  size_t comment = line.find("//");
  if(comment < line.length())
    line = line.substr(0, comment);

  // split by whitespaces
  for(unsigned int i=0; i<line.length(); ++i) {
    if(std::isspace(line[i]) && temp.length() > 0) {
      ret.push_back(temp);
      temp = "";
    }
    else if(!std::isspace(line[i]))
      temp += line[i];
  }
  if(temp.length() > 0)
    ret.push_back(temp);

  return ret;
}
