{

gROOT->LoadMacro("libTmvaTasks.C++"); // Compile and load the HIP TMVA GUI classes
gSystem->Load("libEve.so");

TEveManager::Create();

gEve->GetBrowser()->StartEmbedding(0);
TGFileBrowser *tmvaBrowser = gEve->GetBrowser()->MakeFileBrowser();
//HIPTMVA::GuiFactory::buildGui(tmvaBrowser);
setupGui(tmvaBrowser);
gEve->GetBrowser()->StopEmbedding("HIP TMVA");

}
