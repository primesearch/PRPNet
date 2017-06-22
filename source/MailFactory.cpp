#include "MailFactory.h"
#include "PrimeMail.h"
#include "WWWWMail.h"

Mail   *MailFactory::GetInstance(globals_t *globals, string smtpServer, int32_t smtpPort)
{
   Mail   *pMail;

   switch (globals->i_ServerType)
   {
      case ST_CULLENWOODALL:
      case ST_GFN:
      case ST_XYYX:
      case ST_GENERIC:
      case ST_FACTORIAL:
      case ST_MULTIFACTORIAL:
      case ST_PRIMORIAL:
      case ST_SIERPINSKIRIESEL:
      case ST_FIXEDBNC:
      case ST_TWIN:
      case ST_SOPHIEGERMAIN:
      case ST_FIXEDBKC:
      case ST_CYCLOTOMIC:
      case ST_WAGSTAFF:
         pMail = new PrimeMail(globals, smtpServer, smtpPort);
         break;

      case ST_WIEFERICH:
      case ST_WILSON:
      case ST_WALLSUNSUN:
      case ST_WOLSTENHOLME:
         pMail = new WWWWMail(globals, smtpServer, smtpPort);
         break;

      default:
         printf("Didn't handle servertype %d\n", globals->i_ServerType);
         exit(0);
   }

   return pMail;
}
