#include "xorg-server.h"
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
	{ 0x0314, "Raspberry Pi (BCM2835)" },
	{ -1, NULL }
};

static XF86ModuleVersionInfo rpiVersRec =
{
	RPI_DRIVER_NAME,
	RPI_MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XORG_VERSION_CURRENT,
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
	xf86PrintChipsets( RPI_NAME, "Driver for Raspberry Pi", RPIChipsets );
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
		int entity = xf86ClaimNoSlot(drv, 0, devSections[i], TRUE);
		xf86AddEntityToScreen(sInfo,entity); 
		sInfo->driverVersion = VERSION;
		sInfo->driverName = RPI_DRIVER_NAME;
		sInfo->name = RPI_NAME;
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
	if( pScrn->driverPrivate == NULL ) return FALSE;
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
	RPIPtr pRPI;
	int default_depth, fbbpp;
	rgb defaultWeight = { 0, 0, 0 };
	rgb defaultMask = { 0, 0, 0 };
	Gamma defaultGamma = { 0.0, 0.0, 0.0 };
	uint64_t value;
	int i;

	INFO_MSG( "Beginning PreInit" );

	if( pScrn->numEntities != 1 )
	{
		ERROR_MSG("Driver expected 1 entity, found %d for screen %d", pScrn->numEntities, pScrn->scrnIndex );
		return FALSE;
	}

	RPIGetRec(pScrn);
	pRPI = RPIPTR(pScrn);

	pRPI->EntityInfo = xf86GetEntityInfo(pScrn->entityList[0]);
	pScrn->monitor = pScrn->confScreen->monitor;

	INFO_MSG( "Loading sub modules" );
	if( !xf86LoadSubModule( pScrn, "fbdevhw" ) )
	{
		goto fail;
	}

	INFO_MSG( "calling FBInit" );	
	if( !fbdevHWInit( pScrn, NULL, "/dev/fb0" ) )
	{
		goto fail;
	}

	INFO_MSG( "Init depth" );
	default_depth = 16;
	fbbpp = 32;
	fbdevHWGetDepth(pScrn, &fbbpp);
	INFO_MSG( "fbdevHWGetDepth set fbbpp = %i", fbbpp );
	if( !xf86SetDepthBpp(pScrn, default_depth, 0, fbbpp, Support32bppFb) )
	{
		goto fail;
	}
	xf86PrintDepthBpp(pScrn);

	INFO_MSG( "Init weight" );
	if( !xf86SetWeight(pScrn, defaultWeight, defaultMask ))
	{
		goto fail;
	}

	INFO_MSG( "Init gamma" );
	if( !xf86SetGamma(pScrn, defaultGamma))
	{
		goto fail;
	}

	INFO_MSG( "Init visual" );
	if( !xf86SetDefaultVisual(pScrn, -1))
	{
		goto fail;
	}

	if( pScrn->depth < 16 )
	{
		ERROR_MSG("The requested default visual (%s) has an unsupported depth (%d).", xf86GetVisualName(pScrn->defaultVisual), pScrn->depth);
		goto fail; 
	}
	pScrn->progClock = TRUE;

	INFO_MSG( "Handle options" );
	xf86CollectOptions(pScrn,NULL);
	if(!(pRPI->Options = calloc(1, sizeof(RPIOptions))))
	{
		goto fail;
	}
	memcpy(pRPI->Options, RPIOptions, sizeof(RPIOptions));
	xf86ProcessOptions(pScrn->scrnIndex, pRPI->EntityInfo->device->options, pRPI->Options);


	INFO_MSG( "Setting the video modes" );
	fbdevHWUseBuildinMode(pScrn);

	INFO_MSG( "dismodes = %p", pScrn->display->modes );
	INFO_MSG( "Mode = %p", pScrn->modes );	
	INFO_MSG( "Mon = %p", pScrn->monitor );
	char* name = fbdevHWGetName(pScrn);
	INFO_MSG( "scrn name = %s", name );
	xf86SetDpi(pScrn,0,0);

	switch(pScrn->bitsPerPixel)
	{
	case 16:
	case 24:
	case 32:
		break;
	default:
		ERROR_MSG( "The requested depth (%d) is unsupported", pScrn->bitsPerPixel);
		goto fail;
	}

	INFO_MSG( "PreInit success" );
	
	return TRUE;

fail:
	INFO_MSG( "PriInit fail" );
	RPIFreeRec(pScrn);
	return FALSE;
}

static Bool RPIModeInit(ScrnInfoPtr pScrn, DisplayModePtr pMode)
{
	INFO_MSG("RPIModeInit");
	return TRUE;
}

static Bool RPIScreenInit(int scrnNum, ScreenPtr pScreen, int argc, char** argv )
{
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
	INFO_MSG("RPIScreenInit");
	return TRUE;
}

static Bool RPISwitchMode(int scrnNum, DisplayModePtr pMode, int flags)
{
	xf86Msg( X_INFO, "RPISwitchMode" );
	return TRUE;
}

static void RPIAdjustFrame(int scrnNum, int x, int y, int flags)
{
	xf86Msg( X_INFO, "RPIAdjustFrame" );
}

static Bool RPIEnterVT(int scrnNum, int flags)
{
	xf86Msg( X_INFO, "RPIEnterVT" );
	return TRUE;
}

static void RPILeaveVT(int scrnNum, int flags)
{
	xf86Msg( X_INFO, "RPILeaveVT" );
}

static void RPIFreeScreen(int scrnNum, int flags)
{
	xf86Msg( X_INFO, "RPIFreeScreen" );
}
