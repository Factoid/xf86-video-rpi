#include "xf86.h"
#include "xf86_OSproc.h"
//#include "xf86_ansic.h"
//#include "xf86Resources.h"
#include "rpi_video.h"

static DriverRec RPI = {
	VERSION,
	RPI_DRIVER_NAME,
	RPIIdentify,
	RPIProbe,
	RPIAvailableOptions,
	NULL,
	0
};

static SymTabRec RPIChipsets[] = {
	{ VIDEOCORE_IV, "rpivc4" },
	{ -1, NULL }
};

static XF86ModuleVersionInfo rpiVersRec =
{
    "rpi",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XF86_VERSION_CURRENT,
    RPI_MAJOR_VERSION, RPI_MINOR_VERSION, RPI_PATCHLEVEL,
    ABI_CLASS_VIDEODRV,
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_VIDEODRV,
    {0,0,0,0}
};

XF86ModuleData rpiModuleData = { &rpiVersRec, rpiSetup, NULL };

static const OptionInfoRec RPIOptions[] = {
  { OPTION_HW_CURSOR, "HWcursor",  OPTV_BOOLEAN, {0}, FALSE },
  { OPTION_NOACCEL,   "NoAccel",   OPTV_BOOLEAN, {0}, FALSE },
  { -1,               NULL,        OPTV_NONE,    {0}, FALSE }
};

static void RPIIdentify(int flags)
{
  xf86PrintChipsets( RPI_NAME, "driver for rpi", RPIChipsets );
}

static Bool RPIProbe(DriverPtr drv, int flags)
{
  return FALSE;
}

static const OptionInfoRec* RPIAvailableOptions(int flags, int bus)
{
  return RPIOptions;
}

static pointer rpiSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;

    /* This module should be loaded only once, but check to be sure. */

    if (!setupDone) {
        /*
         * Modules that this driver always requires may be loaded
         * here  by calling LoadSubModule().
         */

        setupDone = TRUE;

        /*
         * The return value must be non-NULL on success even though
         * there is no TearDownProc.
         */
        return (pointer)1;
    } else {
        if (errmaj) *errmaj = LDR_ONCEONLY;
        return NULL;
    }
}
