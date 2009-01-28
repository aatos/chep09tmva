#include "config.h"

#include<fstream>

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

