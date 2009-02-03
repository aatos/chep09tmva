{
   // This macro shows a control bar o run some of the ROOT tutorials.
   // To execute an item, click with the left mouse button.

   gROOT->Reset();

   //Add the tutorials directory to the macro path
   //This is necessary in case this macro is executed from another user directory
   TString dir = gSystem->UnixPathName(gInterpreter->GetCurrentMacroName());
   dir.ReplaceAll("demos.C","");
   dir.ReplaceAll("/./","");
   const char *current = gROOT->GetMacroPath();
   gROOT->SetMacroPath(Form("%s:%s",current,dir.Data()));
   
   TControlBar *bar = new TControlBar("vertical", "Demos",10,10);
   bar->AddButton("Object Browser",   "new TBrowser;",         "Start the ROOT Browser");
//   bar->AddButton("Help Demos",".x demoshelp.C",        "Click Here For Help");
   bar->AddButton("Run test",     "gSystem->Exec(\"make test\");",   "Test analysis system");
   bar->AddButton("Analysis AH",     "gSystem->Exec(\"make ah1\");",   "Run analysis by Aatos");
   bar->AddButton("CHEP'09 TMVA paper",     "gSystem->Exec(\"cd ..; make \");",   "Show CHEP'09 TMVA paper");
   bar->AddButton("Quit",   ".q", "Quit analysis");
   bar->SetButtonWidth(90);
   bar->Show();
   gROOT->SaveContext();
}
