#include "xorg-server.h"
#include "xf86.h"
#include "xf86_OSproc.h"
#include "micmap.h"
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

	if( !xf86LoadDrvSubModule( drv, "fbdevhw" ) )
	{
		return FALSE;
	}

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

static void RPISave( ScrnInfoPtr pScrn )
{
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

	if (xf86LoadSubModule(pScrn, "fb") == NULL) {
		return FALSE;
	}

	RPIGetRec(pScrn);
	pRPI = RPIPTR(pScrn);

	pRPI->EntityInfo = xf86GetEntityInfo(pScrn->entityList[0]);
	pScrn->monitor = pScrn->confScreen->monitor;

	INFO_MSG( "calling FBInit" );	
	if( !fbdevHWInit( pScrn, NULL, "/dev/fb0" ) )
	{
		goto fail;
	}

	INFO_MSG( "Init depth" );
	default_depth = fbdevHWGetDepth(pScrn, &fbbpp);
	if( !xf86SetDepthBpp(pScrn, default_depth, default_depth, fbbpp, Support32bppFb) )
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
	pScrn->rgbBits = 8;
	pScrn->chipset = "fbdev";
	pScrn->videoRam = fbdevHWGetVidmem(pScrn);

	INFO_MSG( "Handle options" );
	xf86CollectOptions(pScrn,NULL);
	if(!(pRPI->Options = malloc(sizeof(RPIOptions))))
	{
		goto fail;
	}
	memcpy(pRPI->Options, RPIOptions, sizeof(RPIOptions));
	xf86ProcessOptions(pScrn->scrnIndex, pRPI->EntityInfo->device->options, pRPI->Options);


	INFO_MSG( "Setting the video modes" );
	fbdevHWSetVideoModes(pScrn);
	{
		DisplayModePtr mode, first = mode = pScrn->modes;
		if( mode != NULL ) do {
			mode->status = xf86CheckModeForMonitor(mode, pScrn->monitor);
			mode = mode->next;
		} while( mode != NULL && mode != first );

		xf86PruneDriverModes(pScrn);
	}

	if( pScrn->modes == NULL )
	{
		fbdevHWUseBuildinMode(pScrn);
	}
	pScrn->currentMode = pScrn->modes;

	pScrn->displayWidth = pScrn->virtualX;

	xf86PrintModes(pScrn);
	
	xf86SetDpi(pScrn,0,0);

	switch(pScrn->bitsPerPixel)
	{
	case 8:
	case 16:
	case 24:
	case 32:
		break;
	default:
		ERROR_MSG( "The requested depth (%d) is unsupported", pScrn );
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
	RPIPtr pRPI = RPIPTR(pScrn);
	VisualPtr visual;
	int init_picture = 0;
	int ret, flags;
	int type;

	INFO_MSG("RPIScreenInit");

	INFO_MSG("\tbitsPerPixel=%d, depth=%d, defaultVisual=%s\n"
	       "\tmask: %x,%x,%x, offset: %d,%d,%d\n",
	       pScrn->bitsPerPixel,
	       pScrn->depth,
	       xf86GetVisualName(pScrn->defaultVisual),
	       pScrn->mask.red,pScrn->mask.green,pScrn->mask.blue,
	       pScrn->offset.red,pScrn->offset.green,pScrn->offset.blue);
	
	if( (pRPI->fbmem = fbdevHWMapVidmem(pScrn)) == NULL)
	{
		return FALSE;
	}

	RPISave(pScrn);
	if( !RPIModeInit(pScrn, pScrn->currentMode ) )
	{
		return FALSE;
	}
	RPIAdjustFrame(scrnNum, pScrn->frameX0, pScrn->frameY0, 0 );
	
	miClearVisualTypes();
	if( !miSetVisualTypes( pScrn->depth, TrueColorMask, pScrn->rgbBits, TrueColor ) )
	{
		return FALSE;
	}

	if( !miSetPixmapDepths() )
	{
		return FALSE;
	}
	INFO_MSG( "Visual types set" );

	if( !fbScreenInit(pScreen, pRPI->fbstart, pScrn->virtualX, pScrn->virtualY, pScrn->xDpi, pScrn->yDpi, pScrn->displayWidth, pScrn->bitsPerPixel ) )
	{
		return FALSE;
	}

	if (pScrn->bitsPerPixel > 8) {
		/* Fixup RGB ordering */
		visual = pScreen->visuals + pScreen->numVisuals;
		while (--visual >= pScreen->visuals) {
			if ((visual->class | DynamicClass) == DirectColor) {
				visual->offsetRed   = pScrn->offset.red;
				visual->offsetGreen = pScrn->offset.green;
				visual->offsetBlue  = pScrn->offset.blue;
				visual->redMask     = pScrn->mask.red;
				visual->greenMask   = pScrn->mask.green;
				visual->blueMask    = pScrn->mask.blue;
			}
		}
	}

	INFO_MSG( "ScreenInit finished" );
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

