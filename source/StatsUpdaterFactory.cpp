#include "StatsUpdaterFactory.h"
#include "PrimorialStatsUpdater.h"
#include "FactorialStatsUpdater.h"
#include "MultiFactorialStatsUpdater.h"
#include "GFNStatsUpdater.h"
#include "XYYXStatsUpdater.h"
#include "CullenWoodallStatsUpdater.h"
#include "FixedBKCStatsUpdater.h"
#include "FixedBNCStatsUpdater.h"
#include "SierpinskiRieselStatsUpdater.h"
#include "SophieGermainStatsUpdater.h"
#include "GenericStatsUpdater.h"
#include "CyclotomicStatsUpdater.h"
#include "CarolKyneaStatsUpdater.h"
#include "WWWWStatsUpdater.h"
#include "WagstaffStatsUpdater.h"

StatsUpdater   *StatsUpdaterFactory::GetInstance(DBInterface *dbInterface, Log *theLog, int32_t serverType, bool needsDoubleCheck)
{
   StatsUpdater   *theUpdater;

   switch (serverType)
   {
      case ST_CULLENWOODALL:
         theUpdater = new CullenWoodallStatsUpdater();
         break;

      case ST_GFN:
         theUpdater = new GFNStatsUpdater();
         break;

      case ST_XYYX:
         theUpdater = new XYYXStatsUpdater();
         break;
         
      case ST_GENERIC:
         theUpdater = new GenericStatsUpdater();
         break;

      case ST_FACTORIAL:
         theUpdater = new FactorialStatsUpdater();
         break;

      case ST_MULTIFACTORIAL:
         theUpdater = new MultiFactorialStatsUpdater();
         break;

      case ST_PRIMORIAL:
         theUpdater = new PrimorialStatsUpdater();
         break;

      case ST_SIERPINSKIRIESEL:
         theUpdater = new SierpinskiRieselStatsUpdater();
         break;

      case ST_FIXEDBNC:
      case ST_TWIN:
         theUpdater = new FixedBNCStatsUpdater();
         break;

      case ST_SOPHIEGERMAIN:
         theUpdater = new SophieGermainStatsUpdater();
         break;

      case ST_FIXEDBKC:
         theUpdater = new FixedBKCStatsUpdater();
         break;
         
      case ST_CYCLOTOMIC:
         theUpdater = new CyclotomicStatsUpdater();
         break;
         
      case ST_CAROLKYNEA:
         theUpdater = new CarolKyneaStatsUpdater();
         break;

      case ST_WAGSTAFF:
         theUpdater = new WagstaffStatsUpdater();
         break;

      case ST_WIEFERICH:
      case ST_WILSON:
      case ST_WALLSUNSUN:
      case ST_WOLSTENHOLME:
         theUpdater = new WWWWStatsUpdater();
         break;

      default:
         printf("Didn't handle servertype %d\n", serverType);
         exit(0);
   }

   // Use the existing database connection
   theUpdater->SetDBInterface(dbInterface);
   theUpdater->SetLog(theLog);
   theUpdater->SetNeedsDoubleCheck(needsDoubleCheck);

   return theUpdater;
}
