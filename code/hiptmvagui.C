{
gROOT->LoadMacro("libTmvaTasks.C++"); // Compile and load the HIP TMVA GUI classes
gSystem->Load("libEve.so");

TEveManager::Create(kTRUE, "FI");

gEve->GetBrowser()->StartEmbedding(0);
tmvaBrowser = gEve->GetBrowser()->MakeFileBrowser();
setupGui(tmvaBrowser);
gEve->GetBrowser()->StopEmbedding("HIP TMVA");

gEve->GetBrowser()->StartEmbedding();
gROOT->ProcessLineFast("new TCanvas");
gEve->GetBrowser()->StopEmbedding("Canvas");

TMacro *m = new TMacro();
m->SetName("openOutputFile");
tmvaBrowser->Add(m);

TMacro *closeOutputFile = new TMacro();
closeOutputFile->SetName("closeOutputFile");
tmvaBrowser->Add(closeOutputFile);
}
