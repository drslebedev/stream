#include "hadaq/definess.h"

#include <stdio.h>
#include <sys/time.h>
#include <time.h>


void hadaqs::RawEvent::Dump()
{
   printf("*** Event #0x%06x fullid=0x%04x size %u *** \n",
         (unsigned) GetSeqNr(), (unsigned) GetId(), (unsigned) GetSize());
}

void hadaqs::RawEvent::InitHeader(uint32_t id)
{
   tuDecoding = EvtDecoding_64bitAligned;
   SetId(id);
   SetSize(sizeof(hadaqs::RawEvent));
   // timestamp at creation of structure:
   time_t tempo = time(NULL);
   struct tm* gmTime = gmtime(&tempo);
   uint32_t date = 0, clock = 0;
   date |= gmTime->tm_year << 16;
   date |= gmTime->tm_mon << 8;
   date |= gmTime->tm_mday;
   SetDate(date);
   clock |= gmTime->tm_hour << 16;
   clock |= gmTime->tm_min << 8;
   clock |= gmTime->tm_sec;
   SetTime(clock);
}

// ===========================================================

void hadaqs::RawSubevent::Dump(bool print_raw_data)
{
   printf("   *** Subevent size %u decoding 0x%06x id 0x%04x trig 0x%08x %s align %u *** \n",
             (unsigned) GetSize(),
             (unsigned) GetDecoding(),
             (unsigned) GetId(),
             (unsigned) GetTrigNr(),
             IsSwapped() ? "swapped" : "not swapped",
             (unsigned) Alignment());

   if (!print_raw_data) return;

   bool newline = true;

   unsigned size = ((GetSize() - sizeof(RawSubevent)) / Alignment());
   unsigned width = 2;
   if (size>=100) width = 3;
   if (size>=1000) width = 4;

   for (unsigned ix=0; ix < size; ix++)
   {
      if (ix % 8 == 0) printf("  ");

      newline = false;

      printf("  [%*u] %08x", width, ix, (unsigned) Data(ix));

      if (((ix + 1) % 8) == 0)  { printf("\n"); newline = true; }
   }

   if (!newline) printf("\n");
}
