#include "xf86.h"
#include "xf86_OSproc.h"
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
	int numDevSections, numUsed;
	GDevPtr *devSections;
	int *usedChips;
	int i;
	Bool foundScreen = FALSE;

	if( (numDevSections = xf86MatchDevice(RPI_DRIVER_NAME,&devSections)) <= 0 ) return FALSE;

	for( i = 0; i < numDevSections; ++i )
	{
		ScrnInfoPtr sInfo = xf86AllocateScreen(drv, 0);	
		sInfo->driverVersion = VERSION;
		sInfo->driverName = RPI_DRIVER_NAME;
		sInfo->Probe = RPIProbe;
		sInfo->PreInit = RPIPreInit;
		sInfo->ScreenInit = RPIScreenInit;
		sInfo->SwitchMode = RPISwitchMode;
		sInfo->AdjustFrame = RPIAdjustFrame;
		sInfo->EnterVT = RPIEnterVT;
		sInfo->LeaveVT = RPILeaveVT;
		sInfo->FreeScreen = RPIFreeScreen;
		foundScreen = TRUE;
	}

	free(devSections);
	return foundScreen;
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
		xf86AddDriver(&RPI,module,0);
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

static Bool RPIGetRec(ScrnInfoPtr pScrn)
{
	if( pScrn->driverPrivate != NULL ) return TRUE;
	pScrn->driverPrivate = xnfcalloc(sizeof(RPIRec), 1);
	return TRUE;
}

static void RPIFreeRec(ScrnInfoPtr pScrn)
{
	if( pScrn->driverPrivate == NULL ) return;

	free(pScrn->driverPrivate);
	pScrn->driverPrivate = NULL;
}

static Bool RPIPreInit(ScrnInfoPtr pScrn,int flags)
{
	return TRUE;
}

static Bool RPIModeInit(ScrnInfoPtr pScrn, DisplayModePtr pMode)
{
	return TRUE;
}

static Bool RPIScreenInit(int scrnNum, ScreenPtr pScrn, int argc, char** argv )
{
	return TRUE;
}

static Bool RPISwitchMode(int scrnNum, DisplayModePtr pMode, int flags)
{
	return TRUE;
}

static void RPIAdjustFrame(int scrnNum, int x, int y, int flags)
{
}

static Bool RPIEnterVT(int scrnNum, int flags)
{
	return TRUE;
}

static void RPILeaveVT(int scrnNum, int flags)
{
}

static void RPIFreeScreen(int scrnNum, int flags)
{
}
