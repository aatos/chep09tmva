// -*- c++ -*-
#ifndef CHEP09TMVA_OUTPUT_H
#define CHEP09TMVA_OUTPUT_H

#include<ostream>
#include<string>
#include<set>
#include<map>
#include<vector>

class PrintValue {
public:
  PrintValue(): type(kString), str(""), number(0), width(1) {}
  PrintValue(double value): type(kNumber), str(""), number(value), width(10) {}
  PrintValue(const std::string& value);

  bool isString() const { return type == kString; }
  bool isNumber() const { return type == kNumber; }
  unsigned int getWidth() const { return width; }

  friend std::ostream& operator<<(std::ostream&, const PrintValue&);

private:
  enum Type { kString, kNumber };

  Type type;
  std::string str;
  double number;
  unsigned width;
};

class MyOutput {
public:
  MyOutput();
  MyOutput(const std::string& file);

  void addMethod(const std::string& name);
  void addResult(const std::string& method, const std::string& column, double value);
  void setComment(const std::string& column, const std::string& comment);

  void print() const;
  void print(std::ostream&) const;

  void writeFile() const;

private:
  std::string outputFile;
  std::map<std::string, std::vector<PrintValue> > methods;
  unsigned int maxMethodParams;
  std::vector<std::string> columns;

  typedef std::map<std::string, double> ColumnData;
  typedef std::map<std::string, ColumnData> ValueData;
  ValueData values;

  typedef std::map<std::string, std::string> CommentData;
  CommentData comments;
};

#endif
