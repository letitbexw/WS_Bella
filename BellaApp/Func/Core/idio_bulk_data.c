/*
 * idio_bulk_data.c
 *
 *  Created on: Feb 3, 2026
 *      Author: xiongwei
 */


#include "config.h"
#include "debug.h"
#include "timers.h"
#ifdef CNFG_AID_HAS_BULK_DATA_IAP2
#include "ep_iap.h"
#endif
#ifdef CNFG_AID_HAS_BULK_DATA_HID
#include "ep_hid.h"
#include "hid.h"
#endif
#ifdef CNFG_AID_HAS_BULK_DATA_EA
#include "ep_ea.h"
#endif
#ifdef CNFG_AID_HAS_BULK_DATA_PD
#include "ep_pd.h"
#include "aid_pd.h"
#endif
#include "idio.h"
#include "idio_bulk_data.h"
#include "main.h"
