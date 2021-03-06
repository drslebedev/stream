#ifndef HADAQ_TDCMESSAGE_H
#define HADAQ_TDCMESSAGE_H

#include <stdint.h>

#include <stdio.h>

namespace hadaq {

   enum TdcMessageKind {
      tdckind_Trailer  = 0x00000000,
      tdckind_Header   = 0x20000000,
      tdckind_Debug    = 0x40000000,
      tdckind_Epoch    = 0x60000000,
      tdckind_Mask     = 0xe0000000,
      tdckind_Hit      = 0x80000000,  // original hit message
      tdckind_Hit1     = 0xa0000000,  // fine time replaced with 5ps binning value
      tdckind_Hit2     = 0xc0000000,
      tdckind_Calibr   = 0xe0000000   // calibration data for next two hits
   };

   enum TdcConstants {
      MaxNumTdcChannels = 65
   };

   /** TdcMessage is wrapper for data, produced by FPGA-TDC
    * struct is used to avoid any potential overhead */

   struct TdcMessage {
      protected:
         uint32_t   fData;

         static unsigned gFineMinValue;
         static unsigned gFineMaxValue;

      public:

         TdcMessage() : fData(0) {}

         TdcMessage(uint32_t d) : fData(d) {}

         void assign(uint32_t d) { fData = d; }

         inline uint32_t getData() const { return fData; }

         /** assign operator for the message */
         TdcMessage& operator=(const TdcMessage& src) { fData = src.fData; return *this; }

         /** Returns kind of the message
          * If used for the hit message, four different values can be returned */
         inline uint32_t getKind() const { return fData & tdckind_Mask; }

         inline bool isHit0Msg() const { return getKind() == tdckind_Hit; } // original hit message
         inline bool isHit1Msg() const { return getKind() == tdckind_Hit1; } // with replaced fine counter
         inline bool isHit2Msg() const { return getKind() == tdckind_Hit2; } // repaired 0x3fff message
         inline bool isHitMsg() const { return isHit0Msg() || isHit1Msg() || isHit2Msg(); }

         inline bool isEpochMsg() const { return getKind() == tdckind_Epoch; }
         inline bool isDebugMsg() const { return getKind() == tdckind_Debug; }
         inline bool isHeaderMsg() const { return getKind() == tdckind_Header; }
         inline bool isTrailerMsg() const { return getKind() == tdckind_Trailer; }
         inline bool isCalibrMsg() const { return getKind() == tdckind_Calibr; }

         // methods for epoch

         /** Return Epoch for epoch marker, 28 bit */
         inline uint32_t getEpochValue() const { return fData & 0xFFFFFFF; }
         /** Get reserved bit for epoch, 1 bit */
         inline uint32_t getEpochRes() const { return (fData >> 28) & 0x1; }

         // methods for hit

         /** Returns hit channel ID */
         inline uint32_t getHitChannel() const { return (fData >> 22) & 0x7F; }

         /** Returns hit coarse time counter, 11 bit */
         inline uint32_t getHitTmCoarse() const { return fData & 0x7FF; }
         inline void setHitTmCoarse(uint32_t coarse) { fData = (fData & ~0x7FF) | (coarse & 0x7FF); }

         /** Returns hit fine time counter, 10 bit */
         inline uint32_t getHitTmFine() const { return (fData >> 12) & 0x3FF; }

         /** Returns time stamp, which is simple combination coarse and fine counter */
         inline uint32_t getHitTmStamp() const { return (getHitTmCoarse() << 10) | getHitTmFine(); }

         /** Returns hit edge 1 - rising, 0 - falling */
         inline uint32_t getHitEdge() const {  return (fData >> 11) & 0x1; }

         inline bool isHitRisingEdge() const { return getHitEdge() == 0x1; }
         inline bool isHitFallingEdge() const { return getHitEdge() == 0x0; }

         void setAsHit1(uint32_t finebin);

         /** Returns hit reserved value, 2 bits */
         inline uint32_t getHitReserved() const { return (fData >> 29) & 0x3; }

         // methods for calibration message

         inline uint32_t getCalibrFine(unsigned n = 0) const { return (fData >> n*14) & 0x3fff; }
         inline void setCalibrFine(unsigned n = 0, uint32_t v = 0) { fData = (fData & ~(0x3fff << n*14)) | ((v & 0x3fff) << n*14); }

         // methods for header

         /** Return error bits of header message */
         inline uint32_t getHeaderErr() const { return fData & 0xFFFF; }

         /** Return reserved bits of header message */
         inline uint32_t getHeaderRes() const { return (fData >> 16) & 0xFF; }

         /** Return data format: 0 - normal, 1 - double edges for each hit */
         inline uint32_t getHeaderFmt() const { return (fData >> 24) & 0xF; }

         // methods for debug message

         /** Return error bits of header message */
         inline uint32_t getDebugKind() const { return (fData >> 24) & 0xF; }

         /** Return reserved bits of header message */
         inline uint32_t getDebugValue() const { return fData  & 0xFFFFFF; }


         void print(double tm = -1.);

         static double CoarseUnit() { return 5e-9; }

         static double SimpleFineCalibr(unsigned fine)
         {
            if (fine<=gFineMinValue) return 0.;
            if (fine>=gFineMaxValue) return CoarseUnit();
            return (CoarseUnit() * (fine - gFineMinValue)) / (gFineMaxValue - gFineMinValue);
         }

         /** Method set static limits, which are used for simple interpolation of time for fine counter */
         static void SetFineLimits(unsigned min, unsigned max)
         {
            gFineMinValue = min;
            gFineMaxValue = max;
         }
   };

}

#endif
