#include<iostream>

#include <TString.h>
#include <TTree.h>
#include <TH2F.h>
#include <TCanvas.h>
#include <TLatex.h>

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

  TCanvas *canvas = new TCanvas("csvoutput", "CSV Output", 600, 400);

  tree->SetMarkerStyle(5);
  tree->SetMarkerColor(kRed);
  if(par2.Length() > 0) {
    tree->Draw(Form("%s:%s", par2.Data(), par1.Data()));
    TH1 *histo = tree->GetHistogram();
    if(!histo)
      return;

    histo->SetTitle(Form("%s with different classifier parameters", value.Data()));
    histo->GetXaxis()->SetTitle(Form("Classifier parameter %s", par1.Data()));
    histo->GetYaxis()->SetTitle(Form("Classifier parameter %s", par2.Data()));

    float x = 0;
    float y = 0;
    float val = 0;

    tree->SetBranchAddress(par1, &x);
    tree->SetBranchAddress(par2, &y);
    tree->SetBranchAddress(value, &val);

    TLatex l;
    l.SetTextSize(0.03);
    
    Long64_t nentries = tree->GetEntries();
    for(Long64_t entry=0; entry < nentries; ++entry) {
      tree->GetEntry(entry);
    
      l.DrawLatex(x, y, Form("%.3f %%", val*100));
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
