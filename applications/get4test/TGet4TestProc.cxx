#include "TGet4TestProc.h"

#include "TH1.h"

#include "TGo4Log.h"

#include "get4/SubEvent.h"

#include "go4/TStreamEvent.h"

TGet4TestProc::TGet4TestProc() :
   TUserProcessor()
{
}

TGet4TestProc::TGet4TestProc(const char* name) :
   TUserProcessor(name)
{
   fMultipl = MakeTH1('I', "GET4TEST/Get4TestMultipl", "Number of messages in event", 32, 0., 32.);
}


void TGet4TestProc::Add(unsigned rocid, unsigned get4id, unsigned chid)
{
   unsigned id = code(rocid, get4id, chid);

   TGet4TestRec& rec = fMap[id];

   rec.rocid = rocid;
   rec.get4id = get4id;
   rec.chid = chid;

   TString prefix = Form("GET4TEST/ROC%u_GET%u_CH%u/HROC%u_GET%u_CH%u_", rocid, get4id, chid, rocid, get4id, chid);
   TString info = Form("ROC%u GET4 %u Channel %u", rocid, get4id, chid);

   rec.fWidth = MakeTH1('I', (prefix+"Width").Data(), (TString("Signal width on ") + info).Data(), 4096, 0., 4096.);
}


void TGet4TestProc::Process(TStreamEvent* ev)
{

   for (TGet4TestMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++) {
      iter->second.reset();
   }

    get4::SubEvent* sub0 = dynamic_cast<get4::SubEvent*> (ev->GetSubEvent("ROC0"));

    if (sub0!=0) {
       // TGo4Log::Info("Find GET4 data for ROC0 size %u  trigger: %10.9f", sub0->fExtMessages.size(), ev->GetTriggerTime()*1e-9);
       fMultipl->Fill(sub0->fExtMessages.size());

       TGet4TestMap::iterator iter;

       for (unsigned cnt=0;cnt<sub0->fExtMessages.size();cnt++) {
          const get4::Message& msg = sub0->fExtMessages[cnt].msg();

          unsigned indx = code(0, msg.getGet4Number(), msg.getGet4ChNum());

          iter = fMap.find(indx);
          if (iter == fMap.end()) continue;


          TGet4TestRec& rec = iter->second;

          if (msg.getGet4Edge() == 0) {
             rec.lastrising = msg.getGet4Ts();
          } else {
             rec.lastfalling = msg.getGet4Ts();

             rec.fWidth->Fill(rec.lastfalling - rec.lastrising);
          }
       }
    }
    else
       TGo4Log::Error("Not found GET4 data for ROC0");
}
