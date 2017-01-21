#include "HTMLGeneratorFactory.h"
#include "WWWWHTMLGenerator.h"
#include "CullenWoodallHTML.h"
#include "SierpinskiRieselHTML.h"
#include "FixedBKCHTML.h"
#include "FixedBNCHTML.h"
#include "GFNHTML.h"
#include "XYYXHTML.h"
#include "PrimorialHTML.h"
#include "FactorialHTML.h"
#include "MultiFactorialHTML.h"
#include "SophieGermainHTML.h"
#include "CyclotomicHTML.h"
#include "CarolKyneaHTML.h"
#include "GenericHTML.h"
#include "WagstaffHTML.h"

HTMLGenerator  *HTMLGeneratorFactory::GetHTMLGenerator(globals_t *globals)
{
   HTMLGenerator *htmlGenerator;

   switch (globals->i_ServerType)
   {
      case ST_SIERPINSKIRIESEL:
         htmlGenerator = new SierpinskiRieselHTML(globals);
         break;

      case ST_CULLENWOODALL:
         htmlGenerator = new CullenWoodallHTML(globals);
         break;

      case ST_PRIMORIAL:
         htmlGenerator = new PrimorialHTML(globals);
         break;

      case ST_FACTORIAL:
         htmlGenerator = new FactorialHTML(globals);
         break;

      case ST_MULTIFACTORIAL:
         htmlGenerator = new MultiFactorialHTML(globals);
         break;

      case ST_GFN:
         htmlGenerator = new GFNHTML(globals);
         break;

      case ST_FIXEDBNC:
      case ST_TWIN:
         htmlGenerator = new FixedBNCHTML(globals);
         break;

      case ST_SOPHIEGERMAIN:
         htmlGenerator = new SophieGermainHTML(globals);
         break;

      case ST_FIXEDBKC:
         htmlGenerator = new FixedBKCHTML(globals);
         break;

      case ST_XYYX:
         htmlGenerator = new XYYXHTML(globals);
         break;
         
      case ST_CYCLOTOMIC:
         htmlGenerator = new CyclotomicHTML(globals);
         break;
         
      case ST_CAROLKYNEA:
         htmlGenerator = new CarolKyneaHTML(globals);
         break;

      case ST_GENERIC:
         htmlGenerator = new GenericHTML(globals);
         break;

      case ST_WIEFERICH:
      case ST_WILSON:
      case ST_WALLSUNSUN:
      case ST_WOLSTENHOLME:
         htmlGenerator = new WWWWHTMLGenerator(globals);
         break;
         
      case ST_WAGSTAFF:
         htmlGenerator = new WagstaffHTML(globals);
         break;

      default:
         printf("Didn't handle servertype %d\n", globals->i_ServerType);
         exit(0);
   }

   return htmlGenerator;
}
