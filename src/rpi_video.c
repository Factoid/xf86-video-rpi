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
#include <xf86cmap.h>
#include <fb.h>

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

   ErrorF( "n values = %i\n", nValues );
   for(int i = 0; i < nValues; ++i )
   {
      eglGetConfigAttrib(disp,config,values[i],&val);
      ErrorF("%s = %i\n", names[i], val);
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
	ErrorF("RPISave\n");
}

static Bool RPIPreInit(ScrnInfoPtr pScrn,int flags)
{
	ErrorF("Start PreInit\n");

	pScrn->monitor = pScrn->confScreen->monitor;

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

	int bpp;
	eglGetConfigAttrib(state->display,config,EGL_BUFFER_SIZE,&bpp);

	if( !xf86SetDepthBpp(pScrn,0,0,bpp,0) )
	{
		goto fail;
	}
	
	rgb c;
	c.red = 0;
	c.green = 0;
	c.blue = 0;
	if( !xf86SetWeight(pScrn,c,c) )
	{
		goto fail;
	}

	Gamma g;
	g.red = 0;
	g.green = 0;
	g.blue = 0;
	if( !xf86SetGamma(pScrn,g) )
	{
		goto fail;
	}

	// Should be auto-detecting TrueColor, doesn't...
	if( !xf86SetDefaultVisual(pScrn, TrueColor) )
	{
		goto fail;
	}

	pScrn->currentMode = calloc(1,sizeof(DisplayModeRec));
	pScrn->zoomLocked = TRUE;
	graphics_get_display_size(0, &pScrn->currentMode->HDisplay, &pScrn->currentMode->VDisplay );	
	pScrn->modes = xf86ModesAdd(pScrn->modes,pScrn->currentMode);
	//	intel_glamor_pre_init(pScrn);	

	ErrorF("PreInit Success\n");
	return TRUE;

fail:
	ErrorF("PreInit Failed\n");
	RPIFreeRec(pScrn);
	return FALSE;
}

static Bool RPIModeInit(ScrnInfoPtr pScrn, DisplayModePtr pMode)
{
	ErrorF("RPIModeInit\n");
	return TRUE;
}

void RPIPutImage( DrawablePtr pDraw, GCPtr pGC, int depth, int x, int y, int w, int h, int leftpad, int format, char* pBits )
{
	ErrorF("RPIPutImage\n");
}

void RPICopyArea( DrawablePtr pSrc, DrawablePtr pDest, GCPtr pGC, int srcx, int srcy, int w, int h, int destx, int desty )
{
	ErrorF("RPICopyArea\n");
}

void RPICopyPlane( DrawablePtr pSrc, DrawablePtr pDest, GCPtr pGC, int srcx, int srcy, int w, int h, int destx, int desty, unsigned long plane )
{
	ErrorF("RPICopyPlane\n");
}

void RPIPolyPoint( DrawablePtr pDraw, GCPtr pGC, int mode, int npt, DDXPointPtr pptInit )
{
	ErrorF("RPIPolyPoint\n");
}

void RPIPolyLines( DrawablePtr pDraw, GCPtr pGC, int mode, int npt, DDXPointPtr pptInit )
{
	ErrorF("RPIPolyPoint\n");
}

void RPIPolySegment( DrawablePtr pDraw, GCPtr pGC, int nSeg, xSegment* pSegs )
{
	ErrorF("RPIPolySegment\n");
}

void RPIPolyRectangle( DrawablePtr pDraw, GCPtr pGC, int nRecs, xRectangle* pRect )
{
	ErrorF("RPIPolyRectangle\n");
}

void RPIPolyArc( DrawablePtr pDraw, GCPtr pGC, int nArcs, xArc* pArcs )
{
	ErrorF("RPIPolyArc\n");
}

void RPIFillPolygon( DrawablePtr pDraw, GCPtr pGC, int shape, int mode, int count, DDXPointPtr pPts )
{
	ErrorF("RPIFillPolygon\n");
}

void RPIPolyFillRect( DrawablePtr pDraw, GCPtr pGC, int nRects, xRectangle* rects)
{
	ErrorF("RPIPolyFillRect\n");
}

void RPIPolyFillArc( DrawablePtr pDraw, GCPtr pGC, int nArcs, xArc* arcs )
{
	ErrorF("RPIPolyFillArc\n");
}

void RPIPolyText8( DrawablePtr pDraw, GCPtr pGC, int x, int y, int count, char* chars )
{
	ErrorF("RPIPolyText8\n");
}

void RPIPolyText16( DrawablePtr pDraw, GCPtr pGC, int x, int y, int count, unsigned short* chars )
{
	ErrorF("RPIPolyText16\n");
}

void RPIImageText8(DrawablePtr pDraw, GCPtr pGC, int x, int y, int count, char * chars )
{
	ErrorF("RPIImageText8\n");
}

void RPIImageText16( DrawablePtr pDraw, GCPtr pGC, int x, int y, int count, unsigned short * chars )
{
	ErrorF("RPIImageText16\n");
}

void RPIImageGlyphBlt( DrawablePtr pDraw, GCPtr pGC, int x, int y, unsigned int nglyph, CharInfoPtr * ppci, pointer pglyphBase )
{
	ErrorF("RPIImageGlyphBlt\n");
}

void RPIPolyGlyphBlt( DrawablePtr pDraw, GCPtr pGC, int x, int y, unsigned int nglyph, CharInfoPtr * ppci, pointer pglyphBase )
{
	ErrorF("RPIPolyGlyphBlt\n");
}

void RPIPushPixels( GCPtr pGC, PixmapPtr pPix, DrawablePtr pDraw, int w, int h, int x, int y )
{
	ErrorF("RPIPushPixels\n");
}

static GCOps RPIGCOps = {
0, //RPIFillSpans,
0, //RPISetSpans,
RPIPutImage,
RPICopyArea,
RPICopyPlane,
RPIPolyPoint,
RPIPolyLines,
RPIPolySegment,
RPIPolyRectangle,
RPIPolyArc,
RPIFillPolygon,
RPIPolyFillRect,
RPIPolyFillArc,
RPIPolyText8,
RPIPolyText16,
RPIImageText8,
RPIImageText16,
RPIImageGlyphBlt,
RPIPolyGlyphBlt,
RPIPushPixels
};

void RPIChangeGC(GCPtr pGC, unsigned long mask)
{
	ErrorF("RPIChangeGC\n");
}

void RPIValidateGC(GCPtr pGC, unsigned long changes, DrawablePtr pDraw )
{
	ErrorF("RPIValidateGC\n");
}
/*
void RPICopyGC()
{
}
*/
void RPIDestroyGC( GCPtr pGC )
{
	ErrorF("RPIDestroyGC\n");
}
/*
void RPIChangeClip()
{
}
*/
void RPIDestroyClip( GCPtr pGC )
{
	ErrorF("RPIDestroyClip\n");
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
	
Bool RPICloseScreen( int index, ScreenPtr pScreen )
{
	ErrorF("RPICloseScreen\n");

  DepthPtr depths = pScreen->allowedDepths;
  for( int i = 0; i < pScreen->numDepths; ++i )
  {
    free(depths[i].vids);
  }
  free(depths);
  free(pScreen->visuals);
  return TRUE;
}

// Electing not to borrow code from fbQueryBestSize for now
void RPIQueryBestSize( int class, unsigned short* w, unsigned short* h, ScreenPtr pScreen )
{
	ErrorF("RPIQueryBestSize\n");
}

Bool RPISaveScreen( ScreenPtr pScreen, int on )
{
	ErrorF("RPISaveScreen\n");
  return TRUE;
}

void RPIGetImage( DrawablePtr pDraw, int sx, int sy, int w, int h, unsigned int format, unsigned long planemask, char* pdstLine )
{
	ErrorF("RPIGetImage\n");
}

Bool RPICreateWindow( WindowPtr pWin )
{
	ErrorF("RPICreateWindow\n");
	return TRUE;
}

Bool RPIDestroyWindow( WindowPtr pWin )
{
  return TRUE;
}

Bool RPIPositionWindow( WindowPtr pWin, int x, int y )
{
	ErrorF("RPIPositionWindow\n");
  return TRUE;
}

Bool RPIChangeWindowAttributes( WindowPtr pWin, unsigned long mask )
{
	ErrorF("RPIChangeWindowAttributes\n");
  return TRUE;
}

Bool RPIRealizeWindow( WindowPtr pWin )
{
	ErrorF("RPIRealizeWindow\n");
  return TRUE;
}

Bool RPIUnrealizeWindow( WindowPtr pWin )
{
  ErrorF("RPIUnrealizeWindow\n");
  return TRUE;
}

int RPIValidateTree( WindowPtr pParent, WindowPtr pChild, VTKind vtk )
{
  ErrorF("RPIValidateTree\n");
  return miValidateTree(pParent, pChild, vtk);
}

void RPIWindowExposures( WindowPtr pWin, RegionPtr region, RegionPtr others )
{
	ErrorF("RPIWindowExposures\n");
  miWindowExposures(pWin,region,others);
}

PixmapPtr RPICreatePixmap( ScreenPtr pScreen, int w, int h, int d, int hint )
{
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum]; 
	ErrorF("RPICreatePixmap\n");
	return fbCreatePixmapBpp( pScreen, w, h, d, 32, hint );
}

Bool RPIDestroyPixmap( PixmapPtr p )
{
	ErrorF("RPIDestroyPixmap\n");
	return fbDestroyPixmap(p);
}

//miPointerConstrainCursor
void RPIConstrainCursor( DeviceIntPtr pDev, ScreenPtr pScreen, BoxPtr pBox )
{
	ErrorF("RPIConstrainCursor\n");
}

void RPICursorLimits( DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor, BoxPtr pHotbox, BoxPtr pTopLeft )
{
	ErrorF("RPICursorLimits\n");
}

Bool RPIDisplayCursor( DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor )
{
	ErrorF("RPIDisplayCursor\n");
  return TRUE;
}

Bool RPIRealizeCursor( DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor )
{
	ErrorF("RPIRealizeCursor\n");
  return TRUE;
}

Bool RPISetCursorPosition( DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y, Bool genEvent )
{
	ErrorF("RPISetCursorPosition\n");
  return TRUE;
}

static Bool RPICreateGC( GCPtr pGC )
{
	pGC->ops = &RPIGCOps;
	pGC->funcs = &RPIGCFuncs;
	ScreenPtr pScreen = pGC->pScreen;	
	ErrorF("RPICreateGC\n");
	return TRUE;
}

Bool RPICreateColormap( ColormapPtr pmap )
{
	ErrorF("RPICreateColormap\n");
  miInitializeColormap(pmap);
	return TRUE;
}

void RPIDestroyColormap( ColormapPtr pmap )
{
  ErrorF("RPIDestroyColormap\n");
}

void RPIInstallColormap( ColormapPtr pmap )
{
	ErrorF("RPIInstallColormap\n");
  miInstallColormap(pmap);
}

void RPIUninstallColormap( ColormapPtr pmap )
{
	ErrorF("RPIUninstallColormap\n");
  miUninstallColormap(pmap); 
}

int RPIListInstalledColormaps( ScreenPtr pScreen, XID* pmaps )
{
  return miListInstalledColormaps(pScreen,pmaps);
}

void RPIResolveColor( short unsigned int* pred, short unsigned int* pgreen, short unsigned int* pblue, VisualPtr pVisual )
{
	ErrorF("RPIResolveColor\n");
}

RegionPtr RPIBitmapToRegion( PixmapPtr pPixmap )
{
  return RegionCreate(NULL,1);
}

void RPIBlockHandler( int sNum, pointer bData, pointer pTimeout, pointer pReadmask )
{
//	ErrorF("RPIBlockHandler\n");
	struct timeval** tvpp = (struct timeval**)pTimeout;
	(*tvpp)->tv_sec = 0;
	(*tvpp)->tv_usec = 100;
}

void RPIWakeupHandler( int sNum, pointer wData, unsigned long result, pointer pReadmask )
{
//	ErrorF("RPIWakeupHandler\n");
}

static Bool RPICreateScreenResources( ScreenPtr pScreen )
{	
	ErrorF("RPICreateScreenResources\n");
  if( !miCreateScreenResources(pScreen) )
    return FALSE;

	return TRUE;
}

PixmapPtr RPIGetWindowPixmap( WindowPtr pWin )
{
	ErrorF("RPIGetWindowPixmap\n");
  // The pixmap for the root window is stored in it's screen devPrivate
  if( pWin->parent == 0 )
  {
    return pWin->drawable.pScreen->devPrivate;
  }

  return _fbGetWindowPixmap(pWin);
}

void RPISetWindowPixmap( WindowPtr pWin, PixmapPtr pPix )
{
	ErrorF("RPISetWindowPixmap\n");
  _fbSetWindowPixmap(pWin,pPix);
}

PixmapPtr RPIGetScreenPixmap( ScreenPtr pScreen )
{
	ErrorF("RPIGetScreenPixmap\n");
  return miGetScreenPixmap(pScreen);
//	return pScreen->PixmapPerDepth[0];
}

void RPISetScreenPixmap( PixmapPtr pPixmap )
{
	ErrorF("RPISetScreenPixmap\n");
  miSetScreenPixmap( pPixmap );
//  pScreen->PixmapPerDepth[0] = pPixmap;
}

void RPIMarkWindow( WindowPtr pWin )
{
	ErrorF("RPIMarkWindow\n");
  miMarkWindow(pWin);
}

void RPIHandleExposures( WindowPtr pWin )
{
	ErrorF("RPIHandleExposures\n");
}

Bool RPIDeviceCursorInitialize( DeviceIntPtr pDev, ScreenPtr pScreen )
{
	ErrorF("RPIDeviceCursorInitialize\n");
	return TRUE;
}

void RPILoadPalette( ScrnInfoPtr pScrn, int numColors, int *indicies, LOCO *colors, VisualPtr pVisual )
{
}

static Bool RPIScreenInit(int scrnNum, ScreenPtr pScreen, int argc, char** argv )
{
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];

  if( !fbAllocatePrivates(pScreen,NULL) )
  {
    ErrorF("Unable to allocate fb privates\n");
    goto fail;
  }
  pScreen->defColormap = FakeClientID(0);  
  ErrorF("RPIScreenInit\n");
	pScreen->CloseScreen = RPICloseScreen;
	pScreen->QueryBestSize = RPIQueryBestSize;
	pScreen->SaveScreen = RPISaveScreen;
	pScreen->GetImage = RPIGetImage;
  //pScreen->GetSpans = RPIGetSpans;
	//pScreen->SourceValidate = RPISourceValidate;

  pScreen->CreateWindow = RPICreateWindow;
	pScreen->DestroyWindow = RPIDestroyWindow;
	pScreen->PositionWindow = RPIPositionWindow;
	pScreen->ChangeWindowAttributes = RPIChangeWindowAttributes;
	pScreen->RealizeWindow = RPIRealizeWindow;
  pScreen->UnrealizeWindow = RPIUnrealizeWindow;
	pScreen->ValidateTree = RPIValidateTree;
  //pScreen->PostValidateTree = RPIPostValidateTree;
	pScreen->WindowExposures = RPIWindowExposures;
  //pScreen->CopyWindow = RPICopyWindow;
  //pScreen->ClearToBackground = RPIClearToBackground;
	//pScreen->ClipNotify = RPIClipNotify;
 	//pScreen->RestackWindow = RPIRestackWindow;

	pScreen->CreatePixmap = RPICreatePixmap;
	pScreen->DestroyPixmap = RPIDestroyPixmap;
 
	//pScreen->RealizeFont = RPIRealizeFont;
	//pScreen->UnrealizeFont = RPIUnrealizeFont;

	pScreen->ConstrainCursor = RPIConstrainCursor;
  //pScreen->ConstrainCursorHarder = RPIConstrainCursorHarder;
	pScreen->CursorLimits = RPICursorLimits;
	pScreen->DisplayCursor = RPIDisplayCursor;
	pScreen->RealizeCursor = RPIRealizeCursor;
	//pScreen->UnrealizeCursor = RPIUnrealizeCursor;
	//pScreen->RecolorCursor = RPIRecolorCursor;
	//pScreen->SetCursorPosition = RPISetCursorPosition;
	
  pScreen->CreateGC = RPICreateGC;
 
	pScreen->CreateColormap = RPICreateColormap;
	pScreen->DestroyColormap = RPIDestroyColormap;
  pScreen->InstallColormap = RPIInstallColormap;
	pScreen->UninstallColormap = RPIUninstallColormap;
	pScreen->ListInstalledColormaps = RPIListInstalledColormaps;
	//pScreen->StoreColors = RPIStoreColors;
	pScreen->ResolveColor = RPIResolveColor;
  
  pScreen->BitmapToRegion = RPIBitmapToRegion;
  //pScreen->SendGraphicsExpose

	pScreen->BlockHandler = RPIBlockHandler;
	pScreen->WakeupHandler = RPIWakeupHandler;
  
  pScreen->CreateScreenResources = RPICreateScreenResources;
	//pScreen->ModifyPixmapHeader

	pScreen->GetWindowPixmap = RPIGetWindowPixmap;
	pScreen->SetWindowPixmap = RPISetWindowPixmap;
	pScreen->GetScreenPixmap = RPIGetScreenPixmap;
	pScreen->SetScreenPixmap = RPISetScreenPixmap;

	pScreen->MarkWindow = RPIMarkWindow;
  //pScreen->MarkOverlappedWindows;
  //pScreen->ConfigNotify;
  //pScreen->MoveWindow;
  //pScreen->ResizeWindow;
  //pScreen->GetLayerWindow;
	pScreen->HandleExposures = RPIHandleExposures;
  //pScreen->ReparentWindow;

  //pScreen->SetShape;

  //pScreen->ChangeBorderWidth;
  //pScreen->MarkUnealizedWindow;

  pScreen->DeviceCursorInitialize = RPIDeviceCursorInitialize;
  //pScreen->DeviceCursorCleanup;
	
  ErrorF("PictureInit\n");
	if( !PictureInit( pScreen, NULL, 0 ) )
	{
		ErrorF("PictureInit failed\n");
		goto fail;
	}
	PictureSetSubpixelOrder(pScreen,SubPixelHorizontalRGB);

	miClearVisualTypes();
	if( !miSetVisualTypes(pScrn->depth,TrueColorMask,pScrn->rgbBits, TrueColor) )
	{
		ErrorF("SetVisualTypes failed\n");
		goto fail;
	}
	if( !miSetPixmapDepths() )
	{
		ErrorF("SetPixmapDepths failed\n");
		goto fail;
	}

	int rootDepth = 0;
	int numVisuals = 0;
	int numDepths = 0;
	VisualID defaultVis;	
	if( !miInitVisuals(&pScreen->visuals,&pScreen->allowedDepths,&numVisuals,&numDepths,&rootDepth,&defaultVis,1<<31,8,-1) )
	{
		ErrorF("InitVisuals failed\n" );
		goto fail;
	}
  if( !miScreenInit(pScreen, 0, pScrn->currentMode->HDisplay, pScrn->currentMode->VDisplay, 96, 96, 4, rootDepth, numDepths, pScreen->allowedDepths, defaultVis, numVisuals, pScreen->visuals ) )
  {
    ErrorF("ScreenInit failed\n");
    goto fail;
  }
	pScreen->numVisuals = numVisuals;
	pScreen->numDepths = numDepths;
	pScreen->rootDepth = rootDepth;
	
	for( int i = 0; i < pScreen->numDepths; ++i )
	{
		if( pScreen->allowedDepths[i].depth <= 0 || pScreen->allowedDepths[i].depth > 32 )
		{
			ErrorF("Validating depths failed %i, depth %i\n", i, pScreen->allowedDepths[i].depth );
			goto fail;
		}
	}

	if( !miDCInitialize(pScreen, xf86GetPointerScreenFuncs() ) )
	{
		ErrorF("SpriteInitialize failed\n");
		goto fail;
	}

	xf86DisableRandR();
	xf86SetBackingStore(pScreen);
	if( !miCreateDefColormap(pScreen) )
  {
    ErrorF("miCreateDefColormap failed\n");
    goto fail;
  }
//  int flags = CMAP_PALETTED_TRUECOLOR;
//  if( !xf86HandleColormaps(pScreen, 256, 8, NULL, NULL, flags) )
//  {
//    ErrorF("xf86HandleColormaps failed\n");
//    goto fail;
//  }

	ErrorF("ScreenInit Success\n");
	return TRUE;
fail:
	ErrorF("ScreeInit Failed\n");
	return FALSE;
}

static Bool RPISwitchMode(int scrnNum, DisplayModePtr pMode, int flags)
{
	ErrorF( "RPISwitchMode\n" );
	return TRUE;
}

static void RPIAdjustFrame(int scrnNum, int x, int y, int flags)
{
	ErrorF( "RPIAdjustFrame\n" );
}

static Bool RPIEnterVT(int scrnNum, int flags)
{
	ErrorF("RPIEnterVT\n");
	return TRUE;
}

static void RPILeaveVT(int scrnNum, int flags)
{
	ErrorF("RPILeaveVT\n" );
}

static void RPIFreeScreen(int scrnNum, int flags)
{
	ErrorF("RPIFreeScreen\n" );
}

