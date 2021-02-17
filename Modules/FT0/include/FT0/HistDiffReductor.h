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
/// \file   HistDiffReductor.h
/// \author Piotr Konopka
///
#ifndef QUALITYCONTROL_HISTDIFFREDUCTOR_H
#define QUALITYCONTROL_HISTDIFFREDUCTOR_H

#include <TH1.h>
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/Reductor.h"

namespace o2::quality_control_modules::ft0
{

/// \brief A Reductor which obtains the most popular characteristics of TH1.
///
/// A Reductor which obtains the most popular characteristics of TH1.
/// It produces a branch in the format: "mean/D:stddev:entries"
class HistDiffReductor : public quality_control::postprocessing::Reductor
{
 public:
  HistDiffReductor(){
	ILOG(Info) << "\n\n\n\nCONSTRUCTOR\n\n\n";
  };
  ~HistDiffReductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;
  

 private:
  struct {
    Double_t mean;
    Double_t stddev;
    Double_t entries;
  } mStats;

  TH1* histoPrev=0; 
};

} // namespace o2::quality_control_modules::ft0

#endif //QUALITYCONTROL_HISTDIFFREDUCTOR_H
