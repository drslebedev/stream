void first()
{
   // analysis will work as triggerred -
   // after each event all data should be processed and flushed
   base::ProcMgr::instance()->SetTriggeredAnalysis();

   hadaq::TdcMessage::SetFineLimits(31, 421);

   hadaq::TrbProcessor* trb3 = new hadaq::TrbProcessor(0);

   // indicate if raw data should be printed
   trb3->SetPrintRawData(false);

   // set number of errors to be printed, default 1000
   trb3->SetPrintErrors(100);

   // Following levels of histograms filling are supported
   //  0 - none
   //  1 - only basic statistic from TRB
   //  2 - generic statistic over TDC channels
   //  3 - basic per-channel histograms with IDs
   //  4 - per-channel histograms with references
   trb3->SetHistFilling(2); // only basic histograms, no per-channel histograms

   // Create TDC processors with specified ids
   trb3->CreateTDC(0x8a00, 0x8a01, 0x8a02, 0x8a03);

   // Load calibrations for ALL TDCs
   trb3->LoadCalibrations("/data.local1/padiwa/new_");

   // calculate and write calibrations at the end of the run
   //trb3->SetWriteCalibrations("/data.local1/padiwa/new_");

   // enable automatic calibrations of the channels
   //trb3->SetAutoCalibrations(100000);

   // method set window for all TDCs
   trb3->SetTriggerWindow(-4e-7, -0.2e-7);
}
