#ifndef NX_SUBEVENT_H
#define NX_SUBEVENT_H

#include "base/SubEvent.h"

#include "nx/Message.h"

#include <vector>
#include <algorithm>

namespace nx {


   /*
    * Extended message container. Keeps original ROC message, but adds full timestamp and
    * optionally corrected adc valules.
    * Note that extended messages inside the vector will be sorted after full timestamp
    * by the TRocProc::FinalizeEvent
    *
    */

   class MessageExt : public base::MessageExt<nx::Message> {
      protected:
         /* corrected adc value*/
         float fCorrectedADC;

      public:

         MessageExt() : base::MessageExt<nx::Message>(), fCorrectedADC(0) {}

         MessageExt(const nx::Message& msg, double globaltm) :
            base::MessageExt<nx::Message>(msg, globaltm),
            fCorrectedADC(0)
         {
         }

         MessageExt(const MessageExt& src) :
            base::MessageExt<nx::Message>(src),
            fCorrectedADC(src.fCorrectedADC)
         {
         }

         ~MessageExt() {}

         void SetCorrectedADC(float val) { fCorrectedADC = val; }
         float GetCorrectedNxADC() const { return fCorrectedADC; }
   };


   typedef base::SubEventEx<nx::MessageExt> SubEvent;
}



#endif
