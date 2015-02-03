#include "hadaq/TrbProcessor.h"

#include <string.h>

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "hadaq/TrbIterator.h"
#include "hadaq/TdcProcessor.h"
#include "hadaq/HldProcessor.h"

#define RAWPRINT( args ...) if(IsPrintRawData()) printf( args )

unsigned hadaq::TrbProcessor::gNumChannels = 65;
unsigned hadaq::TrbProcessor::gEdgesMask = 0x1;

void hadaq::TrbProcessor::SetDefaults(unsigned numch, unsigned edges)
{
   gNumChannels = numch;
   gEdgesMask = edges;
}


hadaq::TrbProcessor::TrbProcessor(unsigned brdid, HldProcessor* hldproc) :
   base::StreamProc("TRB_%04X", brdid, false),
   fHldProc(hldproc),
   fMap()
{
   if (hldproc==0)
      mgr()->RegisterProc(this, base::proc_TRBEvent, brdid & 0xFF);
   else
      hldproc->AddTrb(this, brdid);

   // printf("Create TrbProcessor %s\n", GetName());

   fMsgPerBrd = 0;
   fErrPerBrd = 0;
   fHitsPerBrd = 0;

   fEvSize = MakeH1("EvSize", "Event size", 500, 0, 5000, "bytes");
   fSubevSize = MakeH1("SubevSize", "Subevent size", 500, 0, 5000, "bytes");
   fLostRate = MakeH1("LostRate", "Relative number of lost packets", 1000, 0, 1., "data lost");

   fHadaqCTSId = 0x8000;
   fHadaqHUBId = 0x9000;  // high 8 bit important,

   fLastTriggerId = 0;
   fLostTriggerCnt = 0;
   fTakenTriggerCnt = 0;

   fSyncTrigMask = 0;
   fSyncTrigValue = 0;

   fUseTriggerAsSync = false;
   fCompensateEpochReset = false;

   fPrintRawData = false;
   fCrossProcess = false;
   fPrintErrCnt = 1000;

   // this is raw-scan processor, therefore no synchronization is required for it
   SetSynchronisationKind(sync_None);

   // only raw scan, data can be immediately removed
   SetRawScanOnly();
}

hadaq::TrbProcessor::~TrbProcessor()
{
}

hadaq::TdcProcessor* hadaq::TrbProcessor::FindTDC(unsigned tdcid) const
{
   if (fHldProc!=0) return fHldProc->FindTDC(tdcid);
   return GetTDC(tdcid, true);
}

void hadaq::TrbProcessor::UserPreLoop()
{
   unsigned numtdc = fMap.size();
   if (numtdc<4) numtdc = 4;
   std::string lbl = "tdc;xbin:";
   unsigned cnt = 0;
   for (SubProcMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
      if (++cnt>0) lbl.append(",");
      char sbuf[50];
      sprintf(sbuf, "%04X", iter->second->GetID());
      lbl.append(sbuf);
   }

   while (cnt++<numtdc)
      lbl.append(",----");

   fMsgPerBrd = MakeH1("MsgPerTDC", "Number of messages per TDC", numtdc, 0, numtdc, lbl.c_str());
   fErrPerBrd = MakeH1("ErrPerTDC", "Number of errors per TDC", numtdc, 0, numtdc, lbl.c_str());
   fHitsPerBrd = MakeH1("HitsPerTDC", "Number of data hits per TDC", numtdc, 0, numtdc, lbl.c_str());
}


bool hadaq::TrbProcessor::CheckPrintError()
{
   if (fPrintErrCnt<=0) return false;

   if (--fPrintErrCnt==0) {
      printf("%5s Suppress all other errors ...\n", GetName());
      return false;
   }

   return true;
}

void hadaq::TrbProcessor::CreateTDC(unsigned id1, unsigned id2, unsigned id3, unsigned id4)
{
   // overwrite default value in the beginning

   for (unsigned cnt=0;cnt<4;cnt++) {
      unsigned tdcid = id1;
      switch (cnt) {
         case 1: tdcid = id2; break;
         case 2: tdcid = id3; break;
         case 3: tdcid = id4; break;
         default: tdcid = id1; break;
      }
      if (tdcid==0) continue;

      if (GetTDC(tdcid, true)!=0) {
         printf("TDC id 0x%04x already exists\n", tdcid);
         continue;
      }

      if (tdcid==fHadaqCTSId) {
         printf("TDC id 0x%04x is used as CTS id\n", tdcid);
         continue;
      }

      if (tdcid==fHadaqHUBId) {
         printf("TDC id 0x%04x is used as HUB id\n", tdcid);
         continue;
      }

      new hadaq::TdcProcessor(this, tdcid, gNumChannels, gEdgesMask);
   }
}


void hadaq::TrbProcessor::SetAutoCalibrations(long cnt)
{
   for (SubProcMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
      if (iter->second->IsTDC())
        ((TdcProcessor*)iter->second)->SetAutoCalibration(cnt);
   }
}


void hadaq::TrbProcessor::DisableCalibrationFor(unsigned firstch, unsigned lastch)
{
   for (SubProcMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
      if (iter->second->IsTDC())
         ((TdcProcessor*)iter->second)->DisableCalibrationFor(firstch, lastch);
   }
}


void hadaq::TrbProcessor::SetWriteCalibrations(const char* fileprefix, bool every_time)
{
   char fname[1024];

   for (SubProcMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
      if (!iter->second->IsTDC()) continue;
      TdcProcessor* tdc = (TdcProcessor*) iter->second;

      snprintf(fname, sizeof(fname), "%s%04x.cal", fileprefix, tdc->GetID());

      tdc->SetWriteCalibration(fname, every_time);
   }
}


bool hadaq::TrbProcessor::LoadCalibrations(const char* fileprefix)
{
   char fname[1024];

   bool res = true;

   for (SubProcMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
      if (!iter->second->IsTDC()) continue;
      TdcProcessor* tdc = (TdcProcessor*) iter->second;

      snprintf(fname, sizeof(fname), "%s%04x.cal", fileprefix, tdc->GetID());

      if (!tdc->LoadCalibration(fname)) res = false;
   }

   return res;
}

void hadaq::TrbProcessor::AddSub(SubProcessor* tdc, unsigned id)
{
   fMap[id] = tdc;
}

void hadaq::TrbProcessor::SetStoreEnabled(bool on)
{
   base::StreamProc::SetStoreEnabled(on);

   for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
      iter->second->SetStoreEnabled(on);
}

void hadaq::TrbProcessor::SetCh0Enabled(bool on)
{
   for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++) {
      if (iter->second->IsTDC())
         ((TdcProcessor*)iter->second)->SetCh0Enabled(on);
   }
}

void hadaq::TrbProcessor::SetTriggerWindow(double left, double right)
{
   base::StreamProc::SetTriggerWindow(left, right);

   for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++) {
      iter->second->SetTriggerWindow(left, right);
      iter->second->CreateTriggerHist(16, 3000, -2e-6, 2e-6);
   }
}


void hadaq::TrbProcessor::AccountTriggerId(unsigned subid)
{
   if (fLastTriggerId==0) {
      fLostTriggerCnt = 0;
      fTakenTriggerCnt = 0;
   } else {
      unsigned diff = (subid - fLastTriggerId) & 0xffff;
      if ((diff > 0x8000) || (diff==0)) {
         // ignore this, most probable it is some mistake
      } else
      if (diff==1) {
         fTakenTriggerCnt++;
      } else {
         fLostTriggerCnt+=(diff-1);
      }
   }

   fLastTriggerId = subid;

   if (fTakenTriggerCnt + fLostTriggerCnt > 1000) {
      double lostratio = 1.*fLostTriggerCnt / (fTakenTriggerCnt + fLostTriggerCnt + 0.);
      FillH1(fLostRate, lostratio);
      fTakenTriggerCnt = 0;
      fLostTriggerCnt = 0;
   }
}


bool hadaq::TrbProcessor::FirstBufferScan(const base::Buffer& buf)
{
   if (buf.null()) return false;

//   RAWPRINT("TRB3 - first scan of buffer %u\n", buf().datalen);

//   printf("Scan TRB buffer size %u\n", buf().datalen);

   hadaq::TrbIterator iter(buf().buf, buf().datalen);

   hadaqs::RawEvent* ev = 0;

   while ((ev = iter.nextEvent()) != 0) {

      if (ev->GetSize() > buf().datalen+4) {
         printf("Corrupted event size %u!\n", ev->GetSize()); return true;
      }

      if (IsPrintRawData()) ev->Dump();

      FillH1(fEvSize, ev->GetSize());

      hadaqs::RawSubevent* sub = 0;
      unsigned subcnt(0);

      while ((sub = iter.nextSubevent()) != 0) {

         if (sub->GetSize() > buf().datalen+4) {
            printf("Corrupted subevent size %u!\n", sub->GetSize()); return true;
         }

         if (IsPrintRawData()) sub->Dump(true);

         FillH1(fSubevSize, sub->GetSize());

         // use only 16-bit in trigger number while CTS make a lot of errors in higher 8 bits
         AccountTriggerId((sub->GetTrigNr() >> 8) & 0xffff);

         ScanSubEvent(sub, ev->GetSeqNr());

         subcnt++;
      }

      AfterEventScan();
   }

   return true;
}

void hadaq::TrbProcessor::AfterEventScan()
{
   // we scan in all TDC processors newly add buffers
   // this is required to
   if (IsCrossProcess()) {
      for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
         iter->second->ScanNewBuffers();

      for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
         iter->second->AfterFill(&fMap);
   }
}

void hadaq::TrbProcessor::ScanSubEvent(hadaqs::RawSubevent* sub, unsigned trb3eventid)
{
   // this is first scan of subevent from TRB3 data
   // our task is statistic over all messages we will found
   // also for trigger-type 1 we should add SYNC message to each processor

   for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
      iter->second->SetNewDataFlag(false);

   unsigned ix = 0;           // cursor

   unsigned trbSubEvSize = sub->GetSize() / 4 - 4;

   if (trbSubEvSize>100000) printf("LARGE subevent %u\n", trbSubEvSize);

   unsigned syncNumber(0);
   bool findSync(false);

//   RAWPRINT("Scan TRB3 raw event 4-bytes size %u\n", trbSubEvSize);
//   printf("Scan TRB3 raw data\n");

   while (ix < trbSubEvSize) {
      //! Extract data portion from the whole packet (in a loop)
      uint32_t data = sub->Data(ix++);

      unsigned datalen = (data >> 16) & 0xFFFF;

//      RAWPRINT("Subevent id 0x%04x len %u\n", (data & 0xFFFF), datalen);

      // ===========  this is header for TDC, build inside the TRB3 =================
//      if ((data & 0xFF00) == 0xB000) {
//         // do that ever you want
//         unsigned brdid = data & 0xFF;
//         RAWPRINT("    TRB3-TDC header: 0x%08x, TRB3-buildin TDC-FPGA brd=%u, size=%u  IGNORED\n", (unsigned) data, brdid, datalen);
//         ix+=datalen;
//         continue;
//      }

      if ((data & 0xFF00) == fHadaqHUBId) {
         RAWPRINT ("   HUB header: 0x%08x, hub=%u, size=%u (ignore)\n", (unsigned) data, (unsigned) data & 0xFF, datalen);

         // ix+=datalen;  // WORKAROUND !!!

         // TODO: formally we should analyze HUB subevent as real subevent but
         // we just skip header and continue to analyze data
         continue;
      }

      //! ==================== CTS header and inside ================
      if ((data & 0xFFFF) == fHadaqCTSId) {
         RAWPRINT("   CTS header: 0x%x, size=%d\n", (unsigned) data, datalen);
         //hTrbTriggerCount->Fill(5);          //! TRB - CTS
         //hTrbTriggerCount->Fill(0);          //! TRB TOTAL

         data = sub->Data(ix++); datalen--;
         unsigned trigtype = (data & 0xFFFF);

         unsigned nInputs = (data >> 16) & 0xf;
         unsigned nTrigChannels = (data >> 20) & 0xf;
         unsigned bIncludeLastIdle = (data >> 25) & 0x1;
         unsigned bIncludeCounters = (data >> 26) & 0x1;
         unsigned bIncludeTimestamp = (data >> 27) & 0x1;
         unsigned nExtTrigFlag = (data >> 28) & 0x3;

         unsigned nCTSwords = nInputs*2 + nTrigChannels*2 +
                              bIncludeLastIdle*2 + bIncludeCounters*3 + bIncludeTimestamp*1;

         RAWPRINT("     CTS trigtype: 0x%04x, datalen=%u nCTSwords=%u\n", trigtype, datalen, nCTSwords);

         // we skip all the information from the CTS right now,
         // we don't need it
         ix += nCTSwords;
         datalen -= nCTSwords;
         // ix should now point to the first ETM word


         // handle the ETM module and extract syncNumber
         if(nExtTrigFlag==0x1) {
	         // ETM sends one word, is probably MBS Vulom Recv
	         // this is not really tested
	         data = sub->Data(ix++); datalen--;
	         syncNumber = (data & 0xFFFFFF);
	         findSync = true;
	         // TODO: evaluate the upper 8 bits in data for status/error
         }
         else if(nExtTrigFlag==0x2) {
	         // ETM sends four words, is probably a Mainz A2 recv
	         data = sub->Data(ix++); datalen--;
	         findSync = true;
	         syncNumber = data; // full 32bits is trigger number
	         // TODO: evaluate word 2 for status/error, skip it for now
	         // word 3+4 are 0xdeadbeef i.e. not used at the moment, so skip it
	         ix += 3;
	         datalen -= 3;
         }
         else {
	         RAWPRINT("Error: Unknown ETM in CTS header found: %x\n", nExtTrigFlag);
	         // TODO: try to do some proper error recovery here...
         }

         if(findSync)
	         RAWPRINT("     Find SYNC %u\n", (unsigned) syncNumber);

         // now ix should point to the first TDC word if datalen>0
         // if not, there is no TDC present

         // This is special TDC processor for data from CTS header
         TdcProcessor* subproc = GetTDC(fHadaqCTSId, true);
         unsigned tdc_index = ix;  // position where subevents starts
         unsigned tdc_datalen = datalen;

         if ((subproc!=0) && (tdc_datalen>0)) {
            // if TDC processor found and length is specified, process such data as normal TDC data
            base::Buffer buf;
            buf.makenew((tdc_datalen+1)*4);

            memset(buf.ptr(), 0xff, 4); // fill dummy sync id in the begin
            sub->CopyDataTo(buf.ptr(4), tdc_index, tdc_datalen);

            buf().kind = 0;
            buf().boardid = fHadaqCTSId;
            buf().format = 0;

            subproc->AddNextBuffer(buf);
            subproc->SetNewDataFlag(true);
         }

         // don't forget to skip the words for the TDC (if any)
         ix += datalen;

         continue;
      }

      TdcProcessor* tdcproc = GetTDC(data & 0xFFFF);
      //! ================= FPGA TDC header ========================
      if (tdcproc != 0) {
         unsigned tdcid = data & 0xFFFF;

         RAWPRINT ("   FPGA-TDC header: 0x%08x, tdcid=0x%04x, size=%u\n", (unsigned) data, tdcid, datalen);

         if (IsPrintRawData()) {
            TdcIterator iter;
            iter.assign(sub, ix, datalen);
            while (iter.next()) iter.printmsg();
         }

         base::Buffer buf;
         buf.makenew((datalen+1)*4);

         memset(buf.ptr(), 0xff, 4); // fill dummy sync id in the begin
         sub->CopyDataTo(buf.ptr(4), ix, datalen);

         buf().kind = 0;
         buf().boardid = tdcid;
         buf().format = 0;

         tdcproc->AddNextBuffer(buf);
         tdcproc->SetNewDataFlag(true);

         ix+=datalen;

         continue; // go to next block
      }  // end of if TDC header


      //! ==================  Dummy header and inside ==========================
      if ((data & 0xFFFF) == 0x5555) {
         RAWPRINT("   Dummy header: 0x%x, size=%d\n", (unsigned) data, datalen);
         //hTrbTriggerCount->Fill(4);          //! TRB - DUMMY
         //hTrbTriggerCount->Fill(0);          //! TRB TOTAL
         while (datalen-- > 0) {
            //! In theory here must be only one word - termination package with the status
            data = sub->Data(ix++);
            RAWPRINT("      word: 0x%08x\n", (unsigned) data);
            //uint32_t fSubeventStatus = data;
            //if (fSubeventStatus != 0x00000001) { bad events}
         }

         continue;
      }

      SubProcMap::const_iterator iter = fMap.find(data & 0xFFFF);
      SubProcessor* subproc = iter != fMap.end() ? iter->second : 0;

      //! ================= any other header ========================
      if (subproc!=0) {
         unsigned subid = data & 0xFFFF;

         RAWPRINT ("   SUB header: 0x%08x, id=0x%04x, size=%u\n", (unsigned) data, subid, datalen);

         base::Buffer buf;
         buf.makenew(datalen*4);

         sub->CopyDataTo(buf.ptr(0), ix, datalen);

         buf().kind = 0;
         buf().boardid = subid;
         buf().format = 0;

         subproc->AddNextBuffer(buf);
         subproc->SetNewDataFlag(true);

         ix+=datalen;

         continue; // go to next block
      }  // end of if SUB header



//      if ((data & 0xFFFF) == 0x8001) {
//         // no idea that this, but appeared in beam-time data with different sizes
//         ix+=datalen;
//         continue;
//      }
//
//      if ((data & 0xFFFF) == 0x8002) {
//         // no idea that this, but appeared in beam-time data with size 0
//         if (datalen!=0) printf("Strange length %u, expected 0 with 8002 subtype\n", datalen);
//         ix+=datalen;
//         continue;
//      }
//
      RAWPRINT("Unknown header 0x%04x length %u in TRB 0x%04x subevent\n", data & 0xFFFF, datalen, GetID());
      ix+=datalen;
   }

   if (fUseTriggerAsSync) {
      findSync = true;
      syncNumber = trb3eventid;
   }

   if (findSync) {
      for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++) {
         if (iter->second->IsNewDataFlag())
            iter->second->AppendTrbSync(syncNumber);
      }
   }
}


void hadaq::TrbProcessor::TransformSubEvent(hadaqs::RawSubevent* sub)
{
   unsigned ix = 0;           // cursor

   unsigned trbSubEvSize = sub->GetSize() / 4 - 4;

   while (ix < trbSubEvSize) {
      //! Extract data portion from the whole packet (in a loop)
      uint32_t data = sub->Data(ix++);

      unsigned datalen = (data >> 16) & 0xFFFF;

      if ((data & 0xFF00) == fHadaqHUBId) {
         // ix+=datalen;  // WORKAROUND !!!

         // TODO: formally we should analyze HUB subevent as real subevent but
         // we just skip header and continue to analyze data
         continue;
      }

      //! ================= FPGA TDC header ========================
      TdcProcessor* subproc = GetTDC(data & 0xFFFF);
      if (subproc != 0) {
         subproc->TransformTdcData(sub, ix, datalen);

         ix+=datalen;
         continue; // go to next block
      }  // end of if TDC header


      // all other blocks are ignored
      ix+=datalen;
   }
}

double hadaq::TrbProcessor::CheckAutoCalibration()
{
   // check and perform auto calibration for all TDCs
   // return true only when calibration can be done

   double progress = 1.;

   for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++) {
      if (!iter->second->IsTDC()) continue;
      TdcProcessor* tdc = (TdcProcessor*) iter->second;
      tdc->fCalibrProgress = tdc->TestCanCalibrate();
      if (tdc->fCalibrProgress < progress) progress = tdc->fCalibrProgress;
   }

   if (progress >= 1.)
      for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++) {
         if (iter->second->IsTDC())
            ((TdcProcessor*) iter->second)->PerformAutoCalibrate();
      }

   return progress;
}


