#include "output.h"

#include<iostream>
#include<sstream>
#include<fstream>
#include<algorithm>
#include<vector>
#include<iomanip>
#include<cstring>
#include<cmath>

PrintValue::PrintValue(const std::string& value): type(kString), str(""), number(0), width(1) {
  std::stringstream ss(value);
  if(ss >> number) {
    type = kNumber;
    width = value.length();
  }
  else {
    str = value;
    type = kString;
    width = value.length();
  }
}

MyOutput::MyOutput(): outputFile("tmva-mvaresults.txt"), maxMethodParams(0) {}
MyOutput::MyOutput(std::string file): outputFile(file), maxMethodParams(0) {}

void MyOutput::addMethod(std::string name) {
  size_t start = 0;
  size_t stop = 0;
  std::vector<PrintValue> params;
  while(stop != std::string::npos) {
    stop =name.find_first_of('_', start+1);
    std::string sub = name.substr(start, stop-start);

    params.push_back(PrintValue(sub));
    start = stop+1;
  }

  maxMethodParams = std::max(maxMethodParams, params.size());

  methods.insert(std::make_pair(name, params));
}

void MyOutput::addResult(std::string method, std::string column, double value) {
  ValueData::iterator columns = values.find(method);
  if(columns == values.end()) {
    ColumnData data;
    data.insert(std::make_pair(column, value));
    values.insert(std::make_pair(method, data));
  }
  else {
    columns->second[column] = value;
  }
  if(std::find(this->columns.begin(), this->columns.end(), column) == this->columns.end()) 
    this->columns.push_back(column);
}

std::ostream& operator<<(std::ostream& s, const PrintValue& val) {
  switch(val.type) {
  case PrintValue::kString:
    s << val.str;
    break;
  case PrintValue::kNumber:
    s << val.number;
    break;
  default:
    s << "N/A";
    break;
  };
  return s;
}

void MyOutput::print() {
  print(std::cout);
}

void MyOutput::writeFile() {
  std::ofstream output(outputFile.c_str());
  print(output);
  output.close();
}

void MyOutput::print(std::ostream& out) {
  std::vector<bool> columnIsString;
  std::vector<unsigned int> columnWidth;

  columnIsString.resize(maxMethodParams+columns.size());
  columnWidth.resize(columnIsString.size());

  columnIsString[0] = true;
  for(unsigned int i=1; i<columnIsString.size(); ++i) {
    columnIsString[i] = false;
  }

  columnWidth[0] = std::strlen("Method");
  for(unsigned int i=1; i<maxMethodParams; ++i) {
    columnWidth[i] = 3+int(log(double(i))+1);
  }
  for(unsigned int i=0; i<columns.size(); ++i) {
    columnWidth[i+maxMethodParams] = columns[i].length();
  }

  std::vector<std::vector<PrintValue> > lines;
  for(std::map<std::string, std::vector<PrintValue> >::const_iterator method = methods.begin(); method != methods.end(); ++method) {
    std::vector<PrintValue> line;

    // Method names
    for(std::vector<PrintValue>::const_iterator iter = method->second.begin(); iter != method->second.end(); ++iter) {
      if(iter->isString()) {
        columnIsString[line.size()] = true;
      }
      columnWidth[line.size()] = std::max(columnWidth[line.size()], iter->getWidth());

      line.push_back(*iter);
    }
    for(unsigned int i=method->second.size(); i<maxMethodParams; ++i) {
      columnWidth[line.size()] = std::max(columnWidth[line.size()], unsigned(3));
      columnIsString[line.size()] = true;
      line.push_back(PrintValue("N/A"));
    }

    // Data columns
    ValueData::const_iterator iValue = values.find(method->first);
    if(iValue == values.end()) {
      for(unsigned i=0; i<columns.size(); ++i) {
        line.push_back(PrintValue(0));
      }
    }
    else {
      for(std::vector<std::string>::const_iterator column = columns.begin(); column != columns.end(); ++column) {
        ColumnData::const_iterator iColumn = iValue->second.find(*column);
        if(iColumn == iValue->second.end()) {
          line.push_back(PrintValue(0));
        }
        else {
          line.push_back(PrintValue(iColumn->second));
        }
      }
    }

    lines.push_back(line);
  }

  // Compulsory line for TTree::ReadFile
  out << "method/C";
  for(unsigned int i=1; i<maxMethodParams; ++i) {
    out << ":par" << i << (columnIsString[i] ? "/C":"/F");
  }
  for(unsigned int i=0; i<columns.size(); ++i) {
    out << ":" << columns[i] << (columnIsString[i+maxMethodParams] ? "/C":"/F");
  }
  out << std::endl;

  // My own header
  out << "########################################" << std::endl
      << "#" << std::endl
      << "# How to use in ROOT: " << std::endl
      << "#" << std::endl
    //<< "# TTree *tree = new TTree(\"data\", \"data\")" << std::endl
    //<< "# tree->ReadFile(\"" << outputFile << "\")" << std::endl
    //<< "# tree->Draw(\"eventEff:par1:par2 >>histo\", \"\", \"goff\")" << std::endl
    //<< "# histo->Project3DProfile(\"xy\")->Draw(\"TEXT\")" << std::endl
      << "# root -l scripts/csv.C" << std::endl
      << "# " << std::endl
      << "# " << std::setw(columnWidth[0]) << "Method" << " ";
  std::stringstream ss;
  for(unsigned int i=1; i<maxMethodParams; ++i) {
    ss.str("");
    ss << "Par" << i;
    out << std::setw(columnWidth[i]) << std::left << ss.str() << " ";
  }
  for(unsigned int i=0; i<columns.size(); ++i) {
    out << std::setw(columnWidth[i+maxMethodParams]) << columns[i] << " ";
  }
  out << std::endl;

  // Actual data
  for(std::vector<std::vector<PrintValue> >::const_iterator iLine = lines.begin(); iLine != lines.end(); ++iLine) {
    out << "  ";
    for(std::vector<PrintValue>::const_iterator iColumn = iLine->begin(); iColumn != iLine->end(); ++iColumn) {
      out << std::setw(columnWidth[iColumn-iLine->begin()]) << std::left << *iColumn << " ";
    }
    out << std::endl;
  }
}
