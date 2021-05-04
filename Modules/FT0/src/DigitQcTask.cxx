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
/// \file   DigitQcTask.cxx
/// \author Artur Furs afurs@cern.ch

#include "TCanvas.h"
#include "TH1.h"
#include "TH2.h"

#include "QualityControl/QcInfoLogger.h"
#include "FT0/DigitQcTask.h"
#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"
#include <Framework/InputRecord.h>
#include <TROOT.h>

namespace o2::quality_control_modules::ft0
{

DigitQcTask::~DigitQcTask()
{
  delete mListHistGarbage;
}

void DigitQcTask::rebinFromConfig()
{
  auto rebinHisto = [](std::string hName, std::string binning){
    vector<std::string> tokenizedBinning;
    boost::split(tokenizedBinning, binning, boost::is_any_of(","));
    if(tokenizedBinning.size() == 3){
      auto htmp = (TH1F*) gROOT->FindObject(hName.data());
      if (htmp->GetDimension() == 1) htmp->SetBins(std::atof(tokenizedBinning[0].c_str()), std::atof(tokenizedBinning[1].c_str()), std::atof(tokenizedBinning[2].c_str()));
      else ILOG(Warning) << "config: trying to set 1D binning to " << htmp->GetDimension() << "D histogram" << ENDM;
    }
    else if(tokenizedBinning.size() == 6){
      auto htmp = (TH2F*) gROOT->FindObject(hName.data());
      if (htmp->GetDimension() == 2) htmp->SetBins(std::atof(tokenizedBinning[0].c_str()), std::atof(tokenizedBinning[1].c_str()), std::atof(tokenizedBinning[2].c_str()),
                                                   std::atof(tokenizedBinning[3].c_str()), std::atof(tokenizedBinning[4].c_str()), std::atof(tokenizedBinning[5].c_str()));
      else ILOG(Warning) << "config: trying to set 2D binning to " << htmp->GetDimension() << "D histogram" << ENDM;
    }
    else{
      ILOG(Warning) << "config: invalid binning parameter: " << hName << " -> "  << binning << ENDM;
    }
  };


  std::string rebinKeyword = "binning";
  for (auto& param : mCustomParameters) {
    if (param.first.rfind(rebinKeyword, 0) != 0) continue;
    std::string hName = param.first.substr(rebinKeyword.length()+1);
    std::string binning = param.second.c_str();
    if (hName.find('*') != std::string::npos) {
      for (const auto& chID : mSetAllowedChIDs) {
        std::string hNameCur = hName.substr(0,hName.find("*")) + std::to_string(chID) + hName.substr(hName.find("*")+1);
        rebinHisto(hNameCur, binning);
      }
    }
    else if (!gROOT->FindObject(hName.data())) {
      ILOG(Warning) << "config: histogram named \"" << hName << "\" not found" << ENDM;
      continue;
    }
    else {
      rebinHisto(hName, binning);
    }
  }
}



void DigitQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize DigitQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  mStateLastIR2Ch = {};
  mMapChTrgNames.insert({ o2::ft0::ChannelData::kNumberADC, "NumberADC" });
  mMapChTrgNames.insert({ o2::ft0::ChannelData::kIsDoubleEvent, "IsDoubleEvent" });
  mMapChTrgNames.insert({ o2::ft0::ChannelData::kIsTimeInfoNOTvalid, "IsTimeInfoNOTvalid" });
  mMapChTrgNames.insert({ o2::ft0::ChannelData::kIsCFDinADCgate, "IsCFDinADCgate" });
  mMapChTrgNames.insert({ o2::ft0::ChannelData::kIsTimeInfoLate, "IsTimeInfoLate" });
  mMapChTrgNames.insert({ o2::ft0::ChannelData::kIsAmpHigh, "IsAmpHigh" });
  mMapChTrgNames.insert({ o2::ft0::ChannelData::kIsEventInTVDC, "IsEventInTVDC" });
  mMapChTrgNames.insert({ o2::ft0::ChannelData::kIsTimeInfoLost, "IsTimeInfoLost" });

  mMapDigitTrgNames.insert({ o2::ft0::Triggers::bitA, "OrA" });
  mMapDigitTrgNames.insert({ o2::ft0::Triggers::bitC, "OrC" });
  mMapDigitTrgNames.insert({ o2::ft0::Triggers::bitVertex, "Vertex" });
  mMapDigitTrgNames.insert({ o2::ft0::Triggers::bitCen, "Central" });
  mMapDigitTrgNames.insert({ o2::ft0::Triggers::bitSCen, "SemiCentral" });

  mHistTime2Ch = std::make_unique<TH2F>("TimePerChannel", "Time vs Channel;Time;Channel;", 4100, -2050, 2050, sNchannels, 0, sNchannels);
  mHistTime2Ch->SetOption("colz");
  mHistAmp2Ch = std::make_unique<TH2F>("AmpPerChannel", "Amplitude vs Channel;Amp;Channel;", 4200, -100, 4100, sNchannels, 0, sNchannels);
  mHistAmp2Ch->SetOption("colz");
  mHistAmp2Adc = std::make_unique<TH2F>("AmpPerADC", "Amplitude vs ADC;Amp;ADC;", 4200, -100, 4100, sNchannels*2, 0, sNchannels*2); // 2 ADC per channel
  mHistAmp2Adc->SetOption("colz");
  mHistOrbit2BC = std::make_unique<TH2F>("OrbitPerBC", "BC-Orbit map;Orbit;BC;", 256, 0, 256, 3564, 0, 3564);
  mHistOrbit2BC->SetOption("colz");

  mHistEventDensity2Ch = std::make_unique<TH2F>("EventDensityPerChannel", "Event density(in BC) per Channel;Channel;BC;", sNchannels, 0, sNchannels, 10000, 0, 1e5);
  mHistEventDensity2Ch->SetOption("colz");

  mHistChDataBits = std::make_unique<TH2F>("ChannelDataBits", "ChannelData bits per ChannelID;Channel;Bit", sNchannels, 0, sNchannels, mMapChTrgNames.size(), 0, mMapChTrgNames.size());
  mHistChDataBits->SetOption("colz");

  for (const auto& entry : mMapChTrgNames) {
    mHistChDataBits->GetYaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
  }

  mHistTriggers = std::make_unique<TH1F>("Triggers", "Triggers from TCM", mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size());
  for (const auto& entry : mMapDigitTrgNames) {
    mHistTriggers->GetXaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
  }
  mHistNchA = std::make_unique<TH1F>("NumChannelsA", "Number of channels(TCM), side A;Nch", sNchannels, 0, sNchannels);
  mHistNchC = std::make_unique<TH1F>("NumChannelsC", "Number of channels(TCM), side C;Nch", sNchannels, 0, sNchannels);
  mHistSumAmpA = std::make_unique<TH1F>("SumAmpA", "Sum of amplitudes(TCM), side A;", 1000, 0, 1e4);
  mHistSumAmpC = std::make_unique<TH1F>("SumAmpC", "Sum of amplitudes(TCM), side C;", 1000, 0, 1e4);
  mHistAverageTimeA = std::make_unique<TH1F>("AverageTimeA", "Average time(TCM), side A", 4100, -2050, 2050);
  mHistAverageTimeC = std::make_unique<TH1F>("AverageTimeC", "Average time(TCM), side C", 4100, -2050, 2050);
  mHistChannelID = std::make_unique<TH1F>("StatChannelID", "ChannelID statistics;ChannelID", sNchannels, 0, sNchannels);
  mHistAdcID = std::make_unique<TH1F>("StatAdcID", "ADC ID = 2*ChID+kNumberADC;ADC ID", sNchannels*2, 0, sNchannels*2);
  mListHistGarbage = new TList();
  mListHistGarbage->SetOwner(kTRUE);
  std::vector<unsigned int> vecChannelIDs;
  if (auto param = mCustomParameters.find("ChannelIDs"); param != mCustomParameters.end()) {
    const auto chIDs = param->second;
    const std::string del = ",";
    vecChannelIDs = parseParameters<unsigned int>(chIDs, del);
  } else {
    for (unsigned int iCh = 0; iCh < sNchannels; iCh++)
      vecChannelIDs.push_back(iCh);
  }
  for (const auto& entry : vecChannelIDs) {
    mSetAllowedChIDs.insert(entry);
  }

  for (const auto& chID : mSetAllowedChIDs) {
    auto pairHistAmp = mMapHistAmp1D.insert({ chID, new TH1F(Form("Amp_channel%i", chID), Form("Amplitude, channel %i", chID), 4200, -100, 4100) });
    auto pairHistAdc0Amp = mMapHistAdc0Amp1D.insert({ chID, new TH1F(Form("Amp_channel%i_ADC0", chID), Form("Amplitude, channel %i ADC 0", chID), 4200, -100, 4100) });
    auto pairHistAdc1Amp = mMapHistAdc1Amp1D.insert({ chID, new TH1F(Form("Amp_channel%i_ADC1", chID), Form("Amplitude, channel %i ADC 1", chID), 4200, -100, 4100) });
    auto pairHistTime = mMapHistTime1D.insert({ chID, new TH1F(Form("Time_channel%i", chID), Form("Time, channel %i", chID), 4100, -2050, 2050) });
    auto pairHistBits = mMapHistPMbits.insert({ chID, new TH1F(Form("Bits_channel%i", chID), Form("Bits, channel %i", chID), mMapChTrgNames.size(), 0, mMapChTrgNames.size()) });
    auto pairHistAmpVsTime = mMapHistAmpVsTime.insert({ chID, new TH2F(Form("Amp_vs_time_channel%i", chID), Form("Amplitude vs time, channel %i;Amp;Time", chID), 420, -100, 4100, 410, -2050, 2050) });
    for (const auto& entry : mMapChTrgNames) {
      pairHistBits.first->second->GetXaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
    }
    if (pairHistAmp.second) {
      getObjectsManager()->startPublishing(pairHistAmp.first->second);
      mListHistGarbage->Add(pairHistAmp.first->second);
    }
    if (pairHistAdc0Amp.second) {
      getObjectsManager()->startPublishing(pairHistAdc0Amp.first->second);
      mListHistGarbage->Add(pairHistAdc0Amp.first->second);
    }
    if (pairHistAdc1Amp.second) {
      getObjectsManager()->startPublishing(pairHistAdc1Amp.first->second);
      mListHistGarbage->Add(pairHistAdc1Amp.first->second);
    }
    if (pairHistTime.second) {
      mListHistGarbage->Add(pairHistTime.first->second);
      getObjectsManager()->startPublishing(pairHistTime.first->second);
    }
    if (pairHistBits.second) {
      mListHistGarbage->Add(pairHistBits.first->second);
      getObjectsManager()->startPublishing(pairHistBits.first->second);
    }
    if (pairHistAmpVsTime.second) {
      mListHistGarbage->Add(pairHistAmpVsTime.first->second);
      getObjectsManager()->startPublishing(pairHistAmpVsTime.first->second);
    }
  }

  rebinFromConfig();

  getObjectsManager()->startPublishing(mHistTime2Ch.get());
  getObjectsManager()->startPublishing(mHistAmp2Ch.get());
  getObjectsManager()->startPublishing(mHistAmp2Adc.get());
  getObjectsManager()->startPublishing(mHistOrbit2BC.get());
  getObjectsManager()->startPublishing(mHistEventDensity2Ch.get());
  getObjectsManager()->startPublishing(mHistChDataBits.get());
  getObjectsManager()->startPublishing(mHistTriggers.get());
  getObjectsManager()->startPublishing(mHistNchA.get());
  getObjectsManager()->startPublishing(mHistNchC.get());
  getObjectsManager()->startPublishing(mHistSumAmpA.get());
  getObjectsManager()->startPublishing(mHistSumAmpC.get());
  getObjectsManager()->startPublishing(mHistAverageTimeA.get());
  getObjectsManager()->startPublishing(mHistAverageTimeC.get());
  getObjectsManager()->startPublishing(mHistChannelID.get());
  getObjectsManager()->startPublishing(mHistAdcID.get());
}

void DigitQcTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity" << activity.mId << ENDM;
  mHistTime2Ch->Reset();
  mHistAmp2Ch->Reset();
  mHistAmp2Adc->Reset();
  mHistOrbit2BC->Reset();
  mHistEventDensity2Ch->Reset();
  mHistChDataBits->Reset();
  mHistTriggers->Reset();
  mHistNchA->Reset();
  mHistNchC->Reset();
  mHistSumAmpA->Reset();
  mHistSumAmpC->Reset();
  mHistAverageTimeA->Reset();
  mHistAverageTimeC->Reset();
  mHistChannelID->Reset();
  mHistAdcID->Reset();
  for (auto& entry : mMapHistAmp1D) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistAdc0Amp1D) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistAdc1Amp1D) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistTime1D) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistPMbits) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistAmpVsTime) {
    entry.second->Reset();
  }
}

void DigitQcTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void DigitQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{

  auto channels = ctx.inputs().get<gsl::span<o2::ft0::ChannelData>>("channels");
  auto digits = ctx.inputs().get<gsl::span<o2::ft0::Digit>>("digits");
  bool isFirst = true;
  uint32_t firstOrbit;
  for (auto& digit : digits) {
    const auto& vecChData = digit.getBunchChannelData(channels);
    bool isTCM = true;
    if (isFirst == true) {
      firstOrbit = digit.getOrbit();
      isFirst = false;
    }
    if (digit.mTriggers.amplA == -5000 && digit.mTriggers.amplC == -5000 && digit.mTriggers.timeA == -5000 && digit.mTriggers.timeC == -5000)
      isTCM = false;
    mHistOrbit2BC->Fill(digit.getOrbit() - firstOrbit, digit.getBC());
    if (isTCM || !digit.mTriggers.getLaserBit()) {
      mHistNchA->Fill(digit.mTriggers.nChanA);
      mHistNchC->Fill(digit.mTriggers.nChanC);
      mHistSumAmpA->Fill(digit.mTriggers.amplA);
      mHistSumAmpC->Fill(digit.mTriggers.amplC);
      mHistAverageTimeA->Fill(digit.mTriggers.timeA);
      mHistAverageTimeC->Fill(digit.mTriggers.timeC);

      for (const auto& entry : mMapDigitTrgNames) {
        if (digit.mTriggers.triggersignals & (1 << entry.first))
          mHistTriggers->Fill(static_cast<Double_t>(entry.first));
      }
    }

    for (const auto& chData : vecChData) {
      int chAdc = -999;
      if (chData.ChainQTC & (1 << o2::ft0::ChannelData::kIsCFDinADCgate)){
        if (chData.ChainQTC & (1 << o2::ft0::ChannelData::kNumberADC)) chAdc = 1;
        else chAdc = 0;
      }
      int adcId = 2*chData.ChId + chAdc;
      mHistTime2Ch->Fill(static_cast<Double_t>(chData.CFDTime), static_cast<Double_t>(chData.ChId));
      mHistAmp2Ch->Fill(static_cast<Double_t>(chData.QTCAmpl), static_cast<Double_t>(chData.ChId));
      mHistAmp2Adc->Fill(static_cast<Double_t>(chData.QTCAmpl), static_cast<Double_t>(adcId));
      mHistEventDensity2Ch->Fill(static_cast<Double_t>(chData.ChId), static_cast<Double_t>(digit.mIntRecord.differenceInBC(mStateLastIR2Ch[chData.ChId])));
      mStateLastIR2Ch[chData.ChId] = digit.mIntRecord;
      mHistChannelID->Fill(chData.ChId);
      mHistAdcID->Fill(adcId);
      if (mSetAllowedChIDs.find(static_cast<unsigned int>(chData.ChId)) != mSetAllowedChIDs.end()) {
        mMapHistAmp1D[chData.ChId]->Fill(chData.QTCAmpl);
        if(chAdc == 0)      mMapHistAdc0Amp1D[chData.ChId]->Fill(chData.QTCAmpl);
        else if(chAdc == 1) mMapHistAdc1Amp1D[chData.ChId]->Fill(chData.QTCAmpl);
        mMapHistTime1D[chData.ChId]->Fill(chData.CFDTime);
        mMapHistAmpVsTime[chData.ChId]->Fill(chData.QTCAmpl, chData.CFDTime);
        for (const auto& entry : mMapChTrgNames) {
          if ((chData.ChainQTC & (1 << entry.first))) {
            mMapHistPMbits[chData.ChId]->Fill(entry.first);
          }
        }
      }
      for (const auto& entry : mMapChTrgNames) {
        if ((chData.ChainQTC & (1 << entry.first))) {
          mHistChDataBits->Fill(chData.ChId, entry.first);
        }
      }
    }
  }
}

void DigitQcTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void DigitQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void DigitQcTask::reset()
{
  // clean all the monitor objects here
  mHistTime2Ch->Reset();
  mHistAmp2Ch->Reset();
  mHistAmp2Adc->Reset();
  mHistOrbit2BC->Reset();
  mHistEventDensity2Ch->Reset();
  mHistChDataBits->Reset();
  mHistTriggers->Reset();
  mHistNchA->Reset();
  mHistNchC->Reset();
  mHistSumAmpA->Reset();
  mHistSumAmpC->Reset();
  mHistAverageTimeA->Reset();
  mHistAverageTimeC->Reset();
  mHistChannelID->Reset();
  mHistAdcID->Reset();
  for (auto& entry : mMapHistAmp1D) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistAdc0Amp1D) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistAdc1Amp1D) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistTime1D) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistPMbits) {
    entry.second->Reset();
  }

  for (auto& entry : mMapHistAmpVsTime) {
    entry.second->Reset();
  }
}

} // namespace o2::quality_control_modules::ft0
