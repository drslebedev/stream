#ifndef HADAQ_TRBPROCESSOR_H
#define HADAQ_TRBPROCESSOR_H

#include "base/StreamProc.h"

#include <map>

#include "hadaq/defines.h"

namespace hadaq {

   class TdcProcessor;


   /** This is generic processor for data, coming from TRB board
    * Normally one requires specific sub-processor for frontend like TDC or any other
    * Idea that TrbProcessor can interpret HADAQ event/subevent structures and
    * will distribute data to sub-processors.  */

   typedef std::map<unsigned,TdcProcessor*> SubProcMap;

   class TrbProcessor : public base::StreamProc {

      friend class TdcProcessor;

      protected:

         SubProcMap fMap;

         unsigned fLastTriggerId;    //! last seen trigger id
         unsigned fLostTriggerCnt;   //! lost trigger counts
         unsigned fTakenTriggerCnt;  //! registered trigger counts

         base::H1handle fEvSize;     //! HADAQ event size
         base::H1handle fSubevSize;  //! HADAQ subevent size
         base::H1handle fLostRate;   //! lost rate

         /** Returns true when processor used to select trigger signal
          * TRB3 not yet able to perform trigger selection */
         virtual bool doTriggerSelection() const { return false; }

         void AccountTriggerId(unsigned id);

         /** Way to register sub-processor, like for TDC */
         void AddSub(TdcProcessor* tdc, unsigned id);

         /** Scan FPGA-TDC data, distribute over sub-processors */
         void ScanTDCV3(hadaq::RawSubevent* sub);

      public:

         TrbProcessor(unsigned brdid);
         virtual ~TrbProcessor();

         /** Set trigger window not only for itself, bit for all subprocessors */
         virtual void SetTriggerWindow(double left, double right);

         /** Scan all messages, find reference signals */
         virtual bool FirstBufferScan(const base::Buffer& buf);

   };
}

#endif
