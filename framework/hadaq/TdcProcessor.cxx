#include "hadaq/TdcProcessor.h"

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "hadaq/TrbProcessor.h"

hadaq::TdcProcessor::TdcProcessor(TrbProcessor* trb, unsigned tdcid) :
   base::StreamProc("TDC", tdcid),
   fTdcId(tdcid)
{
  fChannels = MakeH1("Channels", "TDC channels", NumTdcChannels, 0, NumTdcChannels, "ch");

  if (trb) trb->AddSub(this, tdcid);
}

hadaq::TdcProcessor::~TdcProcessor()
{
}

void hadaq::TdcProcessor::FirstSubScan(hadaq::RawSubevent* sub, unsigned indx, unsigned datalen)
{
   bool isEpoch(false);
   unsigned curEpoch(0);

   while (datalen-- > 0) {
      uint32_t data = sub->Data(indx++);

      uint32_t tmpBits = ((data >> 29) & 0x7);

      switch (tmpBits) {
         case 0x4: { // HIT MESSAGE
            unsigned res = ((data >> 29) & 0x3);
            unsigned curChan = ((data >> 22) & 0x7F);
            unsigned tcoarse = (data & 0x7FF);
            unsigned tfine = ((data >> 12) & 0x3FF);
            bool isrising = (((data >> 11) & 0x1) == 0x1);

//            EPRINT("***  --- tdc data: 0x%08x  time: %12.2f  isrising: %d  chan: 0x%x  tc: 0x%03x  tf: 0x%03x  reserved: 0x%x\n",
//                  data, curEpoch*5.*2048 + tcoarse*5. + tfine*0.011, isrising, curChan,  tcoarse, tfine , res );

            if (!isEpoch) {
               printf("*** LOST EPOCH - ignore next data ***\n");
            } else
            if (tcoarse > 0x7ff) {
               printf("*** SUSPECTED coarse counter %x - IGNORE ***\n", tcoarse);
               isEpoch = false;
            } else
            if (curChan >= NumTdcChannels) {
               printf("TDC Channel number problem %u\n", curChan);
               isEpoch = false;
            } else {

               isEpoch = false;


               FillH1(fChannels, curChan);

               /** ------- The most important lines - forming 'hit' and putting it into the vector ------- */
               /* TTrbRawHit theRawHit (curTDC, curChan, isrising, curEpoch, tcoarse, tfine);
                  fRawHits[curTRB].push_back (theRawHit);
                  if (isrising)
                     hTriggerCount[curTRB][curTDC]->Fill(2);      //! LEADING
                  else
                     hTriggerCount[curTRB][curTDC]->Fill(3);      //! TRAILING
                  hTriggerCount[curTRB][curTDC]->Fill(0);         //! TOTAL
                  hTrbTriggerCount->Fill(2);          //! TRB - TDC
                  hTrbTriggerCount->Fill(0);          //! TRB TOTAL
                */
            }
            break;
         }

         case 0x1: {  //! Data header processing
            uint32_t err = (data & 0xFFFF);
            uint32_t res = ((data >> 16) & 0x1FFF);
            //EPRINT ("***  --- tdc head: 0x%08x, reserved=0x%x, errorbits=0x%x\n", data, res, err);
            // hTriggerCount[curTRB][curTDC]->Fill(5);      //! HEADER
            // hTriggerCount[curTRB][curTDC]->Fill(0);      //! TOTAL
            // hTrbTriggerCount->Fill(2);       //! TRB - TDC
            // hTrbTriggerCount->Fill(0);       //! TRB TOTAL
            if (err) {
               printf("!!! found error bits: 0x%x at tdc 0x%x\n", err, fTdcId);
               // hTriggerCount[curTRB][curTDC]->Fill(1);      //! INVALID
            }
            break;
         }

         case 0x3: { //! Epoch marker processing
            uint32_t res = ((data >> 28) & 0x1);
            isEpoch = true;
            curEpoch = (data & 0xFFFFFFF);
            // EPRINT("***  --- tdc epch: 0x%08x  time: %12.2f  reserved=0x%x, epoch=0x%x\n", data, curEpoch*5.*2048, res, curEpoch);

            // hEpoch[curTRB][curTDC]->Fill(curEpoch);

            //hTriggerCount[curTRB][curTDC]->Fill(4);      //! EPOCH
            //hTriggerCount[curTRB][curTDC]->Fill(0);      //! TOTAL
            //hTrbTriggerCount->Fill(2);       //! TRB - TDC
            //hTrbTriggerCount->Fill(0);       //! TRB TOTAL
            break;
         }

         case 0x2: { //! Debug processing
            uint32_t bits = (data & 0xFFFFFF);
            uint32_t mode = ((data >> 24) & 0x1F);
            // EPRINT ("***  --- debug word: 0x%08x, mode=0x%x, bits=0x%x\n", data, mode, bits);
            // hTriggerCount[curTRB][curTDC]->Fill(6);      //! DEBUG
            // hTriggerCount[curTRB][curTDC]->Fill(0);      //! TOTAL
            // hTrbTriggerCount->Fill(2);       //! TRB - TDC
            // hTrbTriggerCount->Fill(0);       //! TRB TOTAL
            break;
         }

         case 0x0: { //! Reserved processing
            uint32_t res = (data & 0x1FFFFFFF);
            // EPRINT ("***  --- reserved word: 0x%08x, reserved=0x%x\n", data, res);
            // hTriggerCount[curTRB][curTDC]->Fill(7);      //! RESERVED
            // hTriggerCount[curTRB][curTDC]->Fill(0);      //! TOTAL
            // hTrbTriggerCount->Fill(2);       //! TRB - TDC
            // hTrbTriggerCount->Fill(0);       //! TRB TOTAL
            break;
         }

         default:
            printf("Unknows bits 0x%x in header\n", tmpBits);
            break;
      }
   }  // end of loop inside the TDC data
}