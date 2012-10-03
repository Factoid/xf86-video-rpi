#include "config.h"
#include <xorg-server.h>
#include <xf86.h>
#include <xf86_OSproc.h>
#include <micmap.h>
#include <gcstruct.h>
#include <X11/extensions/render.h>
#include "rpi_video.h"
#include <migc.h>
#include <mi.h>

_X_EXPORT DriverRec RPIDriver = {
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

_X_EXPORT XF86ModuleData rpiModuleData = { &rpiVersRec, rpiSetup, NULL };

static const OptionInfoRec RPIOptions[] = {
	{ OPTION_HW_CURSOR, "HWcursor",  OPTV_BOOLEAN, {0}, FALSE },
	{ OPTION_NOACCEL,   "NoAccel",   OPTV_BOOLEAN, {0}, FALSE },
	{ -1,               NULL,        OPTV_NONE,    {0}, FALSE }
};

static void printConfig( ScrnInfoPtr scrn, EGLDisplay disp, EGLConfig config )
{
   EGLint val;
   const char* names[] = { "EGL_BUFFER_SIZE", "EGL_RED_SIZE", "EGL_GREEN_SIZE", "EGL_BLUE_SIZE", "EGL_ALPHA_SIZE", "EGL_CONFIG_CAVEAT", "EGL_CONFIG_ID", "EGL_DEPTH_SIZE", "EGL_LEVEL", "EGL_MAX_PBUFFER_WIDTH", "EGL_MAX_PBUFFER_HEIGHT", "EGL_MAX_PBUFFER_PIXELS", "EGL_NATIVE_RENDERABLE", "EGL_NATIVE_VISUAL_ID", "EGL_NATIVE_VISUAL_TYPE", "EGL_SAMPLE_BUFFERS", "EGL_SAMPLES", "EGL_STENCIL_", "EGL_SURFACE_TYPE", "EGL_TRANSPARENT_TYPE", "EGL_TRANSPARENT_RED", "EGL_TRANSPARENT_GREEN", "EGL_TRANSPARENT_BLUE" };
   const EGLint values[] = { EGL_BUFFER_SIZE, EGL_RED_SIZE, EGL_GREEN_SIZE, EGL_BLUE_SIZE, EGL_ALPHA_SIZE, EGL_CONFIG_CAVEAT, EGL_CONFIG_ID, EGL_DEPTH_SIZE, EGL_LEVEL, EGL_MAX_PBUFFER_WIDTH, EGL_MAX_PBUFFER_HEIGHT, EGL_MAX_PBUFFER_PIXELS, EGL_NATIVE_RENDERABLE, EGL_NATIVE_VISUAL_ID, EGL_NATIVE_VISUAL_TYPE, EGL_SAMPLE_BUFFERS, EGL_SAMPLES, EGL_STENCIL_SIZE, EGL_SURFACE_TYPE, EGL_TRANSPARENT_TYPE, EGL_TRANSPARENT_RED_VALUE, EGL_TRANSPARENT_GREEN_VALUE, EGL_TRANSPARENT_BLUE_VALUE };
   const int nValues = sizeof(values)/sizeof(EGLint);

   printf( "n values = %i\n", nValues );
   for(int i = 0; i < nValues; ++i )
   {
      eglGetConfigAttrib(disp,config,values[i],&val);
      xf86DrvMsg(scrn->scrnIndex, X_INFO, "%s = %i\n", names[i], val);
   }
}

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

	if (!xf86LoadDrvSubModule(drv, "fb"))
		return FALSE;

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
		xf86AddDriver(&RPIDriver,module,0);
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
	INFO_MSG("Start PreInit");

	RPIGetRec(pScrn);
	bcm_host_init();
	EGLConfig config;
	EGLBoolean result;
	EGLint num_config;

	RPIPtr state = RPIPTR(pScrn);
	static const EGLint attribute_list[] =
	{
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE
	};

	static const EGLint context_attributes[] = 
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	// get an EGL display connection
	state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	assert(state->display!=EGL_NO_DISPLAY);

	// initialize the EGL display connection
	result = eglInitialize(state->display, NULL, NULL);
	assert(EGL_FALSE != result);

	// get an appropriate EGL frame buffer configuration
	result = eglChooseConfig(state->display, attribute_list, &config, 1, &num_config);
	assert(EGL_FALSE != result);
	printConfig(pScrn,state->display,config);

	// get an appropriate EGL frame buffer configuration
	//   result = eglBindAPI(EGL_OPENGL_ES_API);
	//   assert(EGL_FALSE != result);
	//   check();

	// create an EGL rendering context
	state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, context_attributes);
	if( state->context == EGL_NO_CONTEXT )
	{
		ERROR_MSG("No context!");
		goto fail;
	} else
	{
		INFO_MSG("Context get! %p", state->context);
	}
	assert(state->context!=EGL_NO_CONTEXT);

	eglGetConfigAttrib(state->display,config,EGL_BUFFER_SIZE,&pScrn->bitsPerPixel);
	pScrn->currentMode = calloc(1,sizeof(DisplayModeRec));
	pScrn->zoomLocked = TRUE;
	graphics_get_display_size(0, &pScrn->currentMode->HDisplay, &pScrn->currentMode->VDisplay );	
	pScrn->modes = xf86ModesAdd(pScrn->modes,pScrn->currentMode);
	//	intel_glamor_pre_init(pScrn);	
	INFO_MSG("PreInit Success");
	return TRUE;

fail:
	INFO_MSG("PreInit Failed");
	RPIFreeRec(pScrn);
	return FALSE;
}

static Bool RPIModeInit(ScrnInfoPtr pScrn, DisplayModePtr pMode)
{
	INFO_MSG("RPIModeInit");
	return TRUE;
}

static Bool RPICreateScreenResources( ScreenPtr pScreen )
{	
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum]; 
	INFO_MSG("RPICreateScreenResources");
	return TRUE;
}

void RPIPolyFillRect( DrawablePtr pDraw, GCPtr pGC, int nRects, xRectangle* rects)
{
}

void RPIPolyText16( DrawablePtr pDraw, GCPtr pGC, int x, int y, int count, char* chars )
{
}

static GCOps RPIGCOps = {
0,//	RPIFillSpans,
0,//	RPISetSpans,
miPutImage,//	RPIPutImage,
miCopyArea,//	RPICopyArea,
miCopyPlane,//	RPICopyPlane,
miPolyPoint,//	RPIPolyPoint,
0,//	RPIPolyLines,
miPolySegment,//	RPIPolySegment,
miPolyRectangle,//	RPIPolyRectangle,
miPolyArc,//	RPIPolyArc,
miFillPolygon,//	RPIFillPolygon,
	RPIPolyFillRect,
miPolyFillArc,//	RPIPolyFillArc,
miPolyText8,//	RPIPolyText8,
	RPIPolyText16,
miImageText8,//	RPIImageText8,
miImageText16,//	RPIImageText16,
miImageGlyphBlt,//	RPIImagGlyptBlt,
miPolyGlyphBlt,//	RPIPolyGlyphBlt,
miPushPixels,//	RPIPushPixels
};

void RPIChangeGC(GCPtr pGC, unsigned long mask)
{
}

void RPIValidateGC(GCPtr pGC, unsigned long changes, DrawablePtr pDraw )
{

}
/*
void RPICopyGC()
{
}
*/
void RPIDestroyGC( GCPtr pGC )
{
}
/*
void RPIChangeClip()
{
}
*/
void RPIDestroyClip( GCPtr pGC )
{
}
/*
void RPICopyClip()
{
}
*/
static GCFuncs RPIGCFuncs = {
	RPIValidateGC,
	RPIChangeGC,
miCopyGC,//	RPICopyGC,
	RPIDestroyGC,
miChangeClip,//	RPIChangeClip,
	RPIDestroyClip,
miCopyClip//	RPICopyClip
};
	
static Bool RPICreateGC( GCPtr pGC )
{
	pGC->ops = &RPIGCOps;
	pGC->funcs = &RPIGCFuncs;
	ScreenPtr pScreen = pGC->pScreen;	
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum]; 
	INFO_MSG("RPICreateGC");
	return TRUE;
}

void RPIQueryBestSize( int class, unsigned short* w, unsigned short* h, ScreenPtr pScreen )
{
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum]; 
	INFO_MSG("RPIQueryBestSize");
}

PixmapPtr RPICreatePixmap( ScreenPtr pScreen, int w, int h, int d, int hint )
{
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum]; 
	INFO_MSG("RPICreatePixmap");
/*
	PixmapPtr p = AllocatePixmap(pScreen,0);
	p->drawable.x = 0;
	p->drawable.y = 0;
	p->drawable.width = w;
	p->drawable.height = h;
	p->drawable.depth = d;
	p->drawable.type = DRAWABLE_PIXMAP;
	p->drawable.class = 0;
	p->drawable.bitsPerPixel = 32;
	p->drawable.id = 0;
	p->refcnt = 1;
	p->devKind = w * 4;
	p->usage_hint = hint;
	p->drawable.serialNumber = NEXT_SERIAL_NUMBER;	
	p->drawable.pScreen = pScreen;
#ifdef COMPOSITE
	p->screen_x = 0;
	p->screen_y = 0;
#endif
	p->devPrivates = 0;
	memset(&p->devPrivates,0,sizeof(DevUnion));
	return p;
*/
	return fbCreatePixmapBpp( pScreen, w, h, d, 32, hint );
}
 
void RPIGetImage( DrawablePtr pDraw, int sx, int sy, int w, int h, unsigned int format, unsigned long planemask, char* pdstLine )
{
}

void RPIDestroyPixmap( PixmapPtr p )
{
}

Bool RPICreateWindow( WindowPtr pWin )
{
	return FALSE;
}

void RPIPositionWindow( WindowPtr pWin, int x, int y )
{
}

Bool RPIDeviceCursorInitialize( DeviceIntPtr pDev, ScreenPtr pScreen )
{
	return FALSE;
}

static Bool RPIScreenInit(int scrnNum, ScreenPtr pScreen, int argc, char** argv )
{
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum]; 
	INFO_MSG("ScreenInit");

	INFO_MSG("Installing handlers");
	pScreen->CreateScreenResources = RPICreateScreenResources;
	pScreen->CreateGC = RPICreateGC;
	pScreen->QueryBestSize = RPIQueryBestSize;
	pScreen->CreatePixmap = RPICreatePixmap;
	pScreen->GetImage = RPIGetImage;
	pScreen->DestroyPixmap = RPIDestroyPixmap;
	pScreen->CreateWindow = RPICreateWindow;
	pScreen->PositionWindow = RPIPositionWindow;
	pScreen->DeviceCursorInitialize = RPIDeviceCursorInitialize;
/*
	pScreen->ChangeWindowAttributes = RPIChangeWindowAttributes;
	pScreen->ClipNotify = RPIClipNotify;
	pScreen->ConstrainCursor = RPIConstrainCursor;
	pScreen->CursorLimits = RPICursorLimits;
	pScreen->DestroyWindow = RPIDestroyWindow;
	pScreen->DisplayCursor = RPIDisplayCursor;
	pScreen->GetSpans = RPIGetSpans;
	pScreen->GetStaticColormap = RPIGetStaticColormap;
	pScreen->InstallColormap = RPIInstallColormap;
	pScreen->ListInstalledColormaps = RPIInstalledColormaps;
	pScreen->PointerNonInterestBox = RPIPointerNonInterestBox;
	pScreen->RealizeCursor = RPIRealizeCursor;
	pScreen->RealizeFont = RPIRealizeFont;
	pScreen->RealizeWindow = RPIRealizeWindow;
	pScreen->RecolorCursor = RPIRecolorCursor;
	pScreen->ResolveColor = RPIResolveColor;
	pScreen->SaveScreen = RPISaveScreen;
	pScreen->SetCursorPosition = RPISetCursorPosition;
	pScreen->StoreColors = RPIStoreColors;
	pScreen->UninstallColormap = RPIUninstallColormape;
	pScreen->UnrealizeCursor = RPIUnrealizeCursor;
	pScreen->UnrealizeFont = RPIUnrealizeFont;
	pScreen->UnrealizeWindow = RPIUnrealizeWindow;
*/	

	INFO_MSG("PictureInit");
	if( !PictureInit( pScreen, NULL, 0 ) )
	{
		ERROR_MSG("PictureInit failed");
		goto fail;
	}
	PictureSetSubpixelOrder(pScreen,SubPixelHorizontalRGB);

	if( !miInitVisuals(&pScreen->visuals,&pScreen->allowedDepths,&pScreen->numVisuals,&pScreen->numDepths,&pScreen->rootDepth,&pScreen->rootVisual,-1,8,0) )
	{
		ERROR_MSG("InitVisuals failed" );
		goto fail;
	}

	INFO_MSG("ScreenInit Success!");
	return TRUE;
fail:
	ERROR_MSG("ScreeInit Failed!");
	return FALSE;
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

