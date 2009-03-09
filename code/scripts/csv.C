#include<iostream>

#include <TString.h>
#include <TTree.h>
#include <TH2F.h>
#include <TCanvas.h>
#include <TLatex.h>

int textColor(double value, double max, double min) {
  if(max > min) {
    double f = (value-min)/(max-min);
    int ncolors = gStyle->GetNumberOfColors();
    return gStyle->GetColorPalette(int(f*(ncolors-1)));
  }
  else {
    return 1;
  }
}

void csv(TString input="tmva.csvoutput.txt", TString par1="par2", TString par2="par3", TString par3="", TString value="eventEffScaled_5") {
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
    if(par3.Length() > 0)
      tree->Draw(Form("%s:%s:%s:%s", par1.Data(), par2.Data(), par3.Data(), value.Data()), "", "COLZ"); //, "", "Z");
    else
      tree->Draw(Form("%s:%s:%s", par2.Data(), par1.Data(), value.Data()), "", "COLZ"); //, "", "Z");

    TH1 *histo = tree->GetHistogram();
    if(!histo)
      return;

    histo->SetTitle(Form("%s with different classifier parameters", value.Data()));
    histo->GetXaxis()->SetTitle(Form("Classifier parameter %s", par1.Data()));
    histo->GetYaxis()->SetTitle(Form("Classifier parameter %s", par2.Data()));
    if(par3.Length() > 0)
      histo->GetZaxis()->SetTitle(Form("Classifier parameter %s", par3.Data()));
    else
      histo->GetZaxis()->SetTitle("");

    if(par3.Length() == 0) {
      float x = 0;
      float y = 0;
      float val = 0;
      double maxVal = tree->GetMaximum(value);
      double minVal = tree->GetMinimum(value);

      tree->SetBranchAddress(par1, &x);
      tree->SetBranchAddress(par2, &y);
      tree->SetBranchAddress(value, &val);
      TLatex l;
      l.SetTextSize(0.03);
    
      Long64_t nentries = tree->GetEntries();
      for(Long64_t entry=0; entry < nentries; ++entry) {
        tree->GetEntry(entry);
    
        l.SetTextColor(textColor(val, maxVal, minVal));
        l.DrawLatex(x, y, Form("%.3f", val*100));
      }
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
