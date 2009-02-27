#include<iostream>

#include <TString.h>
#include <TTree.h>
#include <TH2F.h>
#include <TCanvas.h>
#include <TLatex.h>

int textColor(double value, double max) {
  double f = value/max;
  int ncolors = gStyle->GetNumberOfColors();
  return gStyle->GetColorPalette(int(f*(ncolors-1)));
}

void csv(TString input="tmva.csvoutput.txt", TString par1="par2", TString par2="par3", TString value="eventEffScaled") {
  std::cout << "Usage:" << std::endl
            << ".x scripts/csv.C    with default arguments" << std::endl
            << ".x scripts/csv.C(filename, par1, par2, value)" << std::endl
            << std::endl
            << "  Optional arguments:" << std::endl
            << "    filename        path to CSV file" << std::endl
            << "    par1            name of X-parameter branch" << std::endl
            << "    par2            name of Y-parameter branch (if empty, efficiency is drawn as a function of par1)" << std::endl
            << "    value           name of result (efficiency) branch" << std::endl
            << std::endl;

  TTree *tree = new TTree("data", "data");
  tree->ReadFile(input);

  gStyle->SetPalette(1);
  gStyle->SetPadRightMargin(0.14);

  TCanvas *canvas = new TCanvas("csvoutput", "CSV Output", 1200, 900);

  tree->SetMarkerStyle(kFullDotMedium);
  tree->SetMarkerColor(kRed);
  if(par2.Length() > 0) {
    //tree->Draw(Form("%s:%s", par2.Data(), par1.Data()));
    tree->Draw(Form("%s:%s:%s", par2.Data(), par1.Data(), value.Data()), "", "COLZ"); //, "", "Z");
    TH1 *histo = tree->GetHistogram();
    if(!histo)
      return;

    histo->SetTitle(Form("%s with different classifier parameters", value.Data()));
    histo->GetXaxis()->SetTitle(Form("Classifier parameter %s", par1.Data()));
    histo->GetYaxis()->SetTitle(Form("Classifier parameter %s", par2.Data()));
    histo->GetZaxis()->SetTitle("");

    float x = 0;
    float y = 0;
    float val = 0;
    double maxVal = tree->GetMaximum(value);

    tree->SetBranchAddress(par1, &x);
    tree->SetBranchAddress(par2, &y);
    tree->SetBranchAddress(value, &val);

    TLatex l;
    l.SetTextSize(0.03);
    
    Long64_t nentries = tree->GetEntries();
    for(Long64_t entry=0; entry < nentries; ++entry) {
      tree->GetEntry(entry);
    
      l.SetTextColor(textColor(val, maxVal));
      l.DrawLatex(x, y, Form("%.3f", val*100));
    }
  }
  else {
    tree->Draw(Form("%s:%s", value.Data(), par1.Data()));
    TH1 *histo = tree->GetHistogram();
    if(!histo) 
      return;
    histo->SetTitle(Form("%s with different classifier parameters", value.Data()));
    histo->GetXaxis()->SetTitle(Form("Classifier parameter %s", par1.Data()));
    histo->GetYaxis()->SetTitle(value);
  }
}
