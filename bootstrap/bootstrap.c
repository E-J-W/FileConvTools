#include "bootstrap.h"
#include "read_config.c"

int main(int argc, char *argv[])
{

  //Creating an instance of TApplication
  //This is evidently needed for auto-loading of ROOT libraries, da
  //otherwise the program may crash upon execution depending on how ROOT
  //is set up.
  //http://root.cern.ch/phpBB3/viewtopic.php?f=3&t=14064
  /* int ac; */
  /* char* av[10]; */
  /* theApp = new TApplication("App", &ac, av); // why the hell doesn't this work as a pointer?!?! */

  randGen = new TRandom2();  

  FILE *expData;
  TFile *inp;
  TH1D *h2 = new TH1D("h2","h2",500,0,2000);
  
  if(argc!=2)
    {
      printf("\nbootstrap parameter_file\n");
      printf("-----------------------\nPerformes a bootstrap analysis to determine the distribution of the likelihood ratio method statistic.\n\n");
      exit(-1);
    }

  //initialize values
  addBackground = 0;
  for(int i=0;i<NSPECT;i++)
    for(int j=0;j<S32K;j++)
      expHist[i][j] = 0.;
  
  for(int i=0;i<3;i++)
    for(int j=0;j<NSPECT;j++)
      aFinal[i][j] = 0.;
  
  chisq=0.;
  
  // initialize ROOT stuff
  char dataName[132];
  for(int i=0;i<NSPECT;i++)
    {
      sprintf(dataName,"data_%2d",i);
      data[i] = new TH1D(dataName,";;",S32K,0,S32K-1);
    }
  
  readConfigFile(argv[1],"bootstrap"); //grab data from the parameter file
  /* getc(stdin); */

  // read in experimental spectra
  // check file extension of exp data and copy into float histogram
  if((expData=fopen(expDataName,"r"))==NULL)
    {
      printf("ERROR: Cannot open the experiment data file %s!\n",expDataName);
      exit(-1);
    }
  const char *dot = strrchr(expDataName, '.'); // get the file extension
  if(strcmp(dot + 1,"mca")==0)
    readMCA(expData,expDataName,expHist);
  else if(strcmp(dot + 1,"fmca")==0)
    readFMCA(expData,expDataName,expHist);
  else
    {
      printf("ERROR: Improper type of input file: %s\n",expDataName);
      printf("Integer array (.mca) and float array (.fmca) files are supported.\n");
      exit(-1);
    }
 
  // get the information from the tree
  // open tree
  inp = new TFile(inp_filename,"read");
  if((btree = (TTree*)inp->Get(sort_tree_name))==NULL)
    {
      printf("The specified tree named %s doesn't exist, trying default name 'tree'.\n",sort_tree_name);
      if((btree = (TTree*)inp->Get("tree"))==NULL) // try the default tree name
	{
	  printf("ERROR: The specified tree named %s (within the ROOT file) cannot be opened!\n",sort_tree_name);
	  exit(-1);
	}
    }
  // sort leaf (energy)
  if((sortLeaf = btree->GetLeaf(sort_path))==NULL)
    if((sortBranch = btree->GetBranch(sort_path))==NULL)
      {
	printf("ERROR: Sort data path '%s' doesn't correspond to a branch or leaf in the tree!\n",sort_path);
	exit(-1);
      }
  if(sortLeaf==NULL)
    sortLeaf = (TLeaf*)sortBranch->GetListOfLeaves()->First(); // get the first leaf from the specified branch  
  
  // weight leaf
  if((weightLeaf = btree->GetLeaf(weight_path))==NULL)
    if((weightBranch = btree->GetBranch(weight_path))==NULL)
      {
	printf("ERROR: Weight data path '%s' doesn't correspond to a branch or leaf in the tree!\n",weight_path);
	exit(-1);
      }
  if(weightLeaf==NULL)
    weightLeaf = (TLeaf*)weightBranch->GetListOfLeaves()->First(); // get the first leaf from the specified branch  
  
  // position leaf
  if((posLeaf = btree->GetLeaf(pos_path))==NULL)
    if((posBranch = btree->GetBranch(pos_path))==NULL)
      {
	printf("ERROR: Pos data path '%s' doesn't correspond to a branch or leaf in the tree!\n",pos_path);
	exit(-1);
      }
  if(posLeaf==NULL)
    posLeaf = (TLeaf*)posBranch->GetListOfLeaves()->First(); // get the first leaf from the specified branch  
  
  // color leaf
  if((colLeaf = btree->GetLeaf(col_path))==NULL)
    if((colBranch = btree->GetBranch(col_path))==NULL)
      {
	printf("ERROR: Col data path '%s' doesn't correspond to a branch or leaf in the tree!\n",col_path);
	exit(-1);
      }
  if(colLeaf==NULL)
    colLeaf = (TLeaf*)colBranch->GetListOfLeaves()->First(); //get the first leaf from the specified branch  
  
  // csi leaf
  if((csiLeaf = btree->GetLeaf(csi_path))==NULL)
    if((csiBranch = btree->GetBranch(csi_path))==NULL)
      {
	printf("ERROR: CsI data path '%s' doesn't correspond to a branch or leaf in the tree!\n",csi_path);
	exit(-1);
      }
  if(csiLeaf==NULL)
    csiLeaf = (TLeaf*)csiBranch->GetListOfLeaves()->First(); // get the first leaf from the specified branch  
  // done reading tree
  
  // read groups for sorting
  readGroupMap();

  // number of events
  UInt_t treeEntries = (UInt_t)btree->GetEntries();

  // bootstrap analysis
  int index = 0;
  std::cout << "Generating "<< numCompare << " random spectra with " << numEntries << " entries/spectrum" << std::endl;
  while(index < numCompare)
    {
      Int_t entryCt = 0;
      while(entryCt < numEntries)
	{
	  UInt_t evt = randGen->Integer(treeEntries);

	  // add event to analysis histogram
	  addEventToOutHist(evt);

	  // increment loop
	  entryCt++;

	  // track progress
	  if(entryCt%100 == 0)
	    {
	      Double_t percent = 100.*(((Double_t)index*(Double_t)numEntries + (Double_t)entryCt)/((Double_t)numCompare*(Double_t)numEntries));
	      std::cout << "End of processing index " << index << " entry " << entryCt << "; " 
			<< std::fixed << percent << "% complete...\r" << std::flush;
	    }
	}
      
      // analyze data, get and histo lr
      find_chisqMin();
      h2->Fill(chisq);
      /* printf("\n\nchisq %.15f\n\n",chisq); */
      
      // plot results
      if(plotOutput>=1)
	plotSpectra();
      
      index++;
    }

  // close ROOT file
  inp->Close(); 

  // display histo of chisqs
  /* h2->Draw(); */
  /* theApp->Run(kTRUE); */
  
  // can save to a file
  char title[132];
  sprintf(title,"bootstrap_output.root");
  TFile out(title, "recreate");
  h2->Write();

  std::cout << std::endl;

  return 0;
}

/*******************************************************************************/
void addEventToOutHist(UInt_t evt)
{
  // This function extracts a random event from the tree after 
  // the tree addresses have been set and adds to the output histogram.
  
  Double_t sort_value,weight_value;
  Int_t pos,col,csi,group;
  
  btree->GetEntry(evt);
  /* printf("Got event: %d\n",evt); */
  
  for(int j=0; j<sortLeaf->GetLen(); j++)
    {
      sort_value = sortLeaf->GetValue(j); // in keV
      weight_value = weightLeaf->GetValue(j); // weight
      pos = posLeaf->GetValue(j);
      col = colLeaf->GetValue(j);
      csi = csiLeaf->GetValue(0); // recoil in csi only once per event
      group = group_map[pos][col][csi];
      
      /* printf("pos %2d col %1d E %.3f w %.3f csi %2d\n",pos,col,sort_value,weight_value,csi); */
      /* getc(stdin); */
      
      // drop bad hpge crystals and csi
      /**************************************************************
         for 84Kr: 10.2 (38), 11.0 (40), 11.2 (42), 12.2 (46), no csi
         for 94Sr: 12.2 (46), drop corner CsI (11,15,19,23)
      **************************************************************/      
      int hpge=(pos-1)*4+col; //0-3 pos1, 4-7 pos2, etc.
      /* if(hpge != 38) */
      /* if(hpge != 40) */
      /* if(hpge != 42) */
      if(hpge != 46)
      if(csi != 11)
      if(csi != 15)
      if(csi != 19)
      if(csi != 23)
	{
	  if(sort_value >= 0.0)
	    {
	      if(fwhmResponse==false)
		{
		  dOutHist[group][(int)(sort_value*sort_scaling + sort_shift)]+=(float)weight_value; // no fwhm response
		}
	      else if(fwhmResponse==true)
		{
		  // fwhm response on energy BEFORE scaling!
		  double sort_value_fwhm = FWHM_response(sort_value); // in keV
		  dOutHist[group][(int)(sort_value_fwhm*sort_scaling + sort_shift)]+=(float)weight_value;
		}    
	    }
	}
      
    }
  
}

/*******************************************************************************/
void readGroupMap()
{
  FILE *inp;
  char line[132];
  int  pos,col,csi,group;
  
  if((inp=fopen(group_file,"r"))==NULL)
    {
      printf("\nI can't open file %s\n",group_file);
      exit(1);
    }
  /* printf("\nTIGRESS-CsI group map read from the file %s\n",group_file); */
  
  if(fgets(line,132,inp)!=NULL)
    {
      if(fgets(line,132,inp)!=NULL)
	while(fscanf(inp,"%d %d %d %d",&pos,&col,&csi,&group)!=EOF)
	  if(csi>=1 && csi<=24)
	    if(pos>=1 && pos<=16)
	      if(col>=0 && col<=3)
		group_map[pos][col][csi]=group;
    }  
  else
    {
      printf("Wrong structure of file %s\n",group_file);
      printf("Aborting sort\n");
      exit(1);
    }
  fclose(inp);
}

/*******************************************************************************/
double FWHM_response(double ch_in)
{
  double ch_out,fwhm,sigma,ch;
  
  if(ch_in==0.)
    return ch_in;
  
  ch=ch_in/1000.;
  fwhm=sqrt(fwhmF*fwhmF + fwhmG*fwhmG*ch + fwhmH*fwhmH*ch*ch);
  sigma=fwhm/2.35482;
  if(sigma>0)
    ch_out=randGen->Gaus(ch_in,sigma);
  else
    ch_out=ch_in;

  return ch_out;
}

/*******************************************************************************/
double lrchisq(const double *par)
{
  // chisq from likelihood ratio
  // for more information see Baker and Cousins pg. 439 and Appendix
  double yi=0.;        // model
  double ni=0.;        // experiment
  double lrchisq = 0.; // likelihood ratio chisq
  int i=0;

  for (i=startCh[spCurrent]; i<=endCh[spCurrent]; i++)
    {
      // events in ith bin
      ni = expCurrent[i]; 
      
      // calculate model in the ith bin
      yi = par[0]*simCurrent[i] + par[1] + par[2]*erfc(((double)i-sfc[spCurrent])/sfw[spCurrent]);
      
      // evaluate chisq given input parameters
      if(ni > 0.)
	lrchisq += ( yi - ni + ni*log(ni/yi) );
      else
	lrchisq += yi; // the log(0) case
    }
  lrchisq *= 2.;
  return lrchisq;
}

/*******************************************************************************/
void find_chisqMin()
{
  chisq=0.;
  for(int i=0; i<numSpectra; i++)
    {
      // for more information see minimizer class documentation
      // https://root.cern.ch/root/html/ROOT__Math__Minimizer.html
      char minName[132] = "Minuit";
      char algoName[132] = ""; // default conjugate gradient type method
      ROOT::Math::Minimizer* min = ROOT::Math::Factory::CreateMinimizer(minName, algoName);
  
      // set tolerance , etc...
      min->SetMaxFunctionCalls(1000000); // for Minuit
      min->SetMaxIterations(10000);      // for GSL 
      min->SetTolerance(0.001);
      min->SetPrintLevel(0);             // set to 1 for more info
      
      // communication to the likelihood ratio function
      // (via global parameters)
      for(int j=0; j<S32K; j++)
	{
	  expCurrent[j] = (double)expHist[spectrum[i]][j];
	  simCurrent[j] = (double)dOutHist[spectrum[i]][j];
	}
      spCurrent = i;
      
      // create funciton wrapper for minmizer
      // a IMultiGenFunction type 
      ROOT::Math::Functor lr(&lrchisq,3); 
      
      // behaves best when these are small
      // starting point    
      double variable[3] = {0.001,0.001,0.001};
      // step size
      double step[3] = {0.0001,0.0001,0.0001};
      
      min->SetFunction(lr);
      
      // Set pars for minimization
      min->SetVariable(0,"a0",variable[0],step[0]);
      min->SetVariable(1,"a1",variable[1],step[1]);
      min->SetVariable(2,"a2",variable[2],step[2]);
      
      // do the minimization
      min->Minimize(); 
      
      // grab parameters from minimum
      const double *xs = min->X();

      // print results
      /* std::cout << "Minimum: f(" << xs[0] << "," << xs[1] << "," << xs[2] << "): " */
      /* 		<< min->MinValue()  << std::endl; */

      // assuming 3 parameters
      // save pars
      for(int j=0;j<3;j++)
	aFinal[j][i] = xs[j];

      // add to total chisq
      chisq += min->MinValue();
    }
}

/*******************************************************************************/
void plotSpectra()
{
  TH1D *results[NSPECT];

  c=(TCanvas *)gROOT->FindObject("StdCanv");
  if(c!=NULL)
    c->Close();
  c=new TCanvas("StdCanv","StdCanv",1618,1000);
  c->Clear();
  
  // initialize and fill results histo
  char resultsName[132];
  for(int i=0;i<NSPECT;i++)
    {
      sprintf(resultsName,"results_%2d",i);
      results[i] = (TH1D*)gROOT->FindObject(resultsName);
      if(results[i] == NULL)
	results[i] = new TH1D(resultsName,";;",S32K,0,S32K-1);	\
      results[i]->Reset();
    }

  // be careful with indicies here
  for(int i=0; i<numSpectra; i++)
    for(int j=0; j<S32K; j++)
      results[spectrum[i]]->Fill(j,aFinal[0][i]*dOutHist[spectrum[i]][j]+aFinal[1][i]+aFinal[2][i]*erfc(((double)j-sfc[i])/sfw[i]));
  
  // display limits
  Double_t low[NSPECT],high[NSPECT];
    
  // divide canvas to accomodate all spectra
  // assumes number to plot <= 3*2=6
  c->Divide(3,2,1E-6,1E-6,0);
  
  // formatting the plot area
  // be careful with indicies
  for(int i=1; i<=6; i++)
    {   
      c->GetPad(i)->SetRightMargin(0.01);
      c->GetPad(i)->SetTopMargin(0.025);
      Int_t ind = i-1;
      low[i] = startCh[ind];
      high[i] = endCh[ind];
    }
  
  // plot
  for(int i=1; i<=6; i++)
    {
      c->cd(i);
      // data in black
      data[i]->SetLineStyle(1);
      data[i]->SetLineWidth(1);
      data[i]->SetLineColor(kBlack);
      data[i]->GetXaxis()->SetRangeUser(low[i],high[i]);
      data[i]->SetStats(0);
      data[i]->Draw();
      
      // simulation in red
      results[i]->SetLineStyle(1);
      results[i]->SetLineWidth(1);
      results[i]->SetLineColor(kRed);
      results[i]->Draw("SAME");
    }
  
  c->cd(1);
  // x1,y1,x2,y2
  TLegend *leg = new TLegend(0.70,0.85,0.90,0.95);
  leg->AddEntry(data[1],"Experiment","l");
  leg->AddEntry(results[1],"Simulation","l");
  leg->SetFillColor(0);
  leg->SetFillStyle(0);
  leg->SetBorderSize(0);
  leg->SetTextSize(0.035);
  leg->Draw();
  
  c->SetBorderMode(0);
  c->Update();
  
  /* theApp->Run(kTRUE); */
}

/*******************************************************************************/
int readMCA(FILE* inp,char* filename,float inpHist[NSPECT][S32K])
{
  int i=0;
  int j=0;
  
  int mcaHist[NSPECT][S32K];
  for (i=0; i<=endSpectrum; i++)
    if(fread(mcaHist[i],S32K*sizeof(int),1,inp)!=1)
      {
	printf("ERROR: Error reading file %s!\n",filename);
	exit(-1);
      }
 
  for (i=0; i<NSPECT; i++)
    for (j=0; j<S32K; j++)
      	inpHist[i][j]=(float)mcaHist[i][j];
     
  for(i=1; i<NSPECT; i++)
    for(j=0; j<S32K; j++)
      	data[i]->Fill(j,inpHist[i][j]);
       
  return 1;
}

/*******************************************************************************/
int readFMCA(FILE* inp,char* filename,float inpHist[NSPECT][S32K])
{
  int i=0;
  int j=0;

  for (i=0; i<=endSpectrum; i++)
    if(fread(inpHist[i],S32K*sizeof(float),1,inp)!=1)
      {
	printf("ERROR: Error reading file %s!\n",filename);
	exit(-1);
      }

  for(i=0; i<NSPECT; i++)
    for(j=0; j<S32K; j++)
      data[i]->Fill(j,inpHist[i][j]);

  return 1;
}

