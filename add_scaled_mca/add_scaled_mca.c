#include "add_scaled_mca.h"

float inpHist1[NSPECT][S32K],inpHist2[NSPECT][S32K];
  
int main(int argc, char *argv[])
{

  FILE *input1,*input2;
  FILE *output=NULL;

  if((argc!=6)&&(argc!=3))
    {
      printf("\nadd_mca input1 scale_factor_1 input_2 scale_factor_2 output\n");
      printf("------------------------------\nAdds the two input .mca files together spectrum by spectrum and outputs a combined .fmca file given by output = scale_factor_1*input_1 + scale_factor_2*input_2.\n");
      printf("\nadd_mca input_list output\n");
      printf("-------------------------\nAdds the all of the .mca files apecified in the list file together spectrum by spectrum with the corresponding scale factors and outputs a combined .mca file.\n");
      printf("The list file should contain the relative file paths to the individual .mca files seperated by line.\n\n");
      exit(-1);
    }

  
  //initialize the output histogram
  for (int i=0;i<NSPECT;i++)
    for (int j=0;j<S32K;j++)
      outHist[i][j]=0.;

  //read in the .mca files
  if(argc==6) //two spectrum mode
    {
      float scale1 = atof(argv[2]);
      float scale2 = atof(argv[4]);
      if((input1=fopen(argv[1],"r"))==NULL)
        {
          printf("ERROR: Cannot open the input file %s!\n",argv[1]);
	  perror("Error: ");
          exit(-1);
        }
      if((input2=fopen(argv[3],"r"))==NULL)
        {
          printf("ERROR: Cannot open the input file %s!\n",argv[3]);
	  perror("Error: ");
          exit(-1);
        }

      // read in input1
      const char *dot1 = strrchr(argv[1], '.'); // get the file extension
      if(strcmp(dot1 + 1,"mca")==0)
	readMCA(input1,argv[1],inpHist1);
      else if(strcmp(dot1 + 1,"fmca")==0)
	readFMCA(input1,argv[1],inpHist1);
      else
	{
	  printf("ERROR: Improper type of input file: %s\n",argv[1]);
	  printf("Integer array (.mca) and float array (.fmca) files are supported.\n");
	  exit(-1);
	}  
      fclose(input1);
      
      //append values to the output histogram
      for (int i=0;i<NSPECT;i++)
        for (int j=0;j<S32K;j++)
          outHist[i][j]+=scale1*inpHist1[i][j];
      
      // read in input2
      const char *dot2 = strrchr(argv[3], '.'); // get the file extension
      if(strcmp(dot2 + 1,"mca")==0)
	readMCA(input2,argv[3],inpHist2);
      else if(strcmp(dot2 + 1,"fmca")==0)
	readFMCA(input2,argv[3],inpHist2);
      else
	{
	  printf("ERROR: Improper type of input file: %s\n",argv[2]);
	  printf("Integer array (.mca) and float array (.fmca) files are supported.\n");
	  exit(-1);
	}  
      fclose(input2);

      //append values to the output histogram
      for (int i=0;i<NSPECT;i++)
        for (int j=0;j<S32K;j++)
          outHist[i][j]+=scale2*inpHist2[i][j];
      
      //open the output file   
      if((output=fopen(argv[5],"w"))==NULL)
        {
          printf("ERROR: Cannot open the output .fmca file!\n");
          exit(-1);
        }
    }

  if(argc==3) //spectrum list mode
    {
      char str[256];
      float scale=0.;
      //open the list file
      if((input1=fopen(argv[1],"r"))==NULL)
        {
          /* printf("ERROR: Cannot open the input list file %s!\n",argv[1]); */
          exit(-1);
        }
        
      //scan the list file for .mca files and add them to the output histogram
      while(fscanf(input1,"%s %f",str,&scale)!=EOF)
        {
          //open and read in the .mca file
          if((input2=fopen(str,"r"))==NULL)
            {
              /* printf("ERROR: Cannot open the input file %s!\n",str); */
              exit(-1);
            }
          
          //read in spectra
         // read in input2
	  const char *dot2 = strrchr(str, '.'); // get the file extension
	  if(strcmp(dot2 + 1,"mca")==0)
	    readMCA(input2,str,inpHist2);
	  else if(strcmp(dot2 + 1,"fmca")==0)
	    readFMCA(input2,str,inpHist2);
	  else
	    {
	      printf("ERROR: Improper type of input file: %s\n",argv[2]);
	      printf("Integer array (.mca) and float array (.fmca) files are supported.\n");
	      exit(-1);
	    }  
	  fclose(input2);

          //add the .mca file values to the output histogram    
          for (int i=0;i<NSPECT;i++)
            for (int j=0;j<S32K;j++)
              outHist[i][j]+=scale*inpHist2[i][j];
        }
      fclose(input1);
        
      //open the output file
      if((output=fopen(argv[2],"w"))==NULL)
        {
          printf("ERROR: Cannot open the output .mca file!\n");
          exit(-1);
        }
    }

  // replace bins with less than zero counts
  // e.g. small negative data from bg subtraction
  /* for (int i=0;i<NSPECT;i++) */
  /*   for (int j=0;j<S32K;j++) */
  /*     if(outHist[i][j] < 0.) */
  /*     outHist[i][j]=0.; */
    
  //write the output histogram to disk
  for (int i=0;i<NSPECT;i++)
    fwrite(outHist[i],S32K*sizeof(float),1,output);
  fclose(output);
  if(argc==3)
    printf("Wrote %i spectra to file %s\n",NSPECT,argv[2]);
  if(argc==4)
    printf("Wrote %i spectra to file %s\n",NSPECT,argv[3]);
    
  return 0; //great success
}

/*******************************************************************************/
int readMCA(FILE* inp,char* filename,float inpHist[NSPECT][S32K])
{
  int mcaHist[NSPECT][S32K];
  for (int i=0; i<NSPECT; i++)
    if(fread(mcaHist[i],S32K*sizeof(int),1,inp)!=1)
      {
	break; // dont read in more spectra than there are in the input file
	/* printf("ERROR: Error reading file %s!\n",filename); */
	/* printf("For i=%d fread(mcaHist[i],S32K*sizeof(int),1,inp)=%lu\n",i,fread(mcaHist[i],S32K*sizeof(int),1,inp)); */
	/* exit(-1); */
      }
 
  for (int i=0; i<NSPECT; i++)
    for (int j=0; j<S32K; j++)
      	inpHist[i][j]=(float)mcaHist[i][j];

  return 1;
}

/*******************************************************************************/
int readFMCA(FILE* inp,char* filename,float inpHist[NSPECT][S32K])
{
  for (int i=0; i<NSPECT; i++)
    if(fread(inpHist[i],S32K*sizeof(float),1,inp)!=1)
      {
	break; // dont read in more spectra than there are in the input file
	/* printf("ERROR: Error reading file %s!\n",filename); */
	/* printf("For i=%d fread(inpHist[i],S32K*sizeof(float),1,inp)=%lu\n",i,fread(inpHist[i],S32K*sizeof(float),1,inp)); */
	/* exit(-1); */
      }

  return 1;
}
