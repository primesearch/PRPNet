#include "HTMLGenerator.h"

HTMLGenerator::HTMLGenerator(globals_t *theGlobals)
{
   ii_ServerType = theGlobals->i_ServerType;
   is_HTMLTitle = theGlobals->s_HTMLTitle;
   is_ProjectTitle = theGlobals->s_ProjectTitle;
   is_CSSLink = theGlobals->s_CSSLink;
   is_SortLink = theGlobals->s_SortLink;
   ib_ServerStatsSummaryOnly = theGlobals->b_ServerStatsSummaryOnly;

   ib_NeedsDoubleCheck = theGlobals->b_NeedsDoubleCheck;
   ib_UseLocalTime = theGlobals->b_LocalTimeHTML;
   ii_DelayCount = theGlobals->i_DelayCount;
   ip_Delay = theGlobals->p_Delay;
}
