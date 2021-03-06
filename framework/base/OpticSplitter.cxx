#include "base/OpticSplitter.h"

#include <string.h>

#include "base/ProcMgr.h"


base::OpticSplitter::OpticSplitter(unsigned brdid) :
   base::StreamProc("Splitter", DummyBrdId, false),
   fMap()
{
   mgr()->RegisterProc(this, base::proc_RawData, brdid);

   // this is raw-scan processor, therefore no synchronization is required for it
   SetSynchronisationKind(sync_None);

   // only raw scan, data can be immediately removed
   SetRawScanOnly();
}

base::OpticSplitter::~OpticSplitter()
{
}

void base::OpticSplitter::AddSub(SysCoreProc* proc, unsigned id)
{
   fMap[id] = proc;
}

bool base::OpticSplitter::FirstBufferScan(const base::Buffer& buf)
{
   // TODO: one can treat special case when buffer data only from single board
   // in this case one could deliver buffer as is to the SysCoreProc

   uint64_t* ptr = (uint64_t*) buf.ptr();

   unsigned len = buf.datalen();

   while(len>0) {
      unsigned brdid = (*ptr & 0xffff);

      SysCoreMap::iterator iter = fMap.find(brdid);

      if (iter != fMap.end()) {
         SysCoreProc* proc = iter->second;

         if (proc->fSplitPtr == 0) {
            proc->fSplitBuf.makenew(buf.datalen());
            proc->fSplitPtr = (uint64_t*) proc->fSplitBuf.ptr();
         }

         memcpy(proc->fSplitPtr, ptr, 8);
         proc->fSplitPtr++;
      }

      len-=8;
      ptr++;
   }

   for (SysCoreMap::iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
      SysCoreProc* proc = iter->second;
      if (proc->fSplitPtr != 0) {
         proc->fSplitBuf.setdatalen((proc->fSplitPtr - (uint64_t*) proc->fSplitBuf.ptr())*8);

         proc->fSplitBuf.rec().kind = buf.rec().kind;
         proc->fSplitBuf.rec().format = buf.rec().format;
         proc->fSplitBuf.rec().boardid = iter->first;

         proc->AddNextBuffer(proc->fSplitBuf);

         proc->fSplitPtr = 0;
         proc->fSplitBuf.reset();
      }
   }

   return true;
}
