#ifndef GET4_MBSPROCESSOR_H
#define GET4_MBSPROCESSOR_H

#include "base/StreamProc.h"

namespace get4 {

   enum { NumGet4Channels = 4,
          BinWidthPs = 50,
          FineCounterBins = 0x80 };

   struct Get4MbsChRec {
      base::H1handle fRisFineTm;   ///< histograms of rising stamp for each channel
      base::H1handle fRisCal;      ///< calibration of rising edge
      base::H1handle fFalFineTm;   ///< histograms of falling stamp for each channel
      base::H1handle fFalCal;      ///< calibration of falling edge
      base::H1handle fTotTm;       ///< histograms of time-over threshold for each channel

      double firstr;
      double firstf;
      double lastr;
      double lastf;

      long rising_stat[FineCounterBins];
      double rising_calibr[FineCounterBins];
      long falling_stat[FineCounterBins];
      double falling_calibr[FineCounterBins];

      void clearTimes()
      {
         firstr = 0.;
         firstf = 0.;
         lastr = 0.;
         lastf = 0.;
      }

      double gettm(bool r, bool first = true)
      {
         return r ? (first ? firstr : lastr) : (first ? firstf : lastf);
      }


      void init()
      {
         fRisFineTm = 0; fRisCal = 0;
         fFalFineTm = 0; fFalCal = 0;
         fTotTm = 0;
         clearTimes();

         for (unsigned i=0;i<FineCounterBins;i++) {
            rising_calibr[i] = 1.*BinWidthPs*i;
            falling_calibr[i] = 1.*BinWidthPs*i;
            rising_stat[i] = 0;
            falling_stat[i] = 0;
         }
      }

   };


   struct Get4MbsRec {
      bool used;

      base::H1handle fChannels;  //! histogram with channels
      base::H1handle fErrors;  //! errors kinds

      Get4MbsChRec CH[NumGet4Channels]; //! channels-relevant data

      Get4MbsRec() :
         used(false),
         fChannels(0),
         fErrors(0)
      {
         for(unsigned n=0;n<NumGet4Channels;n++) CH[n].init();
      }

      void clearTimes()
      {
         for(unsigned n=0;n<NumGet4Channels;n++) CH[n].clearTimes();
      }
   };

   struct Get4MbsRef {
      unsigned g1,ch1, g2, ch2;
      bool r1, r2;
      base::H1handle fHist;  //! histogram with channels

      Get4MbsRef() : g1(0), ch1(0), g2(0), ch2(0), r1(true), r2(true), fHist(0) {}
   };

   /** Only for Get4 debug, produce no output data */

   class MbsProcessor : public base::StreamProc {

      protected:

         bool         fIs32mode;     // is 32 bit mode
         unsigned     fTotMult;      // multiplier of tot in 32 bit mode
         std::string  fWriteCalibr;   //! file name to write calibrations at the end
         bool         fUseCalibr;     //! if true, calibration will be used
         long         fAutoCalibr;    //! if positive, will try to calibrate get4 channels

         base::H1handle fMsgPerGet4;   //! histogram with messages per get4
         base::H1handle fErrPerGet4;   //! histogram with errors per get4

//         base::H2handle fSlMsgPerGet4; //! histogram with slow control per GET4
//         base::H2handle fScalerPerGet4; //! histogram with slow control scaler per GET4
//         base::H2handle fDeadPerGet4; //! histogram with slow control dead time per GET4
//         base::H2handle fSpiPerGet4; //! histogram with slow control Spi per GET4
//         base::H2handle fSeuPerGet4; //! histogram with slow control SEU counter per GET4
//         base::H2handle fDllPerGet4; //! histogram with DLL flag per GET4

         std::vector<get4::Get4MbsRec> GET4;    //! vector with GET4-specific histograms

         std::vector<get4::Get4MbsRef> fRef;    //! arrays with reference between channels


         bool get4_in_use(unsigned id) { return id < GET4.size() ? GET4[id].used : false; }

         bool CalibrateChannel(unsigned get4id, unsigned nch, long* statistic, double* calibr, long stat_limit);

         void ProduceCalibration(long stat_limit);

         void CopyCalibration(double* calibr, base::H1handle hcalibr);

         void StoreCalibration(const std::string& fname);


      public:

         MbsProcessor(unsigned get4mask = 0x3, bool is32bit = false, unsigned totmult = 1);
         virtual ~MbsProcessor();

         bool AddRef(unsigned g2, unsigned ch2, bool r2,
                     unsigned g1, unsigned ch1, bool r1,
                     int nbins, double min, double max);

         /** Call to load calibrations from the file */
         bool LoadCalibration(const std::string& fname);

         void SetAutoCalibr(long stat_limit = 1000000);

         void SetWriteCalibr(const std::string& fname) { fWriteCalibr = fname; }

         /** Scan all messages, find reference signals */
         virtual bool FirstBufferScan(const base::Buffer& buf);

         virtual void UserPreLoop();
         virtual void UserPostLoop();
   };
}

#endif
