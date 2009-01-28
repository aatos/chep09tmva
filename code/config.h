// -*- c++ -*-
#ifndef CHEP09TMVA_CONFIG_H
#define CHEP09TMVA_CONFIG_H

#include<string>
#include<vector>

#include <TMVA/Types.h>

bool parseConf(std::string filename, std::vector<std::string>& variables,
               std::vector<std::string>& signalCuts, std::vector<std::string>& bkgCuts,
               std::vector<std::string>& signalFiles, std::vector<std::string>& bkgFiles,
               std::string& trainer,
               std::vector<std::pair<std::string, std::string> >& classifiers);
TMVA::Types::EMVA getType(std::string desc);

#endif
