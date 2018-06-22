#include "hadaq/SpillProcessor.h"

#include "hadaq/definess.h"
#include "hadaq/TrbIterator.h"
#include "hadaq/TdcIterator.h"


const unsigned NUMHISTBINS = 0x1000;
const double EPOCHLEN = 5e-9*0x800;
const unsigned FASTEPOCHS = 2;
const double BINWIDTHFAST = EPOCHLEN*FASTEPOCHS;
const unsigned SLOWEPOCHS = 0x1000;
const double BINWIDTHSLOW = EPOCHLEN*SLOWEPOCHS;
const unsigned NUMSTAT = 100; // use 100 bins for stat calculations
const double BINWIDTHSTAT = BINWIDTHFAST * NUMSTAT;
const unsigned NUMSTATBINS = 8000; // use 100 bins for stat calculations

hadaq::SpillProcessor::SpillProcessor() :
   base::StreamProc("HLD", 0, false)
{
   mgr()->RegisterProc(this, base::proc_TRBEvent, 0);

   fEvType = MakeH1("EvType", "Event type", 16, 0, 16, "id");
   fEvSize = MakeH1("EvSize", "Event size", 500, 0, 50000, "bytes");
   fSubevSize = MakeH1("SubevSize", "Subevent size", 500, 0, 5000, "bytes");

   fSpill = MakeH1("Spill", "Spill structure", NUMSTATBINS, 0., NUMSTATBINS*BINWIDTHSTAT, "sec");
   fLastSpill = MakeH1("Last", "Last spill structure", NUMSTATBINS, 0., NUMSTATBINS*BINWIDTHSTAT, "sec");

   char title[200];
   snprintf(title, sizeof(title), "Fast hits distribution, %5.2f us bins", BINWIDTHFAST*1e6);
   fHitsFast = MakeH1("HitsFast", title, NUMHISTBINS, 0., BINWIDTHFAST*NUMHISTBINS*1e3, "ms");
   snprintf(title, sizeof(title), "Slow hits distribution, %5.2f ms bins", BINWIDTHSLOW*1e3);
   fHitsSlow = MakeH1("HitsSlow", title, NUMHISTBINS, 0., BINWIDTHSLOW*NUMHISTBINS, "sec");

   fLastBinFast = 0;
   fLastBinSlow = 0;
   fLastEpoch = 0;
   fFirstEpoch = true;

   fTdcMin = 0xc000;
   fTdcMax = 0xc010;

   fSpillOnLevel = 5000;
   fSpillOffLevel = 500;
   fSpillMinCnt = 3;
   fSpillStartEpoch = 0;

   fLastSpillEpoch = 0;
   fLastSpillBin = 0;

   fMaxSpillLength = 10.;
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
      return (rightbin - leftbin) < NUMHISTBINS/2 ? -1 : 1;

   return (leftbin - rightbin) < NUMHISTBINS/2 ? 1 : -1;
}

/** Return time difference between epochs in seconds */
double hadaq::SpillProcessor::EpochTmDiff(unsigned ep1, unsigned ep2)
{
   return EpochDiff(ep1, ep2) * EPOCHLEN;
}

void hadaq::SpillProcessor::StartSpill(unsigned epoch)
{
   fSpillStartEpoch = epoch;
   fSpillEndEpoch = 0;
   fLastSpillEpoch = epoch & ~(FASTEPOCHS-1); // mask not used bins
   fLastSpillBin = 0;
   ClearH1(fSpill);
   printf("SPILL ON  0x%08x tm  %6.2f s\n", epoch, EpochTmDiff(0, epoch));
}

void hadaq::SpillProcessor::StopSpill(unsigned epoch)
{
   printf("SPILL OFF 0x%08x len %6.2f s\n", epoch, EpochTmDiff(fSpillStartEpoch, epoch));

   fSpillStartEpoch = 0;
   fSpillEndEpoch = epoch;
   fLastSpillEpoch = 0;
   fLastSpillBin = 0;
   CopyH1(fLastSpill, fSpill);
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

            unsigned slowbin = 0, fastbin = 0; // current bin number in histogram

            if (istdc) {

               TdcIterator iter;
               iter.assign(sub, ix, datalen);

               hadaq::TdcMessage &msg = iter.msg();

               while (iter.next()) {
                  if (msg.isHitMsg()) {
                     unsigned chid = msg.getHitChannel();
                     //unsigned fine = msg.getHitTmFine();
                     //unsigned coarse = msg.getHitTmCoarse();
                     //bool isrising = msg.isHitRisingEdge();

                     numhits++;

                     if (chid!=0) {
                        FastFillH1(fHitsFast, fastbin);
                        FastFillH1(fHitsSlow, slowbin);
                     }

                  } else if (msg.isEpochMsg()) {
                     fLastEpoch = iter.getCurEpoch();

                     fastbin = (fLastEpoch >> 1) % NUMHISTBINS; // use lower bits from epoch
                     slowbin = (fLastEpoch >> 12) % NUMHISTBINS; // use only 12 bits, skipping lower 12 bits

                     if (fFirstEpoch) {
                        fFirstEpoch = false;
                        fLastBinFast = fastbin;
                        fLastBinSlow = slowbin;
                     } else {
                        // clear all previous bins in-between
                        while (CompareEpochBins(fLastBinSlow, slowbin) < 0) {
                           fLastBinSlow = (fLastBinSlow+1) % NUMHISTBINS;
                           SetH1Content(fHitsSlow, fLastBinSlow, 0.);
                        }
                        while (CompareEpochBins(fLastBinFast, fastbin) < 0) {
                           fLastBinFast = (fLastBinFast+1) % NUMHISTBINS;
                           SetH1Content(fHitsFast, fLastBinFast, 0.);
                        }
                     }
                  }
               }
            }

            ix+=datalen;
         } // while (ix < trbSubEvSize)

      } // subevents


      // try to detect start or stop of the spill

      if (!fSpillStartEpoch && (fLastBinSlow>=fSpillMinCnt)) {
         // detecting spill ON
         bool all_over = true;

         // keep 1 sec for spill OFF signal
         if (fSpillEndEpoch && EpochTmDiff(fSpillEndEpoch, fLastEpoch)<1.) all_over = false;

         for (unsigned bin=(fLastBinSlow-fSpillMinCnt); (bin<fLastBinSlow) && all_over; ++bin)
            if (GetH1Content(fHitsSlow, bin) < fSpillOnLevel) all_over = false;

         if (all_over) StartSpill(fLastEpoch);

      } else if (!fSpillStartEpoch && (fLastBinSlow>=fSpillMinCnt)) {
         // detecting spill OFF
         bool all_below = true;

         for (unsigned bin=(fLastBinSlow-fSpillMinCnt); (bin<fLastBinSlow) && all_below; ++bin)
            if (GetH1Content(fHitsSlow, bin) > fSpillOffLevel) all_below = false;

         if (all_below) StopSpill(fLastEpoch);
      }

      // check length of the current spill
      if (fSpillStartEpoch && (EpochTmDiff(fSpillStartEpoch, fLastEpoch) > fMaxSpillLength))
         StopSpill(fLastEpoch);

      if (fLastSpillEpoch) {

         // check when last bin in spill statistic was caclualted
         unsigned diff = EpochDiff(fLastSpillEpoch, fLastEpoch);
         if (diff > 0.8*FASTEPOCHS*NUMHISTBINS) {
            // too large different jump in epoch value - skip most of them

            unsigned jump = diff / FASTEPOCHS*NUMSTAT;
            if (jump>1) jump--;

            fLastSpillBin += jump;
            fLastSpillEpoch += jump*FASTEPOCHS*NUMSTAT;

            diff = EpochDiff(fLastSpillEpoch, fLastEpoch);
         }


         // calculate statistic for all following bins
         while ((diff > 1.5*FASTEPOCHS*NUMSTAT) && (fLastSpillBin < NUMSTATBINS)) {

            // first bin for statistic
            unsigned fastbin = (fLastEpoch >> 1) % NUMHISTBINS;

            double sum = 0, max = 0;

            for (unsigned n=0;n<NUMSTAT;++n) {
               double cont = GetH1Content(fHitsFast, fastbin++);
               sum+=cont;
               if (cont>max) max = cont;
               if (fastbin >= NUMHISTBINS) fastbin = 0;
            }

            sum = sum/NUMSTAT;

            SetH1Content(fSpill, fLastSpillBin, sum>0 ? max/sum : 0.);

            fLastSpillBin++;
            fLastSpillEpoch += FASTEPOCHS*NUMSTAT;

            diff = EpochDiff(fLastSpillEpoch, fLastEpoch);
         }


         if (fLastSpillBin >= NUMSTATBINS) StopSpill(fLastEpoch);

      }

   } // events
   return true;

}
