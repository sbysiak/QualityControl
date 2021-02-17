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
/// \file   HistDiffReductor.cxx
/// \author Piotr Konopka
///

#include <TH1.h>
#include "QualityControl/QcInfoLogger.h"
#include "FT0/HistDiffReductor.h"

namespace o2::quality_control_modules::ft0
{

void* HistDiffReductor::getBranchAddress()
{
  return &mStats;
}

const char* HistDiffReductor::getBranchLeafList()
{
  return "mean/D:stddev:entries";
}

void HistDiffReductor::update(TObject* obj)
{ 

  ILOG(Info) << "\n\n-------------------------\n-------------------------\n--------------------------\n\t\tUPDATE():\n\n\n";
  // todo: use GetStats() instead?
  auto histoCur  = dynamic_cast<TH1*>(obj);
  auto histoDiff = dynamic_cast<TH1*>(obj);
  if (!(histoPrev)) {
        ILOG(Info) << "\n\n\nempty histoPrev !!!\n\n\n\n";
	histoPrev = dynamic_cast<TH1*>(obj);
	histoPrev->Reset();
  }
  ILOG(Info) << "Nbins prev, cur, diff: " << std::to_string(int(histoPrev->GetNbinsX())) << " , " <<  std::to_string(int(histoCur->GetNbinsX())) << " , " << std::to_string(int(histoDiff->GetNbinsX())) << "\n";
  ILOG(Info) << "entries prev, cur, diff: " << std::to_string(int(histoPrev->GetEntries())) << " , " <<  std::to_string(int(histoCur->GetEntries())) << " , " << std::to_string(int(histoDiff->GetEntries())) << "\n";
  
  histoDiff->Add(histoPrev, -1);
  
  ILOG(Info) << "Nbins prev, cur, diff: " << std::to_string(int(histoPrev->GetNbinsX())) << " , " <<  std::to_string(int(histoCur->GetNbinsX())) << " , " << std::to_string(int(histoDiff->GetNbinsX())) << "\n";
  ILOG(Info) << "entries prev, cur, diff: " << std::to_string(int(histoPrev->GetEntries())) << " , " <<  std::to_string(int(histoCur->GetEntries())) << " , " << std::to_string(int(histoDiff->GetEntries())) << "\n";
  if (histoDiff) {
    mStats.entries = histoDiff->GetEntries();
    mStats.stddev = histoDiff->GetStdDev();
    mStats.mean = histoDiff->GetMean();
  }
  histoPrev = histoCur;
  
  ILOG(Info) << "Nbins prev, cur, diff: " << std::to_string(int(histoPrev->GetNbinsX())) << " , " <<  std::to_string(int(histoCur->GetNbinsX())) << " , " << std::to_string(int(histoDiff->GetNbinsX())) << "\n";
  ILOG(Info) << "entries prev, cur, diff: " << std::to_string(int(histoPrev->GetEntries())) << " , " <<  std::to_string(int(histoCur->GetEntries())) << " , " << std::to_string(int(histoDiff->GetEntries())) << "\n";
}

} // namespace o2::quality_control_modules::ft0
