#include "hadaq/SpillProcessor.h"

#include "hadaq/definess.h"
#include "hadaq/TrbIterator.h"
#include "hadaq/TdcIterator.h"


#define NUMEPOCHBINS 0x1000

hadaq::SpillProcessor::SpillProcessor() :
   base::StreamProc("HLD", 0, false)
{
   mgr()->RegisterProc(this, base::proc_TRBEvent, 0);

   fEvType = MakeH1("EvType", "Event type", 16, 0, 16, "id");
   fEvSize = MakeH1("EvSize", "Event size", 500, 0, 50000, "bytes");
   fSubevSize = MakeH1("SubevSize", "Subevent size", 500, 0, 5000, "bytes");

   fSpillSize = 1000;
   fSpill = MakeH1("Spill", "Spill structure", fSpillSize, 0., 10., "sec");
   fLastSpill = MakeH1("Last", "Last spill structure", fSpillSize, 0., 10., "sec");

   double binwidth = 5e-9*0x800*0x1000;

   fHitsData = MakeH1("Hits", "Number of hits", 0x1000, 0., binwidth*NUMEPOCHBINS, "sec");
   fSpillCnt = 0;
   fTotalCnt = 0;
   fLastEpBin = 0;
   fFirstEpoch = true;

   fTdcMin = 0xc000;
   fTdcMax = 0xc010;
}

hadaq::SpillProcessor::~SpillProcessor()
{
}

/** returns -1 when leftbin<rightbin, taking into account overflow around 0x1000)
 *          +1 when leftbin>rightbin
 *          0  when leftbin==rightbin */
int hadaq::SpillProcessor::CompareEpochBins(unsigned leftbin, unsigned rightbin)
{
   if (leftbin == rightbin) return 0;

   if (leftbin < rightbin)
      return (rightbin - leftbin) < NUMEPOCHBINS/2 ? -1 : 1;

   return (leftbin - rightbin) < NUMEPOCHBINS/2 ? 1 : -1;
}


bool hadaq::SpillProcessor::FirstBufferScan(const base::Buffer& buf)
{
   if (buf.null()) return false;

   hadaq::TrbIterator iter(buf().buf, buf().datalen);

   hadaqs::RawEvent* ev = 0;

   unsigned evcnt = 0;

   while ((ev = iter.nextEvent()) != 0) {

      evcnt++;

      DefFillH1(fEvType, (ev->GetId() & 0xf), 1.);
      DefFillH1(fEvSize, ev->GetPaddedSize(), 1.);

      // fMsg.trig_type = ev->GetId() & 0xf;
      // fMsg.seq_nr = ev->GetSeqNr();
      // fMsg.run_nr = ev->GetRunNr();

      // if ((fEventTypeSelect <= 0xf) && ((ev->GetId() & 0xf) != fEventTypeSelect)) continue;

      // if (IsPrintRawData()) ev->Dump();

      unsigned hubid = 0x7654;

      int numhits = 0;

      hadaqs::RawSubevent* sub = 0;

      while ((sub = iter.nextSubevent()) != 0) {
         DefFillH1(fSubevSize, sub->GetSize(), 1.);

         unsigned ix = 0;           // cursor
         unsigned trbSubEvSize = sub->GetSize() / 4 - 4;

         while (ix < trbSubEvSize) {
            //! Extract data portion from the whole packet (in a loop)
            uint32_t data = sub->Data(ix++);

            unsigned datalen = (data >> 16) & 0xFFFF;
            unsigned dataid = data & 0xFFFF;

            // ignore HUB HEADER
            if (dataid == hubid) continue;

            bool istdc = (dataid>=fTdcMin) && (dataid<fTdcMax);

            unsigned epbin = 0; // current bin number in histogram

            if (istdc) {

               TdcIterator iter;
               iter.assign(sub, ix, datalen);

               hadaq::TdcMessage &msg = iter.msg();

               while (iter.next()) {
                  if (msg.isHitMsg()) {
                     //unsigned chid = msg.getHitChannel();
                     //unsigned fine = msg.getHitTmFine();
                     //unsigned coarse = msg.getHitTmCoarse();
                     //bool isrising = msg.isHitRisingEdge();

                     numhits++;

                     FastFillH1(fHitsData, epbin);

                  } else if (msg.isEpochMsg()) {
                     unsigned ep = iter.getCurEpoch();

                     epbin = (ep >> 12) % NUMEPOCHBINS; // use only 12 bits, skipping lower 12 bits

                     // clear all previous bins in-between

                     if (fFirstEpoch) {
                        fFirstEpoch = false;
                        fLastEpBin = epbin;
                     } else {
                        while (CompareEpochBins(fLastEpBin, epbin) < 0) {
                           fLastEpBin = (fLastEpBin+1) % NUMEPOCHBINS;
                           SetH1Content(fHitsData, fLastEpBin, 0.);
                        }
                     }

                  }
               }
            }

            ix+=datalen;
         } // while (ix < trbSubEvSize)

      } // subevents


      // after all subevents are scanned, one could set content of spill extraction
      // now is just number of all hit events in all TDC

      if (!numhits) numhits = fTotalCnt % 77;

      SetH1Content(fSpill, fSpillCnt, numhits);
      fSpillCnt++;
      fTotalCnt++;

      if (fSpillCnt>=fSpillSize) {
         fSpillCnt = 0;
         CopyH1(fLastSpill, fSpill);
         ClearH1(fSpill);
      }

   } // events
   return true;

}
