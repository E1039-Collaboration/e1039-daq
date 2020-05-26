using namespace std;
// Written by Kaz - 3/6/2012

#include "E906ScalerDAQ.h"

#include <TString.h>

namespace
{
  const TString c_topDir = "/home/e1039daq/data/scaler_fft";
}


int PrepareDir( const TString& dir )
{
  int madedir = 0;
  if( 0 != system( Form( "test -d %s", dir.Data() ) ) )
  {
    cout << "INFO - PrepareDir - directory " << dir << " does not exist.  create it..." << endl;
    madedir = system( Form( "mkdir -m 755 -p %s", dir.Data() ) );
    if( 0 != madedir )
    {
      cout << "ERROR - PrepareDir - could not make top level directory: " << dir << endl;
    }
  }
  return madedir;
}




E906ScalerDAQ::E906ScalerDAQ()
{//constructor

  ifstream in;
  //Char_t dummy[20];
  in.open("scalerDAQ_mapping.dat");
  for(Int_t i=0;i<3;i++)
  {
    for(Int_t j=0;j<32;j++)
    {
      in >> scalernames[i][j] >> variablename[i][j]>> scalerstatus[i][j];
    }
  }
  in.close();

  samecounter = 0;
  debugout.open("scalerdaqout.out");
  TVirtualFFT::SetTransform(0);
  rootspills = 0;
  numskipped = 0;
  slowcontrol_counter = 0;
  for(Int_t i=0;i<100;i++)
    Last_PhysicsEvent[i] = 0;

  RFintegral = 0;

  timer = new TStopwatch();

  killscript = 0;
  eventcounter = 0;
  eventsThisRun = 0;
  targbit = 0;
  targetstring[0] = "PROBLEM";
  targetstring[1] = "LH2";
  targetstring[2] = "Empty Flask";
  targetstring[3] = "LD2";
  targetstring[4] = "No Target";
  targetstring[5] = "Iron";
  targetstring[6] = "Carbon";
  targetstring[7] = "Tungsten";

  //initialize histograms, set frequencies, binning, plot limits

  Float_t tmp;
  tmp = pulse_frequency/60;

  num60hz_frames = 5;
  histo60hz_counter = 0;


  bin60hz = Int_t(tmp)+1;//pulse_frequency*(1/60)
  cout << "bin60hz = " << bin60hz <<  " pulse_frequency =  "<< pulse_frequency << endl;
  histo_raw_spectrum = new TH1F("E906 Spill Counter (0.133ms bins)","E906 Spill Counter (0.137ms bins)",Int_t(pulse_frequency*(6.0-0))+1,0,6000);//range in millisec
  histo_for_fft = new TH1F("forFFTHisto","forFFTHisto",Int_t(pulse_frequency*(5.4-1.5))+1,1500,5400);
  histozoom_lvl1 = new TH1F("E906 Spill 1sec Zoom","E906 Spill 1sec Zoom",Int_t(pulse_frequency)+1,2000,3000);
  histozoom_lvl2 = new TH1F("Six 60Hz cycles (100ms Zoom)","E906 Spill 200ms Zoom",Int_t(pulse_frequency*(0.1))+1,3000,3100);

  //create 10 of these, since that is how big the hsito60hz array is
  for(Int_t i=0;i<10;i++)
  {
    sprintf(text,"E906 Spill Folded onto one (phase-locked) 60Hz Line Cycle (frame %d)",i+1);
    histo60hz[i] = new TH1F(text,text,bin60hz,0,16.7);
  }
  fft = new TH1F("forFFTHisto1","forFFTHisto1",Int_t(pulse_frequency*(5.4-1.5)),0,pulse_frequency);
  fft_log = new TH1F("forFFTHisto2","forFFTHisto2",Int_t(pulse_frequency*(5.4-1.5)),0,pulse_frequency);

  //tcanvas initialization
  c1 = new TCanvas("c1","Scaler DAQ",10,300,800,600);
  gpad = new TPad("Graphs","Graphs",0.01,0.01,0.99,0.99);
  gStyle->SetOptStat(0000);
  gStyle->SetTitleSize(0.12,"t");
  gStyle->SetTitleX(0.2);
  gpad->Draw();
  gpad->Divide(2,3);
  pavement = new TPaveText(0.1,0.1,0.98,0.98,"brNDC");
  //gpad->cd(1) is reserved for legends
  pavement60hz = new TPaveText(0.5,0.75,0.9,0.84,"brNDC");
  pavement60hz->AddText("Colors = ~1.3 second frames of spill");

  gpad->cd(2);//60hz

  histo60hz[0]->SetTitle("E906 Spill Folded onto one (phase-locked) 60Hz Line Cycle");
  histo60hz[0]->GetXaxis()->SetTitle("Time (ms)");
  histo60hz[0]->GetYaxis()->SetTitle("Counts            ");
  histo60hz[0]->GetYaxis()->SetTitleSize(0.1);
  histo60hz[0]->SetLineColor(histo60hz_counter+1);
  histo60hz[0]->Draw();


  gpad->cd(6);
  histo_raw_spectrum->SetTitle("E906 Spill Counter (0.133ms bins)");
  histo_raw_spectrum->GetXaxis()->SetTitle("Time (ms)");
  histo_raw_spectrum->GetXaxis()->SetRangeUser(1000,5500);
  histo_raw_spectrum->GetYaxis()->SetTitle("Counts                ");
  histo_raw_spectrum->GetYaxis()->SetTitleSize(0.1);
  histo_raw_spectrum->Draw();

  gpad->cd(4);
  fft->SetAxisRange(1,400,"X");
  fft->SetTitle("FFT");
  fft->GetXaxis()->SetTitle("Frequency (Hz)");
  fft->GetYaxis()->SetTitle("Arbirtary");
  fft->GetYaxis()->SetTitleSize(0.1);
  fft->Draw();

  gpad->cd(5);
  histozoom_lvl1->SetTitle("E906 Spill 1sec Zoom");
  histozoom_lvl1->GetXaxis()->SetTitle("Time (ms)");
  histozoom_lvl1->GetYaxis()->SetTitle("Counts                ");
  histozoom_lvl1->GetYaxis()->SetTitleSize(0.1);
  histozoom_lvl1->Draw();

  gpad->cd(3);
  histozoom_lvl2->SetTitle("Six 60Hz cycles (100ms Zoom)");
  histozoom_lvl2->GetXaxis()->SetTitle("Time (ms)");
  histozoom_lvl2->GetYaxis()->SetTitle("Counts                ");
  histozoom_lvl2->GetYaxis()->SetTitleSize(0.1);
  histozoom_lvl2->Draw();
  //
  gpad->Update();

  //60hz array -- sum array indices <bin60hz to get 60hz normalization
  for(Int_t i=0;i<200;i++){
    for(Int_t j=0;j<10;j++){
      histo60hz_array[j][i] = 0;
    }
  }

  //running sums 
  sumhits=0;sumhits_squared=0;sumclock=0;//reset running sums for duty factor calculation
  normalization = 0;sum_over_60hz = 0; //reset running sums for 60hz normalization
  
  updateflag = true;
  eosflag = false;
  eos_slowcontrol_count = 0;
  runnumber_prev = 0;
  runnumber_int = 0;

}

E906ScalerDAQ::~E906ScalerDAQ()
{
  debugout.close();
}

void E906ScalerDAQ::E906DAQ_output_EPICS()
{
  cout << "Start EPICS output" << endl;
  int counter = 0;
  float R8L=0., R5L=0., R85L=0.;
  //output scaler channels to epics
  for(Int_t i=0;i<110;++i)
  {
    if(PhysicsEvent[i]==0x08020000||PhysicsEvent[i]==0x08030000||PhysicsEvent[i]==0x08040000)
    { //that's 0xE906F004, 5, or 6
      ++counter;

      for(Int_t j=0;j<32;++j)
      { //j is scaler channel
        sprintf(text,"caput -t SC%d_CH%d %lu",counter,j+1,PhysicsEvent[i+1+j]);//construct string for EPICS-write in terse mode (-t...keeps system and EPICS I/O to a minimum).
        sc_ch[counter-1][j] = PhysicsEvent[i+1+j];
        system(text);

        if(counter==1&&j==12)
          R8L = (float)PhysicsEvent[i+1+j];
        else if(counter==2&&j==10)
          R5L = (float)PhysicsEvent[i+1+j];
        else if(counter==2&&j==12)
          R85L = (float)PhysicsEvent[i+1+j];

      }
    }
  }

  Y4DF = 100*(R8L*R5L)/(R85L*207e6);
  cout << "Y4DF = " << Y4DF << "%" << endl;
  //output duty factor to epics
  sprintf(text,"caput -t DUTYFACTOR_SC %.4f",100*dutyfactor_noMI);
  system(text);
  sprintf(text,"caput -t DUTYFACTORMI_SC %.4f",100*dutyfactor_includeMI);
  system(text);

  cout << "Finished writing to EPICS" << endl;
}

void E906ScalerDAQ::E906DAQ_getrunnumber()
{

  //get current event number
  fp = popen("/usr/bin/rsh -n -l e1039daq e1039daq1 'plask -rt Spin -spRunNumber'","r");
  runnumber_flag = true;
  if(fp==NULL)
  {
    cout << "Failed to grab runnumber" << endl;
    killscript = 1;
    runnumber_flag = false;
  }

  while(fgets(runnumber,sizeof(runnumber)-1,fp)!=NULL)
  { //get runnumber into char
    cout << "RUNNUMBER!!!!" << runnumber << endl;
  }

  if(runnumber_flag)
  {
    if(strcmp(runnumber,"0")==0)
    {
      ; //nothing to be done?
    }
    else
    {
      runnumber_int = atoi(runnumber); //convert char to int
    }
  }
  else
  {
    ; //keep old runnumber if plask didn't work
  }
  pclose(fp);//close system command

  sleep(2);
  ifstream inspillcount;
  inspillcount.open("/data2/e1039_data/slowcontrol_data/spillcounter/local_spillcount.dat");
  inspillcount >> spillcounter;
  inspillcount.close();

  ofstream outspillcount;
  outspillcount.open("/data2/e1039_data/slowcontrol_data/spillcounter/scalerDAQ_lastspill.dat");
  outspillcount << spillcounter << endl;
  outspillcount.close();


}
void E906ScalerDAQ::E906DAQ_reset_at_newrun()
{
  //This function is not needed if we the scaler is getting the EOS (in the correct channel).

}

void E906ScalerDAQ::E906DAQ_accumulate_runningsum()
{
  if(eventcounter>1.5*pulse_frequency&&eventcounter<5.4*pulse_frequency)
  {//event counter is updated during histogram fill
    sumhits += (Float_t)PhysicsEvent[18];
    sumhits_squared +=(Float_t)pow(PhysicsEvent[18],2);
    sumclock +=1;
  }
  //end running averages

  //clock
  if(PhysicsEvent[14]==1)
  {
    //   cout << "Found 60Hz flag" << endl;
    start_60hz = 1;//only start counting the 60hz when we see the 60hz flag...discard all events coming before, as we don't know their phase apriori
    counter_60hz = 1;//when we see the 60hz flag, reset 60hz bin counter to 1
  }

  //Fill 60hz and update 60hz counter as needed.  
  if(start_60hz==1&&counter_60hz<=bin60hz)
  {//60hz counter is updated during histogram fill
    sum_over_60hz += PhysicsEvent[18];
    histo60hz_array[histo60hz_counter][counter_60hz-1] += PhysicsEvent[18];
  }
}

void E906ScalerDAQ::E906DAQ_fillhistograms()
{
  const Float_t eventTime = Float_t(1000*eventcounter)/pulse_frequency;
  const Float_t timeOfEOS = 5400;
  const Float_t eosTolerance = 50;
  const UInt_t tooManyCounts = 250;

  if( PhysicsEvent[18] > tooManyCounts )
    cout << "Too many counts for PhysicsEvent[18] = " << PhysicsEvent[18] << " with eventcounter = " << eventcounter << " and bin = " << eventTime << endl;

  //this if check avoids new extra EOS events
  if( ! ( fabs(eventTime-timeOfEOS) < eosTolerance && PhysicsEvent[18] > tooManyCounts ) )
  {
    histo_raw_spectrum->Fill(eventTime,PhysicsEvent[18]);//PhysicsEvent[24] would be another open spigot
    histozoom_lvl1->Fill(eventTime,PhysicsEvent[18]);
    histozoom_lvl2->Fill(eventTime,PhysicsEvent[18]);
    histo_for_fft->Fill(eventTime,PhysicsEvent[18]);
  }

  if(start_60hz==1&&counter_60hz<=bin60hz){
    histo60hz[histo60hz_counter]->SetBinContent(counter_60hz,histo60hz_array[histo60hz_counter][counter_60hz-1]);//array filled under accumulate_runningsum()
    ++counter_60hz;
  }

  if(eventcounter%10000==0&&eventcounter!=0)
  {
    ++histo60hz_counter;
    cout << Form("E906ScalerDAQ::E906DAQ_fillhistograms - eventcounter=%d, histo60hz_counter incremented to %d.", eventcounter, histo60hz_counter ) << endl;
  }
}

void E906ScalerDAQ::E906DAQ_update_counters()
{
  ++eventcounter;
  ++eventsThisRun;
}

void E906ScalerDAQ::E906DAQ_retrieve_coda_sums()
{
  for(Int_t i=0;i<150;++i)
  {
    if(PhysicsEvent[i]==0xe906f007)
    {//EOS coda sums
      coda_Isum = PhysicsEvent[i+1];
      coda_Isquared_sum = PhysicsEvent[i+2];
      coda_Nevts = PhysicsEvent[i+3];
    }
  }

  //eliminate division by zero
  if(coda_Isquared_sum>0&&coda_Nevts>0)
    coda_dutyfactor = pow(coda_Isum/coda_Nevts,2)/(coda_Isquared_sum/coda_Nevts);
  else
    coda_dutyfactor = -9999;//bad spill
}

Bool_t E906ScalerDAQ::E906DAQ_slowcontrol_check()
{
  //0x8c10cc
  if(PhysicsEvent[1]==0x8c10cc&&!eosflag)
  {//if slowcontrol event, but eos has not been seen, execute eos routines
    cout << "EOS missed.  Executing FFT at slowcontrol" << endl;
    ++eos_slowcontrol_count; //keep a count of missed EOS bits
    //   eosflag = true;
    return true;
  }

  return false;//this is good...didn't miss EOS at EOS flag
}

void E906ScalerDAQ::E906DAQ_EOS_report()
{
  if((eosflag==false&&PhysicsEvent[reference_index+17]>0&&PhysicsEvent[reference_index+17]<10))
  {
    cout << "--------------" << endl;
    cout << "Found EOS event: " <<  eventcounter << "  " << PhysicsEvent[reference_index+17] << endl;
    cout << "EOS found: eventsThisRun = "<< eventsThisRun <<  "  eventcounter =  " << eventcounter << "  PhysicsEvent[16]="<< PhysicsEvent[reference_index+17] << hex <<  "   PhysicsEvent[27]=" << PhysicsEvent[27] << dec<< endl;
    cout << "--------------" << endl;
  }
}

Bool_t E906ScalerDAQ::E906DAQ_endofspill_check()
{
  //check any event for target bits.  
  Int_t clockevents;
  clockevents = (PhysicsEvent[10])>>16;

  if(PhysicsEvent[reference_index+6]>0&&PhysicsEvent[reference_index+6]<10)
  {
    cout << "BOS found " << eventcounter << "  PhysicsEvent[reference_index+6]="<< PhysicsEvent[reference_index+6] <<   endl;
    updateflag = true;
    eosflag = false;

    //if bos, look for target bit
    cout << PhysicsEvent[21] << " " << PhysicsEvent[23] << " " << PhysicsEvent[24] << endl;// These 3 bits are used to identify the 7 target positions

    if(PhysicsEvent[reference_index+10]==0&&PhysicsEvent[reference_index+11]==0&&PhysicsEvent[reference_index+6]>0)
      targbit = 1; //LH2
    else if(PhysicsEvent[reference_index+10]==0&&PhysicsEvent[reference_index+11]>0&&PhysicsEvent[reference_index+6]>0)
      targbit = 2; //Empty
    else if(PhysicsEvent[reference_index+10]>0&&PhysicsEvent[reference_index+11]>0&&PhysicsEvent[reference_index+6]>0)
      targbit = 3; //LD2
    else if(PhysicsEvent[reference_index+10]>0&&PhysicsEvent[reference_index+11]>0&&PhysicsEvent[reference_index+6]==0)
      targbit = 4; //Solid1
    else if(PhysicsEvent[reference_index+10]>0&&PhysicsEvent[reference_index+11]==0&&PhysicsEvent[reference_index+6]==0)
      targbit = 5; //Solid2
    else if(PhysicsEvent[reference_index+10]>0&&PhysicsEvent[reference_index+11]==0&&PhysicsEvent[reference_index+6]>0)
      targbit = 6; //Solid3
    else if(PhysicsEvent[reference_index+10]==0&&PhysicsEvent[reference_index+11]>0&&PhysicsEvent[reference_index+6]==0)
      targbit = 7; //Solid4
    else
      targbit = 0; //bad

    cout << "Target bit is " << targbit << endl;
  }

  if((clockevents==6||PhysicsEvent[reference_index+17]>0)&&eosflag==false)
  {
    cout << "EOS found " << eventcounter << "  PhysicsEvent[reference_index+17]="<< PhysicsEvent[reference_index+17] <<   endl;
    eosflag = true;
    return true;
  }
  else
  {  
    return false;
  }
}

void E906ScalerDAQ::E906DAQ_skip_check()
{
  //For a 7.3kHz trigger rate, this means we skipped an EOS
  if(eventcounter>50000)
    numskipped++;

  cout << "NUMSKIPPED SPILLS = " << numskipped << endl;  
  cout << "Slowcontrol EOS check = " << eos_slowcontrol_count << endl;  
}

Int_t E906ScalerDAQ::E906DAQ_killscript()
{
  return killscript;
}

void E906ScalerDAQ::E906DAQ_fft()
{
  cout << "Do FFT...";
  timer->Start();
  max60hz = 0;
  for(Int_t j=0;j<num60hz_frames;++j)
  {
    for(Int_t i=0;i<histo60hz[j]->GetNbinsX();++i)
    {
      if(max60hz<histo60hz[j]->GetBinContent(i)){
        max60hz = histo60hz[j]->GetBinContent(i);
      }
    }
  }

  //Start FFT
  gpad->cd(4);
  TH1 *FFTHisto(0);
  FFTHisto = histo_for_fft->FFT(FFTHisto,"MAG");
  const Int_t nEntries = FFTHisto->GetEntries();
  for(Int_t k=1;k<nEntries;++k)
  {
    fft->SetBinContent(k,FFTHisto->GetBinContent(k));
    fft_log->SetBinContent(k,FFTHisto->GetBinContent(k));
  }
  fft->SetAxisRange(1,400,"X");
  fft->SetTitle("FFT");
  fft->GetXaxis()->SetTitle("Frequency (Hz)");
  fft->GetYaxis()->SetTitle("Arbirtary");
  fft->GetYaxis()->SetTitleSize(0.1);
  fft->Draw();
  gpad->Update();

  delete FFTHisto; //must be deleted, or memory leak will appear during the next iteration of FFT
  FFTHisto = 0;

}

void E906ScalerDAQ::E906DAQ_Get_ACNET()
{
  char tmpchar[30];

  fp2 = popen("acnet -t G:TURN13","r");
  while(fgets(tmpchar,sizeof(tmpchar)-1,fp2)!=NULL){
    turn13 = atof(tmpchar);
  }
  pclose(fp2);

  fp2 = popen("acnet -t G:BNCH13","r");
  while(fgets(tmpchar,sizeof(tmpchar)-1,fp2)!=NULL){
    bunch13 = atof(tmpchar);
  }
  pclose(fp2);

  fp2 = popen("acnet -t G:NBSYD","r");
  while(fgets(tmpchar,sizeof(tmpchar)-1,fp2)!=NULL){
    nbsyd = atof(tmpchar);
  }
  pclose(fp2);

  fp2 = popen("acnet -t F:NM3SEM","r");
  while(fgets(tmpchar,sizeof(tmpchar)-1,fp2)!=NULL){
    nm3sem = atof(tmpchar);
  }
  pclose(fp2);

  cout << "turn13 = " << turn13 << "  bunch13 = " << bunch13 << " nbsyd = " << nbsyd << endl;
}

void E906ScalerDAQ::E906DAQ_dutyfactor()
{
  cout << "Calculate Dutyfactor...";

  //Eliminate division by zero for empty spills 
  if(sumclock==0)
    sumclock = 1;

  if(sumhits_squared==0)
    sumhits_squared=1;

  cout << "sumhits = " << sumhits << "  squared = " << sumhits_squared << "  sumclock = " << sumclock << endl;
  dutyfactor_includeMI = (nbsyd*bunch13)*pow(sumhits,2)/(588*sumclock*sumhits_squared);
  dutyfactor_noMI = pow(sumhits,2)/(sumclock*sumhits_squared);
  cout << "DF_withMI = " << dutyfactor_includeMI*100 << "% " << endl;
  cout << "DF_noMI = " << dutyfactor_noMI*100 << "%" <<  endl;
}

void E906ScalerDAQ::E906DAQ_updateplots(Bool_t EOS)
{
  if(EOS&&updateflag)
  {//at EOS, update ONCE
    updateflag = false;
    cout << "Updating Plots at EOS...";
    for(Int_t i=1;i<3;++i)
    {
      gpad->cd(i);
      if(i==1)
      {
        pavement->Draw();
        cout << "Finished drawing Pavement " << endl;
      }
      if(i==2)
      {
        histo60hz[0]->GetYaxis()->SetRangeUser(0,1.5*max60hz);
        histo60hz[0]->SetLineColor(histo60hz_counter+1);
        histo60hz[0]->Draw("same");
        pavement60hz->Draw();
      }
      //what to do for i=3-7?
    }
    gpad->Update();
  }
  else
  {//if not EOS, update once every 2000 evts
    if(eventcounter%2000==0)
    {

      gpad->cd(2);//60hz
      histo60hz[histo60hz_counter]->SetTitle("E906 Spill Folded onto one (phase-locked) 60Hz Line Cycle");
      histo60hz[histo60hz_counter]->GetXaxis()->SetTitle("Time (ms)");
      histo60hz[histo60hz_counter]->GetYaxis()->SetTitle("Counts            ");
      histo60hz[histo60hz_counter]->GetYaxis()->SetTitleSize(0.1);
      histo60hz[histo60hz_counter]->SetLineColor(histo60hz_counter+1);
      if(histo60hz_counter>0)
        histo60hz[0]->GetYaxis()->SetRangeUser(0,histo60hz[1]->GetBinContent(20)*3);

      if(histo60hz_counter==0)
        histo60hz[histo60hz_counter]->Draw();
      else
      {
        histo60hz[0]->Draw("same");
        histo60hz[histo60hz_counter]->Draw("same");
      }

      gpad->cd(6);
      histo_raw_spectrum->SetTitle("E906 Spill Counter (0.133ms bins)");
      histo_raw_spectrum->GetXaxis()->SetTitle("Time (ms)");
      histo_raw_spectrum->GetXaxis()->SetRangeUser(1000,5500);
      histo_raw_spectrum->GetYaxis()->SetTitle("Counts                ");
      histo_raw_spectrum->GetYaxis()->SetTitleSize(0.1);
      histo_raw_spectrum->Draw();

      gpad->cd(4);
      fft->SetAxisRange(1,600,"X");
      fft->SetTitle("FFT");
      fft->GetXaxis()->SetTitle("Frequency (Hz)");
      fft->GetYaxis()->SetTitle("Arbirtary");
      fft->GetYaxis()->SetTitleSize(0.1);
      fft->Draw();

      gpad->cd(5);
      histozoom_lvl1->SetTitle("E906 Spill 1sec Zoom");
      histozoom_lvl1->GetXaxis()->SetTitle("Time (ms)");
      histozoom_lvl1->GetYaxis()->SetTitle("Counts                ");
      histozoom_lvl1->GetYaxis()->SetTitleSize(0.1);
      histozoom_lvl1->Draw();

      gpad->cd(3);
      histozoom_lvl2->SetTitle("Six 60Hz cycles (100ms Zoom)");
      histozoom_lvl2->GetXaxis()->SetTitle("Time (ms)");
      histozoom_lvl2->GetYaxis()->SetTitle("Counts                ");
      histozoom_lvl2->GetYaxis()->SetTitleSize(0.1);
      histozoom_lvl2->Draw();
      gpad->Modified();
      gpad->Update();
    }
  } 
}

void E906ScalerDAQ::E906DAQ_reset_at_newspill()
{
  //Reset all relevant counters, running sums, and histograms AFTER plots from previous run have been saved
  cout << "----------------------------------------------" << endl;
  cout << "End of spill -- Reset all variables/histograms" << endl;
  eventcounter = 0;
  counter_60hz = 1;
  start_60hz = 0;
  histo60hz_counter = 0;

  cout << "Clear arrays" << endl;
  for(Int_t i=0;i<200;++i)
  {
    for(Int_t j=0;j<10;++j)
    {
      histo60hz_array[j][i] = 0;
    }
  }

  for(Int_t i=0;i<100;++i)
    Last_PhysicsEvent[i] = 0;

  //Clear graphpad.  If this isn't done, saving the canvas to file (png) will take longer wth each iteration (I'm guessing due to the gpad buffer not getting flushed).  
  gpad->Clear();
  gpad->Draw();
  gpad->Divide(2,3);

  sumhits=0;sumhits_squared=0;sumclock=0;//reset running sums for duty factor calculation
  normalization = 0;sum_over_60hz = 0; //reset running sums for 60hz normalization
  PhysicsEvent = 0;

  cout << "Reset Histograms " << endl;
  if(histo_raw_spectrum->GetEntries()>0)
  {
    histo_raw_spectrum->Clear();
    histo_raw_spectrum->Reset();
  }
  if(histo_for_fft->GetEntries()>0)
  {
    histo_for_fft->Clear();
    histo_for_fft->Reset();
  }
  if(histozoom_lvl1->GetEntries()>0)
  {
    histozoom_lvl1->Clear();
    histozoom_lvl1->Reset();
  }
  if(histozoom_lvl2->GetEntries()>0)
  {
    histozoom_lvl2->Clear();
    histozoom_lvl2->Reset();
  }
  for(Int_t i=0;i<num60hz_frames;++i)
  {
    if(histo60hz[i]->GetEntries()>0)
    {
      histo60hz[i]->Clear();
      histo60hz[i]->Reset();
    }
  }
  if(fft->GetEntries()>0)
  {
    fft->Clear();
    fft->Reset();
  }  
  if(fft_log->GetEntries()>0)
  {
    fft_log->Clear();
    fft_log->Reset();
  }  

  cout << "Reset Complete" << endl;
  cout << "----------------------------------------------" << endl;
  cout << "RF integral = " << RFintegral << endl;
  RFintegral = 0;
  cout << "---------Data Process Time--------------------" << endl;
  timer->Stop();
  timer->Print();
  cout << "----------------------------------------------" << endl;

  cout << "Waiting for next spill..." << endl;
  cout << endl;
}

void E906ScalerDAQ::E906DAQ_make_legend()
{
  cout << "Making legends...";
  for(Int_t i=0;i<40;++i)
  {
    timestamp[i] = NULL;
    datestamp[i] = NULL;
  }

  fp2 = popen("date \'+%Y%m%d%H%M%S\'","r");//get timestamp
  while(fgets(timestamp_int,sizeof(timestamp_int)-1,fp2)!=NULL){
  }
  timestamp_int[14] = '\0';
  cout << timestamp_int << endl;
  pclose(fp2);

  ofstream out_timestamp;
  out_timestamp.open("/data2/data/scalerDAQ/timestamp/timestamp.dat");
  out_timestamp << timestamp_int<< endl;
  out_timestamp.close();

  fp2 = popen("date \'+%Y-%m-%d-%a-h%H-m%M-s%S\'","r");//get timestamp
  while(fgets(timestamp,sizeof(timestamp)-1,fp2)!=NULL){
  }
  pclose(fp2);

  //CONSTRUCT TPAVETEXT (ADDS TIMESTAMP, DUTYFACTOR, AND ACC PARAMETERS)
  sprintf(dutyfactor_char,"Duty Factor*(bnch13*nbsyd/588) = %.3f%% ",100*dutyfactor_includeMI);
  dutyfactorS = TString(dutyfactor_char);
  sprintf(dutyfactor_char2,"Duty_Factor@7.5kHz = %.3f%% ",100*dutyfactor_noMI);
  dutyfactorS2 = TString(dutyfactor_char2);

  sprintf(dutyfactor_char,"SQDF2 = %.3f%% ",Y4DF);
  dutyfactorS = TString(dutyfactor_char);
  sprintf(acc_char,"Turn13 = %.1f, Bnch13 = %.0f, NBSYD = %.0f",turn13,bunch13,nbsyd);
  accS = TString(acc_char);


  pavement->Clear();
  sprintf(text,"SeaQuest Spill Number: %d",spillcounter);
  pavement->AddText(text);
  pavement->AddText(timestamp);
  pavement->AddText(dutyfactorS2);	  
  pavement->AddText(dutyfactorS);
  pavement->AddText(accS);
  pavement->AddText("Target: " + targetstring[targbit]);
  cout << "done" << endl;
  //FINISH TPAVETEXT
}

void E906ScalerDAQ::E906DAQ_save()
{
  TString imgTopDir = c_topDir + "/scaler_data";

  fp2 = popen("date \'+%Y-%m-%d-%a\'","r");//get timestamp
  while(fgets(datestamp,sizeof(datestamp)-1,fp2)!=NULL){
  }
  pclose(fp2);
  //note: timestamp built elsewhere

  for(Int_t i=14;i<40;i++)
    datestamp[i] = NULL;

  for(Int_t i=26;i<40;i++)
    timestamp[i] = NULL;

  TString timestampS(timestamp);
  TString datestampS(datestamp);

  TString imgDir = imgTopDir + "/" + datestamp;
  cout << "png file directory = " << imgDir << endl;
  PrepareDir( imgDir );

  //TIMESTAMP THE PLOT AND OUTPUT TO FILE
  TString outfilename = imgDir + "/E906FFT_" + timestampS +".png";
  c1->SaveAs(outfilename);

  TString latestFile = imgTopDir + "/" + "E906FFT_last.png";
  c1->SaveAs(latestFile);

  c1->SaveAs("/data2/e906daq/scalerDAQ/E906FFT_last.png");
}

Int_t E906ScalerDAQ::E906DAQ_transient_flag()
{
  //do we have a transient
  if(PhysicsEvent[18]>100000)
    transient_flag = 1;//yes we do
  else
    transient_flag = 0;//no we don't

  return transient_flag;
}

Int_t E906ScalerDAQ::E906DAQ_goodevent_check()
{
  Int_t goodevent_flag;
  goodevent_flag = 0; //bad by default

  for(Int_t i=0;i<15;i++)
  {
    if(PhysicsEvent[i]==0xe906f002)
      reference_index = i;//+4 is bos, +5 is eos
  }

  if(PhysicsEvent[1]==0x8c10cc)
  {//slowcontrol
    ++slowcontrol_counter;
    cout << endl;
    cout << "Found Slowcontrol Event " << endl;
    goodevent_flag=0;
  }
  else if((PhysicsEvent[4]!=0&&PhysicsEvent[11]==0xe906f002&&PhysicsEvent[12]==0x08010000)||PhysicsEvent[reference_index+17]>0)
  {
    //[11],[12] are 0xE906F002 and 0xE906F003, [reference_index+17] is the EOS.  These flags mean this event is a high frequency scaler flag (i.e it's the 10kHz trigger, and is not one of the main daq scalers).  The main DAQ scaler events will be written with the flag E906F004
    goodevent_flag = 1;
  }
  else if(PhysicsEvent[reference_index+6]>0&&PhysicsEvent[reference_index+6]<10)
  {
    cout << "--------------" << endl;
    cout << "Found BOS event: " <<  eventcounter << endl;
    cout << "--------------" << endl;
    cout << "targbit = " << targbit << endl;
    RFintegral = 0;
    goodevent_flag = 0;
  }
  else if( (PhysicsEvent[reference_index+17]>0&&PhysicsEvent[reference_index+17]<10) || PhysicsEvent[28]==0x08020000)
  {//EOS
    goodevent_flag = 1;
  }
  else
  {
    cout << "Unknown Event" << endl;
  }

  return goodevent_flag;
}

void E906ScalerDAQ::E906DAQ_subtract_lastevent()
{
  ULong_t tmp_long[100];
  for(Int_t i=0;i<30;++i)
  {
    tmp_long[i] = Last_PhysicsEvent[i];
    Last_PhysicsEvent[i]=PhysicsEvent[i];
    PhysicsEvent[i] -= tmp_long[i];
  }
}

void E906ScalerDAQ::E906DAQ_getdata(ULong_t *kazdata)
{

  PhysicsEvent = kazdata;


  //The basic structure of the output CODA file is as follows.  For each event, there are header words that we don't really care about. 
  //PhysicsEvent[11] and [12] are the markers denoting the start of the scaler data, and are set as 0xe906f002 (hex) (or 3909545986(dec)) and 0xe906f003 (hex) (or 3909545987(dec)).  After this word, comes the first scaler channel that we care about.  Of the 32 scaler channels available, we are using channels 16-31 (counting from 0).  


  //PhysicsEvent[13] = channel16 = nothing
  //PhysicsEvent[14] = channel17 = NIMch3 = Fast clock.  This was a gategenerator outputting a 7.3kHz clock during run I.
  //PhysicsEvent[15 = channel18 = 60Hz pulser
  //PhysicsEvent[16 = channel19 = NIMch5 = BOS
  //PhysicsEvent[17] = channel20 = NIMch6 = EOS
  //PhysicsEvent[18] = channel21 = NIMch7 = data
  //PhysicsEvent[19] = channel22 = NIMch8 = Y4B2 during Run I
  //PhysicsEvent[20] = channel23 = encoder sign
  //PhysicsEvent[21] = encoder steps (via gategenerator)
  //PhysicsEvent[22] = SAMCH12 = NIMch10 = targbit1
  //PhysicsEvent[23] = SAMCH13 = NIMch11 = targbit2
  //PhysicsEvent[24] = SAMCH15 = NIMch12 = targbit3

  if(eventcounter%1000==0)
  {
    cout << eventcounter << " evt this spill, " << eventsThisRun << " counts this run" << "\r";
    fflush(stdout);// flush or else we'll see nothing
 //   cout << "fortest" <<endl;
  }

  //The following loop is designed to do two things.  Output every word from the particular event into a file, and to slowdown the main ET system to check if the ET data buffer is large (or numerous) enough to absorb all the events from one spill.  
  //THIS SHOULD BE COMMENTED OUT DURING PRODUCTION RUNNING TO PREVENT SLOW-DOWN OF DATA-TAKING

  //COMMENTED OUT FOR PRODUCTION RUNNING
  // for(Int_t i=0;i<20;i++){
  //   debugout << eventcounter << "  " << i << "  " << hex << PhysicsEvent[i] << endl;
  // }
}

Int_t E906ScalerDAQ::E906DAQ_return_eventcounter()
{
  return eventcounter;
}

void E906ScalerDAQ::E906DAQ_output_root()
{
  cout << "Outputting histograms to ROOTFILE. Iteration " << rootspills << endl;

  TString rootDir = c_topDir + "/rootfiles";
  PrepareDir(rootDir);
  TString rootfile = rootDir + Form("/scaler_%d.root", runnumber_int);
  TFile *updatefile = new TFile(rootfile,"update");//In principle, update also creates if file doesn't exist...

  //Set histogram name to spill count
  cout << "Outputting rootfile to : " << rootfile << endl;

  if(spillcounter!=spillcounter_previous)
  {
    sprintf(text,"spill%d",spillcounter);
    samecounter = 0;
    spillcounter_previous = spillcounter;
  }
  else
  {
    ++samecounter;
    sprintf(text,"spill%d_v%d",spillcounter,samecounter);
  }

  //GetDirectory returns 0 if dir does not exist
  //mkdir returns 0 if dir does exist, so only use it if the dir doesn't yet exit
  dir = updatefile->GetDirectory(text);
  if(0==dir)
    dir = updatefile->mkdir(text);
  dir->cd();

  TString hi;
  TTree *datatree = new TTree("tree","tree");
  datatree->Branch("bunch13",&bunch13,"bunch13/F");
  datatree->Branch("turn13",&turn13,"turn13/F");
  datatree->Branch("nbsyd",&nbsyd,"nbsyd/F");
  datatree->Branch("nm3sem",&nm3sem,"nm3sem/F");
  datatree->Branch("dutyfactor",&dutyfactor_noMI,"dutyfactor/F");

  for(Int_t i=0;i<3;++i)
  {
    for(Int_t j=0;j<32;++j)
    {
      //hi = TString(scalernames[i][j]);
      //strcpy(text,scalernames[i][j]);
      //variablename
      hi = TString(variablename[i][j]);
      strcpy(text,variablename[i][j]);
      strcat(text,"/I");
      datatree->Branch(hi,&sc_ch[i][j],text);
    }
  }
  datatree->Fill();
  datatree->Write();

  sprintf(text,"raw_%d_%d",spillcounter,samecounter);
  histo_raw_spectrum->SetName(text);
  histo_raw_spectrum->SetTitle(text); 
  sprintf(text,"FFT_%d_%d",spillcounter,samecounter);
  fft->SetName(text);
  fft->SetTitle(text);

  histo_raw_spectrum->Write();
  fft->Write();

  for(Int_t i=0;i<num60hz_frames;i++){
    histo60hz[i]->Write();
  }

  dir = 0;
  updatefile->Close(); // must close, or else won't be able to open after next spill
}
void E906ScalerDAQ::E906DAQ_output_ascii(){
  int counter = 0;
  int note = 0;
 // int timestamp_linux = 0;
  TString topAsciiDir = c_topDir + "/ascii";
  TString asciiDir = topAsciiDir + "/" + datestamp;
  PrepareDir(asciiDir);
  cout << "Ascii output dir = " << asciiDir << endl;

  ofstream outascii;
  plask_flag = true;
  fp = popen("plask -rt Sea2sc -cmp EBe906sc -cmpEventNumber","r");
  if(fp==NULL)
  {
    cout << "Failed to grab runnumber" << endl;
    plask_flag = false;
    killscript = 1;
    evnum_int = -9999;
  }

  while(fgets(evnum,sizeof(evnum)-1,fp)!=NULL){ //get runnumber into char
    //    cout << "RUNNUMBER!!!!" << runnumber << endl;
  }
  evnum_int = atoi(evnum); //convert char to int
  pclose(fp);//close system command
  
    //output to epics
    sprintf(text,"caput -t NM3SEM %.4f",nm3sem);
    system(text);
    //sprintf(text,"caput -t coda_evnum %d",evnum_int);
    //system(text);
    sprintf(text,"caput -t local_evnum %d",eventcounter);
    system(text);
    sprintf(text,"caput -t plask_evnum %d",plask_flag);
    system(text);
    sprintf(text,"caput -t plask_runnumber %d",runnumber_flag);
    system(text);
    sprintf(text,"caput -t turn13 %.4f",turn13);
    system(text);
    sprintf(text,"caput -t bunch13 %.4f",bunch13);
    system(text);
    sprintf(text,"caput -t nbsyd %.4f",nbsyd);
    system(text);
 // const TString asciiname = asciiDir + Form("/scaler_ascii_run%d.tsv", runnumber_int);
  const TString asciiname = asciiDir + Form("/spill_%09d_ScalerDAQ.tsv", spillcounter);
  outascii.open(asciiname, ios::out | ios::app );  
 // if(runnumber_int!=runnumber_prev)
 // {
   // outascii << "timestamp  runnumber   spillcounter NM3SEM coda_evnum local_evnum  plask_evnum plask_runnumber duty_factor_withMI   duty_factor_noMI turn13 bunch13 nbsyd ";
    
    fp2 = popen("date \'+%s\'","r");//get timestamp
    while(fgets(timestamp_linux,sizeof(timestamp_linux)-1,fp2)!=NULL){
    }
    timestamp_linux[10] = '\0';
    cout <<"linux time:   " << timestamp_linux <<endl;
    pclose(fp2);

    outascii << "spill_id" << "\t" << timestamp_linux << "\t" << spillcounter << "\t" << note <<endl;
    outascii << "timestamp" << "\t" << timestamp_linux << "\t" << timestamp_int << "\t" << note << endl;
    outascii << "runnumber" << "\t" << timestamp_linux << "\t" << runnumber_int << "\t" << note << endl;
    outascii << "NM3SEM" << "\t" << timestamp_linux << "\t" << nm3sem << "\t" << note << endl;
    outascii << "coda_evnum" << "\t" << timestamp_linux << "\t" << evnum_int << "\t" << note << endl;
    outascii << "local_evnum" << "\t" << timestamp_linux << "\t" << eventcounter << "\t" << note << endl;
    outascii << "plask_evnum" << "\t" << timestamp_linux << "\t" << plask_flag << "\t" << note << endl;
    outascii << "plask_runnumber" << "\t" << timestamp_linux << "\t" << runnumber_flag << "\t" << note << endl;
    outascii << "DUTYFACTORMI_SC" << "\t" << timestamp_linux << "\t" << 100*dutyfactor_includeMI << "\t" << "duty_factor_withMI" << endl;
    outascii << "DUTYFACTOR_SC" << "\t" << timestamp_linux << "\t" << 100*dutyfactor_noMI << "\t" << "duty_factor_noMI" << endl;
    outascii << "turn13" << "\t" << timestamp_linux << "\t" << turn13 << "\t" << note << endl;
    outascii << "bunch13" << "\t" << timestamp_linux << "\t" << bunch13 << "\t" << note << endl;
    outascii << "nbsyd" << "\t" << timestamp_linux << "\t" << nbsyd << "\t" << note << endl;
    
    
    
    
    
   // for(Int_t i=0;i<3;++i)
   // {
   //   for(Int_t j=0;j<32;++j)
   //   {
   //     outascii << scalernames[i][j] << " ";
   //   }
   // }
   // outascii << " end " << endl;
   // eventsThisRun = 0;
   // }
   // outascii << timestamp_int << "  " << runnumber_int << "  " << spillcounter << "  " << nm3sem << "  " << evnum_int << "  " << eventcounter << "  " << plask_flag << "  " << runnumber_flag << "  " << 100*dutyfactor_includeMI << "  " << 100*dutyfactor_noMI <<  " " << turn13 << "  " << bunch13  << "  " << nbsyd << "  ";

 // for(Int_t i=0;i<110;i++)
 // {
   // if(PhysicsEvent[i]==0x08020000||PhysicsEvent[i]==0x08030000||PhysicsEvent[i]==0x08040000)
   // {//that's 0xE906F004, 5, or 6
      //      scaler = PhysicsEvent[i+1]; //i+1 is scaler board ID
    //  ++counter;
     // for(Int_t j=0;j<32;++j)
     // { //j is scaler channel
     //   outascii << PhysicsEvent[i+1+j] << " ";
     // }
   // }
 // }
 // outascii << endl;
 // outascii << "end";
 // outascii << endl;
 // outascii.close();

  for(Int_t i=0;i<110;i++)
  {
    //cout << "test afjafj" << endl;
    if(PhysicsEvent[i]==0x08020000)
    {
      //cout << "scaler1" << endl;
      ++counter;
      for(Int_t j=0;j<32;++j)
      {//j is scaler channel
        if (scalerstatus[0][j] == 0){
        outascii << scalernames[0][j] << "\t" << timestamp_linux << "\t" << PhysicsEvent[i+1+j] << "\t" << variablename[0][j] << endl;
        }
      }
    }else if(PhysicsEvent[i]==0x08030000)
    {
      //cout << "scaler2" << endl;
      ++counter;
      for(Int_t j=0;j<32;++j)
      {//j is scaler channel
        //cout << "scalerstatus1=" << scalerstatus[1][j] << endl;
        if (scalerstatus[1][j] == 0){
        //cout << "scalerstatus2=" << scalerstatus[1][j] << endl;
        outascii << scalernames[1][j] << "\t" << timestamp_linux << "\t" << PhysicsEvent[i+1+j] << "\t" << variablename[1][j] << endl;
        }
      }
    }else if(PhysicsEvent[i]==0x08040000)
      {//j is scaler channel
       //cout << "scaler1" << endl; 
       ++counter;
       for(Int_t j=0;j<32;++j)
       {//j is scaler channel
         if (scalerstatus[2][j] == 0){
         outascii << scalernames[2][j] << "\t" << timestamp_linux << "\t" << PhysicsEvent[i+1+j] << "\t" << variablename[2][j] << endl;
         }
       }
     }
   }

  outascii.close();



  //copy ascii file to /data2
  //Daily data is stored in a separate folder
  time_t now = time(0);
  tm *now_tm = localtime(&now);

  char dummy[5];
  sprintf(dummy, "%04d", 1900+now_tm->tm_year);
  TString year = dummy;
  sprintf(dummy, "%02d", 1+now_tm->tm_mon);
  TString month = dummy;
  sprintf(dummy, "%02d", now_tm->tm_mday);
  TString day = dummy;
  sprintf(dummy, "%02d", now_tm->tm_hour);
  TString hour = dummy;
  sprintf(dummy, "%02d", now_tm->tm_min);
  TString minute = dummy;
  sprintf(dummy, "%02d", now_tm->tm_sec);
  TString sec = dummy;

  TString ascii2Dir = "/data2/e1039_data/slowcontrol_data";
  TString raidDir = ascii2Dir + "/" +"slowcontrol_" + year + "_" + month + "_" + day;
  PrepareDir( raidDir );

  system( Form( "cp %s %s", asciiname.Data(), raidDir.Data() ) );
  //system( Form( "cp %s /data2/e1039_data/slowcontrol_data/ascii", asciiname.Data() ) );

  runnumber_prev = runnumber_int;
}

Bool_t E906ScalerDAQ::E906DAQ_return_eosflag()
{
  return eosflag;
}
