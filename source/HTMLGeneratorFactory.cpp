#include "HTMLGeneratorFactory.h"
#include "SimplePrimeHTML.h"
#include "SierpinskiRieselHTML.h"
#include "GenericHTML.h"
#include "DMDivisorHTMLGenerator.h"
#include "GFNDivisorHTMLGenerator.h"

HTMLGenerator  *HTMLGeneratorFactory::GetHTMLGenerator(globals_t *globals)
{
   HTMLGenerator *htmlGenerator;

   switch (globals->i_ServerType)
   {
      case ST_SIERPINSKIRIESEL:
         htmlGenerator = new SierpinskiRieselHTML(globals);
         break;

      case ST_CULLENWOODALL:
      case ST_PRIMORIAL:
      case ST_FACTORIAL:
      case ST_MULTIFACTORIAL:
      case ST_TWIN:
      case ST_TWINANDSOPHIE:
      case ST_SOPHIEGERMAIN:
      case ST_FIXEDBKC:
      case ST_CAROLKYNEA:
      case ST_HYPERCW:
      case ST_GENERIC:
      case ST_WAGSTAFF:
         htmlGenerator = new SimplePrimeHTML(globals, BY_N);
         break;

      case ST_CYCLOTOMIC:
      case ST_GFN:
         htmlGenerator = new SimplePrimeHTML(globals, BY_B);
         break;

      case ST_FIXEDBNC:
         htmlGenerator = new SimplePrimeHTML(globals, BY_K);
         break;

      case ST_LEYLAND:
      case ST_LIFCHITZ:
         htmlGenerator = new SimplePrimeHTML(globals, BY_Y);
         break;

      case ST_DMDIVISOR:
         htmlGenerator = new DMDivisorHTMLGenerator(globals);
         break;

      case ST_GFNDIVISOR:
         htmlGenerator = new GFNDivisorHTMLGenerator(globals);
         break;

      default:
         printf("Didn't handle servertype %d\n", globals->i_ServerType);
         exit(0);
   }

   return htmlGenerator;
}
