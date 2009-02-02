// -*- c++ -*-
#ifndef CHEP09TMVA_CONFIG_H
#define CHEP09TMVA_CONFIG_H

#include<string>
#include<vector>
#include<map>

#include <TMVA/Types.h>

struct MyConfig {
  std::string trainer;
  std::vector<std::string> variables;
  std::vector<std::string> signalCuts;
  std::vector<std::string> bkgCuts;
  std::vector<std::string> signalTrainFiles;
  std::vector<std::string> signalTestFiles;
  std::vector<std::string> bkgTrainFiles;
  std::vector<std::string> bkgTestFiles;
  std::vector<std::string> reports;
  std::map<std::string, std::string> classifiers;
};

bool parseConf(std::string filename, MyConfig& config);
std::string stripType(const std::string& desc);
TMVA::Types::EMVA getType(std::string desc);

#endif
