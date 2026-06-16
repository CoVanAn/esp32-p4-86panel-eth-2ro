#ifndef KS_SOURCE_UI_BRIDGE_H
#define KS_SOURCE_UI_BRIDGE_H

/*
 * Header bridge để các asset trong src/fonts và src/images
 * khi include "../ui.h" sẽ trỏ tới src/ksmart/ui.h
 */
#ifdef LV_FONT_FMT_TXT_LARGE
#undef LV_FONT_FMT_TXT_LARGE
#endif
#define LV_FONT_FMT_TXT_LARGE 1

#include "ksmart/ui.h"

#endif
