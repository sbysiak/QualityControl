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
  mTimeHistogram->Reset();
  mChargeHistogram->Reset();
  mChargeHistogramCurrent->Reset();
  mChargeTrend->Set(0);
  mTTree->Reset();
  mAmplitudeAndTime->Reset();
   
  auto channels = ctx.inputs().get<gsl::span<o2::ft0::ChannelData>>("channels");
  auto digits = ctx.inputs().get<gsl::span<o2::ft0::Digit>>("digits");

  std::vector<o2::ft0::ChannelData> channelDataCopy(channels.begin(), channels.end());
  std::vector<o2::ft0::Digit> digitDataCopy(digits.begin(), digits.end());

  EventWithChannelData event;
  mTTree->Branch("EventWithChannelData", &event);

  int d=0;
  int c=0;
  for (auto& digit : digits) {
    auto currentChannels = digit.getBunchChannelData(channels);
    auto timestamp = o2::InteractionRecord::bc2ns(digit.getBC(), digit.getOrbit());
    event = EventWithChannelData{ digit.getEventID(), digit.getBC(), digit.getOrbit(), timestamp, std::vector<o2::ft0::ChannelData>(currentChannels.begin(), currentChannels.end()) };
    //mTTree->Fill();
    //ILOG(Info) << "\t\t\tdigit: " << std::to_string(d) << "\n";
    d++;
    
    
    for (auto& channel : currentChannels) {
      mChargeHistogram->Fill(channel.QTCAmpl);
      mChargeHistogramCurrent->Fill(channel.QTCAmpl);
      mTimeHistogram->Fill(channel.CFDTime);
      mAmplitudeAndTime->Fill(channel.QTCAmpl, channel.CFDTime);
      //ILOG(Info) << "\t\t\t\tchannel: " << std::to_string(c) << "\n";
      c++;
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
