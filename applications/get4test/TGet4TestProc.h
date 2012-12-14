#ifndef TGet4TestProc_H
#define TGet4TestProc_H

#include "go4/TUserProcessor.h"

#include <map>

struct TGet4TestRec {
   unsigned rocid;
   unsigned get4id;
   unsigned chid;

   TH1* fWidth;

   unsigned lastrising;
   unsigned lastfalling;
   unsigned hitcnt;

   TGet4TestRec() : rocid(0), get4id(0), chid(0), fWidth(0) {}

   void reset() {
      lastrising = 0;
      lastfalling = 0;
      hitcnt = 0;
   }

};

typedef std::map<unsigned,TGet4TestRec> TGet4TestMap;

class TGet4TestProc : public TUserProcessor {

   protected:

      TH1* fMultipl;   //!
      TH1* fEvPerCh;   //!  events per channel
      TH1* fBadPerCh;  //!  bad events per channel
      TH1* fHitPerCh;  //!  bad events per channel

      TGet4TestMap fMap; //! all channels which should be present

   public:

      TGet4TestProc();

      TGet4TestProc(const char* name);

      inline unsigned code(unsigned rocid, unsigned get4id, unsigned chid) const { return rocid*1000 + get4id*10 + chid; }

      void Add(unsigned rocid, unsigned get4id)
      {
         for (unsigned indx=0;indx<4;indx++)
            Add(rocid, get4id, indx);
      }

      /** Register specific get4 channel for testing */
      void Add(unsigned rocid, unsigned get4id, unsigned chid);

      /** Create common histograms for all channels */
      void MakeHistos();

      virtual void Process(TStreamEvent*);



   ClassDef(TGet4TestProc, 1)
};


#endif
