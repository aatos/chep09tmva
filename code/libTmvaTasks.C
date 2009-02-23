#include "TROOT.h"
#include "TTask.h"
#include "TFolder.h"
#include "TGFileBrowser.h"
#include "Riostream.h"

class HIPTMVARun : public TTask {
public:
  HIPTMVARun() { ; }
  HIPTMVARun(const char *name,const char *title);
  virtual ~HIPTMVARun() { ; }
  void Exec(Option_t *option="");
  Bool_t IsFolder() const;
  ClassDef(HIPTMVARun,1) // Run the analysis
};

ClassImp(HIPTMVARun)

HIPTMVARun::HIPTMVARun(const char *name, const char *title)
  :TTask(name, title)
{
}

void HIPTMVARun::Exec(Option_t *option)
{
  TString command = Form("gSystem->Exec(\"./chep09tmva %s\")", GetName());
  cout << "Running command: " << command <<endl;
  gROOT->ProcessLine(command);
}

Bool_t HIPTMVARun::IsFolder() const
{
  return kFALSE;
}

class HIPTMVARunList : public TTask {
public:
  HIPTMVARunList() { ; }
  HIPTMVARunList(const char *name,const char *title);
  virtual ~HIPTMVARunList() { ; }
  void Exec(Option_t *option="") { ; }

  ClassDef(HIPTMVARunList, 1)
};

ClassImp(HIPTMVARunList)

HIPTMVARunList::HIPTMVARunList(const char *name, const char *title)
  :TTask(name, title)
{
  Add(new HIPTMVARun("tmva-aatos.conf", "tmva-aatos.conf"));
  Add(new HIPTMVARun("tmva-matti.conf", "tmva-matti.conf"));
  Add(new HIPTMVARun("tmva-pekka.conf", "tmva-pekka.conf"));
}

void setupGui(TGFileBrowser *b)
{
  TFolder *hipTMVA = new TFolder("HIPTMVA", "HIP TMVA GUI");
  //  gROOT->GetListOfBrowsables()->Add(hipTMVA);
  b->Add(hipTMVA);
  //  TFolder *runMacros = hipTMVA->AddFolder("runConfigurations", "HIPTMVARun configurations"); 
  hipTMVA->Add(new HIPTMVARunList("Configurations", "Run configurations"));
  TFolder *results = hipTMVA->AddFolder("results", "Results (TMVA.root)"); 
  //  runMacros->Add(new HIPTMVARun("Menu", "HIP TMVA Menu system"));
}
//}
