// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   BasicDigitQcTask.cxx
/// \author Milosz Filus
///

#include "TCanvas.h"
#include "TH1.h"
#include "TFile.h"
#include "TGraph.h"
#include "TDatime.h"

#include "QualityControl/QcInfoLogger.h"
#include "FT0/BasicDigitQcTaskPlus.h"
#include "FT0/Utilities.h"
#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"
#include <Framework/InputRecord.h>

//#include <unistd.h>


namespace o2::quality_control_modules::ft0
{

BasicDigitQcTaskPlus::~BasicDigitQcTaskPlus()
{
}

void BasicDigitQcTaskPlus::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize BasicDigitQcTaskPlus" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  mChargeHistogram = std::make_unique<TH1F>("Charge", "Charge", 200, 0, 200);
  mChargeHistogramCurrent = std::make_unique<TH1F>("ChargeCurrent", "ChargeCurrent", 200, 0, 200);
  mChargeTrend = std::make_unique<TGraph>(1);
  mTimeHistogram = std::make_unique<TH1F>("Time", "Time", 1200, -1200, 1200);
  mAmplitudeAndTime = std::make_unique<TH2F>("ChargeAndTime", "ChargeAndTime", 10, 0, 200, 10, 0, 200);
  mTTree = std::make_unique<TTree>("EventTree", "EventTree");

  getObjectsManager()->startPublishing(mChargeHistogram.get());
  getObjectsManager()->startPublishing(mChargeHistogramCurrent.get());
  getObjectsManager()->startPublishing(mChargeTrend.get());
  getObjectsManager()->startPublishing(mTimeHistogram.get());
  getObjectsManager()->startPublishing(mTTree.get());
  getObjectsManager()->startPublishing(mAmplitudeAndTime.get());


  //Float_t randFloat;
  //mTTree->Branch("randomFloat", &randFloat);
  //EventWithChannelData event;
  mTTree->Branch("EventWithChannelData", &event);

  mChargeTrend->SetName("ChargeTrend");
  mChargeTrend->SetTitle("mean of charge per cycle");
  mChargeTrend->GetXaxis()->SetTimeDisplay(1);
  mChargeTrend->GetXaxis()->SetTimeFormat("%H:%M:%S | %d-%m-%Y");
  mChargeTrend->SetMarkerStyle(8);
}

void BasicDigitQcTaskPlus::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity" << activity.mId << ENDM;
  mTimeHistogram->Reset();
  mChargeHistogram->Reset();
  mChargeHistogramCurrent->Reset();
  mAmplitudeAndTime->Reset();
  mChargeTrend->Set(0);
  mTTree->Reset();
}

void BasicDigitQcTaskPlus::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle " << std::to_string(countCycles) << ENDM;
  countCycles++;
  countIter=0;
}

void BasicDigitQcTaskPlus::monitorData(o2::framework::ProcessingContext& ctx)
{

  if (!(countIterTotal%100)) ILOG(Info, Support) << "\n\nmonitorData() iter: " << std::to_string(countIter) << ", total: " << std::to_string(countIterTotal) << ENDM;
  countIter++;
  countIterTotal++;
  // get separate hist for each TF
  //mTimeHistogram->Reset();
//   mChargeHistogram->Reset();
  mChargeHistogramCurrent->Reset();
  //mTTree->Reset();
//  mAmplitudeAndTime->Reset();
  //reset();
   
  auto channels = ctx.inputs().get<gsl::span<o2::ft0::ChannelData>>("channels");
  auto digits = ctx.inputs().get<gsl::span<o2::ft0::Digit>>("digits");

  std::vector<o2::ft0::ChannelData> channelDataCopy(channels.begin(), channels.end());
  std::vector<o2::ft0::Digit> digitDataCopy(digits.begin(), digits.end());


  int d=0;
  int c=0;
  for (auto& digit : digits) {
    auto currentChannels = digit.getBunchChannelData(channels);
    auto timestamp = o2::InteractionRecord::bc2ns(digit.getBC(), digit.getOrbit());
   
    timestamps.push_back(timestamp);
    event = EventWithChannelData{ digit.getEventID(), digit.getBC(), digit.getOrbit(), timestamp, std::vector<o2::ft0::ChannelData>(currentChannels.begin(), currentChannels.end()) };
    //randFloat = d*d*d/(2+d);
    mTTree->Fill();
    //ILOG(Info) << "\t\t\tdigit: " << std::to_string(d) << "\n";
    d++;
    
    
    for (auto& channel : currentChannels) {
      mChargeHistogram->Fill(channel.QTCAmpl);
      mChargeHistogramCurrent->Fill(channel.QTCAmpl);
      mTimeHistogram->Fill(channel.CFDTime);
      mAmplitudeAndTime->Fill(channel.QTCAmpl, channel.CFDTime);
      //ILOG(Info) << "\t\t\t\tchannel: " << std::to_string(c) << "\n";
      //std::cout << "\t\tloop [" << d <<"/"<< c << "]: " << "event=" <<digit.getEventID() << ", orbit="<< digit.getOrbit() << ", BC="<<digit.getBC()<<", timestamp="<<timestamp  << "\n";
      c++;
      //unsigned int microseconds=100*1000;
      //usleep( microseconds );
      //boost::this_thread::sleep( boost::posix_time::milliseconds(10) );
    }
  }

  
  // saving TTree if you want to read it from disk
  // its required to make a copy of tree - but it is only for debbuging

  // auto hFile = std::make_unique<TFile>("debugTree.root", "RECREATE");
  // auto treeCopy = mTTree->CloneTree();
  // treeCopy->SetEntries();
  // treeCopy->Write();
  // mTTree->Print();
  // hFile->Close();
}

void BasicDigitQcTaskPlus::endOfCycle()
{
  mChargeTrend->SetPoint(mChargeTrend->GetN(), TDatime().Convert(), mChargeHistogramCurrent->GetMean());
  ILOG(Info, Support) << "endOfCycle" << ENDM;
  ILOG(Info) << "\n\n\nLegth of timestamps vector: " << timestamps.size() << "\n\n\n\n" << ENDM;
  
  double ts_start=*std::min_element(timestamps.begin(), timestamps.end());
  double ts_stop=*std::max_element(timestamps.begin(), timestamps.end());
  double ts_mean=0;
  //std::cout << ts_start << ts_stop << ts_mean << "\n";
  //getObjectsManager()->addMetadata(mChargeHistogramCurrent->GetName(), "timeStart", std::to_string(ts_start));
  //getObjectsManager()->addMetadata(mChargeHistogramCurrent->GetName(), "timeStop" , std::to_string(ts_stop));
  //getObjectsManager()->addMetadata(mChargeHistogramCurrent->GetName(), "timeMean" , std::to_string(ts_mean));

  double ts_sum = 0; 
  //ILOG(Info) << "timestamps vector is currently = \n" << ENDM;
  for (auto const& ts : timestamps){
    ts_sum += ts;
    //ILOG(Info) << ts << ", " << ENDM ;
  }
  //ILOG(Info) << "\n(end of vector)" <<ENDM;
  ts_mean = ts_sum / timestamps.size();
  
  
  getObjectsManager()->getMonitorObject(mChargeHistogramCurrent->GetName())->addOrUpdateMetadata("timeStart", std::to_string(ts_start));
  getObjectsManager()->getMonitorObject(mChargeHistogramCurrent->GetName())->addOrUpdateMetadata("timeStop", std::to_string(ts_stop));
  getObjectsManager()->getMonitorObject(mChargeHistogramCurrent->GetName())->addOrUpdateMetadata("timeMean", std::to_string(ts_mean));
  std::map<std::string, std::string> myMap = getObjectsManager()->getMonitorObject(mChargeHistogramCurrent->GetName())->getMetadataMap();

  for(auto elem : myMap)
  {
     ILOG(Info) << "\n\nnext map elem: " << elem.first << " " << elem.second << "\n\n\n" << ENDM;
  }
  timestamps={};

  //auto mo = getObjectsManager()->getMonitorObject(mChargeHistogramCurrent->GetName());
  //qcdb.storeMO(mo, 1234, 4321);


}

void BasicDigitQcTaskPlus::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void BasicDigitQcTaskPlus::reset()
{
  // clean all the monitor objects here

  mTimeHistogram->Reset();
  mChargeHistogram->Reset();
  mChargeHistogramCurrent->Reset();
  mChargeTrend->Set(0);
  mTTree->Reset();
  mAmplitudeAndTime->Reset();
}

} // namespace o2::quality_control_modules::ft0
