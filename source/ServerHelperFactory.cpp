#include "ServerHelperFactory.h"
#include "PrimeServerHelper.h"
#include "WWWWServerHelper.h"

ServerHelper  *ServerHelperFactory::GetServerHelper(globals_t *globals, DBInterface *dbInterface)
{
   ServerHelper *serverHelper;

   switch (globals->i_ServerType)
   {
      case ST_SIERPINSKIRIESEL:
      case ST_CULLENWOODALL:
      case ST_PRIMORIAL:
      case ST_FACTORIAL:
      case ST_MULTIFACTORIAL:
      case ST_GFN:
      case ST_XYYX:
      case ST_GENERIC:
      case ST_FIXEDBNC:
      case ST_TWIN:
      case ST_SOPHIEGERMAIN:
      case ST_FIXEDBKC:
      case ST_CYCLOTOMIC:
      case ST_CAROLKYNEA:
      case ST_WAGSTAFF:
         serverHelper = new PrimeServerHelper(dbInterface, globals->p_Log);
         break;

      case ST_WIEFERICH:
      case ST_WILSON:
      case ST_WALLSUNSUN:
      case ST_WOLSTENHOLME:
         serverHelper = new WWWWServerHelper(dbInterface, globals->p_Log);
         break;

      default:
         printf("Didn't handle servertype %d\n", globals->i_ServerType);
         exit(0);
   }

   return serverHelper;
}
