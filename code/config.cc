#include "config.h"

#include<fstream>

std::string stripType(const std::string& desc) {
  std::string type("");
  size_t t = desc.find("_");
  if(t < desc.length())
    type = desc.substr(0, t);
  else
    type = desc;

  return type;
}

TMVA::Types::EMVA getType(std::string desc) {
  std::string type = stripType(desc);

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

bool parseConf(std::string filename, MyConfig& config) {
  config.variables.clear();
  config.classifiers.clear();
  config.trainer = "";

  std::ifstream file(filename.c_str());
  if(!file) {
    std::cout << "File " << filename << " does not exist" << std::endl;
    return false;
  }

  std::string line;

  enum mode_t {kNone, kVar, kSignalCut, kBkgCut, kSignalTrainFiles, kSignalTestFiles, kBkgTrainFiles, kBkgTestFiles, kTrain, kClass};
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
      if(!parseConf(parsed[1], config))
        return false;
    }

    if(line == "Variables:") {
      mode = kVar;
      config.variables.clear();
    }
    else if(line == "SignalCuts:") {
      mode = kSignalCut;
      config.signalCuts.clear();
    }
    else if(line == "BackgroundCuts:") {
      mode = kBkgCut;
      config.bkgCuts.clear();
    }
    else if(line == "SignalTrainFiles:") {
      mode = kSignalTrainFiles;
      config.signalTrainFiles.clear();
    }
    else if(line == "SignalTestFiles:") {
      mode = kSignalTestFiles;
      config.signalTestFiles.clear();
    }
    else if(line == "BackgroundTrainFiles:") {
      mode = kBkgTrainFiles;
      config.bkgTrainFiles.clear();
    }
    else if(line == "BackgroundTestFiles:") {
      mode = kBkgTestFiles;
      config.bkgTestFiles.clear();
    }
    else if(line == "Trainer:") {
      mode = kTrain;
      config.trainer = "";
    }
    else if(line == "Classifiers:") {
      mode = kClass;
      config.classifiers.clear();
    }
    else if(mode == kVar) {
      if(parsed.size() != 1) {
        std::cout << "Parse error at line " << lineno << ": \"" << line << "\"" << std::endl;
        std::cout << "Expected only one string at line" << std::endl;
        return false;
      }
      config.variables.push_back(parsed[0]);
    }
    else if(mode == kSignalCut) {
      if(parsed.size() > 0) {
        std::string s(parsed[0]);
        for(std::vector<std::string>::const_iterator iter = parsed.begin()+1; iter != parsed.end(); ++iter) {
          s += " ";
          s += *iter;
        }
        config.signalCuts.push_back(s);
      }
    }
    else if(mode == kBkgCut) {
      if(parsed.size() > 0) {
        std::string s(parsed[0]);
        for(std::vector<std::string>::const_iterator iter = parsed.begin()+1; iter != parsed.end(); ++iter) {
          s += " ";
          s += *iter;
        }
        config.bkgCuts.push_back(s);
      }
    }
    else if(mode == kSignalTrainFiles) {
      if(parsed.size() != 1) {
        std::cout << "Parse error at line " << lineno << ": \"" << line << "\"" << std::endl;
        std::cout << "Expected only one string at line" << std::endl;
        return false;
      }
      config.signalTrainFiles.push_back(parsed[0]);
    }
    else if(mode == kSignalTestFiles) {
      if(parsed.size() != 1) {
        std::cout << "Parse error at line " << lineno << ": \"" << line << "\"" << std::endl;
        std::cout << "Expected only one string at line" << std::endl;
        return false;
      }
      config.signalTestFiles.push_back(parsed[0]);
    }
    else if(mode == kBkgTrainFiles) {
      if(parsed.size() != 1) {
        std::cout << "Parse error at line " << lineno << ": \"" << line << "\"" << std::endl;
        std::cout << "Expected only one string at line" << std::endl;
        return false;
      }
      config.bkgTrainFiles.push_back(parsed[0]);
    }
    else if(mode == kBkgTestFiles) {
      if(parsed.size() != 1) {
        std::cout << "Parse error at line " << lineno << ": \"" << line << "\"" << std::endl;
        std::cout << "Expected only one string at line" << std::endl;
        return false;
      }
      config.bkgTestFiles.push_back(parsed[0]);
    }
    else if(mode == kTrain) {
      if(parsed.size() != 1) {
        std::cout << "Parse error at line " << lineno << ": \"" << line << "\"" << std::endl;
        std::cout << "Expected only one string at line" << std::endl;
        return false;
      }
      config.trainer = parsed[0];
    }
    else if(mode == kClass) {
      if(parsed.size() != 2) {
        std::cout << "Parse error at line " << lineno << ": \"" << line << "\"" << std::endl;
        std::cout << "Expected two strings at line" << std::endl;
        return false;
      }
      config.classifiers.insert(std::make_pair(parsed[0], parsed[1]));
    }
  }
  file.close();
  
  return true;
}

