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

#include "QualityControl/QcInfoLogger.h"
#include "FT0/BasicDigitQcTask.h"
#include "FT0/Utilities.h"
#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"
#include <Framework/InputRecord.h>

namespace o2::quality_control_modules::ft0
{

BasicDigitQcTask::~BasicDigitQcTask()
{
}

void BasicDigitQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize BasicDigitQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  mChargeHistogram = std::make_unique<TH1F>("Charge", "Charge", 200, 0, 200);
  mTimeHistogram = std::make_unique<TH1F>("Time", "Time", 400, -2000, 2000);
  mAmplitudeAndTime = std::make_unique<TH2F>("ChargeAndTime", "ChargeAndTime", 10, 0, 200, 10, 0, 200);
  int nchannels = 208;
  // int nchannelsA = 96, nchannelsC = 112;
  mChargePerChannel = std::make_unique<TH2F>("ChargePerChannel", "ChargePerChannel", nchannels, 0, nchannels, 200, 0, 200);
  mTimePerChannel = std::make_unique<TH2F>("TimePerChannel", "TimePerChannel", nchannels, 0, nchannels, 80, -200, 200);

  mTriggerHistChargeA = std::make_unique<TH1F>("chargeA", "trigger-chargeA", 200, 0, 200);
  mTriggerHistChargeC = std::make_unique<TH1F>("chargeC", "trigger-chargeC", 200, 0, 200);
  mTriggerHistTimeA = std::make_unique<TH1F>("timeA", "trigger-timeA", 80, -200, 200);
  mTriggerHistTimeC = std::make_unique<TH1F>("timeC", "trigger-timeC", 80, -200, 200);

  mTriggerHistOrA = std::make_unique<TH1F>("orA", "trigger-orA", 2, -0.5, 1.5);
  mTriggerHistOrC = std::make_unique<TH1F>("orC", "trigger-orC", 2, -0.5, 1.5);
  mTriggerHistCen = std::make_unique<TH1F>("Cen", "trigger-Cen", 2, -0.5, 1.5);
  mTriggerHistSCen = std::make_unique<TH1F>("SCen", "trigger-SCen", 2, -0.5, 1.5);
  mTriggerHistVertex = std::make_unique<TH1F>("Vertex", "trigger-Vertex", 2, -0.5, 1.5);
  mTriggerHistTriggers = std::make_unique<TH1F>("triggers", "triggers", 5, 1, 6);
  mTriggerHistTriggersCorr = std::make_unique<TH2F>("triggersCorr", "triggers correlation", 5, 1, 6, 5, 1, 6);

  mTriggerHistTriggers->GetXaxis()->SetBinLabel(1, "orA");
  mTriggerHistTriggers->GetXaxis()->SetBinLabel(2, "orC");
  mTriggerHistTriggers->GetXaxis()->SetBinLabel(3, "Vertex");
  mTriggerHistTriggers->GetXaxis()->SetBinLabel(4, "SemiCentral");
  mTriggerHistTriggers->GetXaxis()->SetBinLabel(5, "Central");

  mTriggerHistTriggersCorr->GetXaxis()->SetBinLabel(1, "orA");
  mTriggerHistTriggersCorr->GetXaxis()->SetBinLabel(2, "orC");
  mTriggerHistTriggersCorr->GetXaxis()->SetBinLabel(3, "Vertex");
  mTriggerHistTriggersCorr->GetXaxis()->SetBinLabel(4, "SemiCentral");
  mTriggerHistTriggersCorr->GetXaxis()->SetBinLabel(5, "Central");
  mTriggerHistTriggersCorr->GetYaxis()->SetBinLabel(1, "orA");
  mTriggerHistTriggersCorr->GetYaxis()->SetBinLabel(2, "orC");
  mTriggerHistTriggersCorr->GetYaxis()->SetBinLabel(3, "Vertex");
  mTriggerHistTriggersCorr->GetYaxis()->SetBinLabel(4, "SemiCentral");
  mTriggerHistTriggersCorr->GetYaxis()->SetBinLabel(5, "Central");



  mTTree = std::make_unique<TTree>("EventTree", "EventTree");

  getObjectsManager()->startPublishing(mChargeHistogram.get());
  getObjectsManager()->startPublishing(mTimeHistogram.get());
  getObjectsManager()->startPublishing(mTTree.get());
  getObjectsManager()->startPublishing(mAmplitudeAndTime.get());
  getObjectsManager()->startPublishing(mChargePerChannel.get());
  getObjectsManager()->startPublishing(mTimePerChannel.get());

  getObjectsManager()->startPublishing(mTriggerHistChargeA.get());
  getObjectsManager()->startPublishing(mTriggerHistChargeC.get());
  getObjectsManager()->startPublishing(mTriggerHistTimeA.get());
  getObjectsManager()->startPublishing(mTriggerHistTimeC.get());

  getObjectsManager()->startPublishing(mTriggerHistOrA.get());
  getObjectsManager()->startPublishing(mTriggerHistOrC.get());
  getObjectsManager()->startPublishing(mTriggerHistCen.get());
  getObjectsManager()->startPublishing(mTriggerHistSCen.get());
  getObjectsManager()->startPublishing(mTriggerHistVertex.get());
  getObjectsManager()->startPublishing(mTriggerHistTriggers.get());
  getObjectsManager()->startPublishing(mTriggerHistTriggersCorr.get());

  mTTree->Branch("EventWithChannelData", &mEvent);

}

void BasicDigitQcTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity" << activity.mId << ENDM;
  mTimeHistogram->Reset();
  mChargeHistogram->Reset();
  mTTree->Reset();
}

void BasicDigitQcTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void BasicDigitQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto channels = ctx.inputs().get<gsl::span<o2::ft0::ChannelData>>("channels");
  auto digits = ctx.inputs().get<gsl::span<o2::ft0::Digit>>("digits");

  std::vector<o2::ft0::ChannelData> channelDataCopy(channels.begin(), channels.end());
  std::vector<o2::ft0::Digit> digitDataCopy(digits.begin(), digits.end());

  int d=0, c=0;
  for (auto& digit : digits) {
    auto currentChannels = digit.getBunchChannelData(channels);
    auto timestamp = o2::InteractionRecord::bc2ns(digit.getBC(), digit.getOrbit());
    mEvent = EventWithChannelData{ digit.getEventID(), digit.getBC(), digit.getOrbit(), timestamp, std::vector<o2::ft0::ChannelData>(currentChannels.begin(), currentChannels.end()) };
    mTTree->Fill();


    mTriggerHistChargeA->Fill(digit.mTriggers.amplA);
    mTriggerHistChargeC->Fill(digit.mTriggers.amplC);

    mTriggerHistTimeA->Fill(digit.mTriggers.timeA);
    mTriggerHistTimeC->Fill(digit.mTriggers.timeC);

    mTriggerHistOrA->Fill(digit.mTriggers.getOrA());
    mTriggerHistOrC->Fill(digit.mTriggers.getOrC());
    mTriggerHistCen->Fill(digit.mTriggers.getCen());
    mTriggerHistSCen->Fill(digit.mTriggers.getSCen());
    mTriggerHistVertex->Fill(digit.mTriggers.getVertex());

    mTriggerHistTriggers->Fill(digit.mTriggers.getOrA()*1);
    mTriggerHistTriggers->Fill(digit.mTriggers.getOrC()*2);
    mTriggerHistTriggers->Fill(digit.mTriggers.getVertex()*3);
    mTriggerHistTriggers->Fill(digit.mTriggers.getSCen()*4);
    mTriggerHistTriggers->Fill(digit.mTriggers.getCen()*5);

    bool triggers[5] = {digit.mTriggers.getOrA(), digit.mTriggers.getOrC(), digit.mTriggers.getVertex(), digit.mTriggers.getSCen(), digit.mTriggers.getCen()};
    int idx1=1;
    for (auto t1 : triggers){
      int idx2=1;
      for (auto t2 : triggers){
        mTriggerHistTriggersCorr->Fill(t1*idx1, t2*idx2);
        idx2++;
      }
      idx1++;
    }


    // return char
    // mTriggerHistEventFlags->Fill(digit.mTriggers.eventFlags);
    // mTriggerHistTriggerSignals->Fill(digit.mTriggers.triggersignals;
    // mTriggerHistNChanA->Fill(digit.mTriggers.nChanA);
    // mTriggerHistNChanC->Fill(digit.mTriggers.nChanC);


    d++;
    for (auto& channel : currentChannels) {
      mChargeHistogram->Fill(channel.QTCAmpl);
      mTimeHistogram->Fill(channel.CFDTime);
      mAmplitudeAndTime->Fill(channel.QTCAmpl, channel.CFDTime);
      mChargePerChannel->Fill(channel.ChId, channel.QTCAmpl);
      mTimePerChannel->Fill(channel.ChId, channel.CFDTime);
      c++;
      // std::cout << "Current digit:channel = " << d << ":" << c << std::endl;
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

void BasicDigitQcTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void BasicDigitQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void BasicDigitQcTask::reset()
{
  // clean all the monitor objects here

  mTimeHistogram->Reset();
  mChargeHistogram->Reset();
  mTTree->Reset();
  mAmplitudeAndTime->Reset();
}

} // namespace o2::quality_control_modules::ft0
