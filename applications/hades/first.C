// this is example for
#include "base/ProcMgr.h"
#include "hadaq/TdcMessage.h"
#include "hadaq/TrbProcessor.h"
#include "hadaq/HldProcessor.h"


void first()
{
   base::ProcMgr::instance()->SetRawAnalysis(true);
   // base::ProcMgr::instance()->SetTriggeredAnalysis(true);

   // all new instances get this value
   base::ProcMgr::instance()->SetHistFilling(2);

   // this limits used for liner calibrations when nothing else is available
   hadaq::TdcMessage::SetFineLimits(31, 491);

   // Enable 1 or disable 0 errors logging from following enumeration
   //  { errNoHeader, errChId, errEpoch, errFine, err3ff, errCh0, errMismatchDouble, errUncknHdr, errDesignId, errMisc }
   hadaq::TdcProcessor::SetErrorMask(0xffffff);

   // default channel numbers and edges mask
   // 1 - use only rising edge, falling edge is ignore
   // 2   - falling edge enabled and fully independent from rising edge
   // 3   - falling edge enabled and uses calibration from rising edge
   // 4   - falling edge enabled and common statistic is used for calibration
   hadaq::TrbProcessor::SetDefaults(49, 2);

   // [min..max] range for TDC ids
   hadaq::TrbProcessor::SetTDCRange(0x5000, 0x7FFF);

   // [min..max] range for HUB ids
   hadaq::TrbProcessor::SetHUBRange(0x100, 0x100);

   // when first argument true - TRB/TDC will be created on-the-fly
   // second parameter is function name, called after elements are created
   hadaq::HldProcessor* hld = new hadaq::HldProcessor(true, "after_create");

   // first parameter if filename  prefix for calibration files
   //     and calibration mode (empty string - no file I/O)
   // second parameter is hits count for autocalibration
   //     0 - only load calibration
   //    -1 - accumulate data and store calibrations only at the end
   //    >0 - automatic calibration after N hits in each active channel
   //    >1000000000 - automatic calibration after N hits only once
   // third parameter is trigger type mask used for calibration
   //   (1 << 0xD) - special 0XD trigger with internal pulser, used also for TOT calibration
   //    0x3FFF - all kinds of trigger types will be used for calibration (excluding 0xE and 0xF)
   //   0x80000000 in mask enables usage of temperature correction

   //   hld->ConfigureCalibration("", 100000, (1 << 0xD));
   hld->ConfigureCalibration("", 0, 0);
   // only accept trigger type 0x1 when storing file
   // new hadaq::HldFilter(0x1);

   // create ROOT file store
   // base::ProcMgr::instance()->CreateStore("td.root");

   // 0 - disable store
   // 1 - std::vector<hadaq::TdcMessageExt> - includes original TDC message
   // 2 - std::vector<hadaq::MessageFloat>  - compact form, without channel 0, stamp as float (relative to ch0)
   // 3 - std::vector<hadaq::MessageDouble> - compact form, with channel 0, absolute time stamp as double
   base::ProcMgr::instance()->SetStoreKind(0);

   // when configured as output in DABC, one specifies:
   // <OutputPort name="Output2" url="stream://file.root?maxsize=5000&kind=3"/>


}

// extern "C" required by DABC to find function from compiled code

extern "C" void after_create(hadaq::HldProcessor* hld)
{
  // return;
   printf("Called after all sub-components are created\n");

   if (hld==0) return;

   for (unsigned k=0;k<hld->NumberOfTRB();k++) {
      hadaq::TrbProcessor* trb = hld->GetTRB(k);
      if (trb==0) continue;
      //printf("Configure %s!\n", trb->GetName());
      trb->SetPrintErrors(10);
      // trb->SetPrintErrors(1000000000L);
   }

   for (unsigned k=0;k<hld->NumberOfTDC();k++) {
      hadaq::TdcProcessor* tdc = hld->GetTDC(k);
      if (tdc==0) continue;

      //printf("Configure %s!\n", tdc->GetName());

      // tdc->SetUseLastHit(true);

      //for (unsigned nch=2;nch<tdc->NumChannels();nch++)
      //   tdc->SetRefChannel(nch, 1, 0xffff, 2000,  -10., 10.);
   }
}

