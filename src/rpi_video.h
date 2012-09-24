#ifndef __RPI_VIDEO_H__
#define __RPI_VIDEO_H__

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#define VERSION 0
#define RPI_NAME "RPI"         /* the name used to prefix messages */
#define RPI_DRIVER_NAME "rpi"  /* the driver name as used in config file */
#define RPI_MODULEVENDORSTRING "Adrian's RPI xf86 driver"
#define RPI_MAJOR_VERSION 0
#define RPI_MINOR_VERSION 0
#define RPI_PATCHLEVEL    1

#define INFO_MSG(fmt, ...) \
		do { xf86DrvMsg(pScrn->scrnIndex, X_INFO, fmt "\n",\
				##__VA_ARGS__); } while (0)
#define CONFIG_MSG(fmt, ...) \
		do { xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, fmt "\n",\
				##__VA_ARGS__); } while (0)
#define WARNING_MSG(fmt, ...) \
		do { xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "WARNING: " fmt "\n",\
				##__VA_ARGS__); } while (0)
#define ERROR_MSG(fmt, ...) \
		do { xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "ERROR: " fmt "\n",\
				##__VA_ARGS__); } while (0)
#define EARLY_ERROR_MSG(fmt, ...) \
		do { xf86Msg(X_ERROR, "ERROR: " fmt "\n",\
				##__VA_ARGS__); } while (0)

typedef enum {
	OPTION_HW_CURSOR,
	OPTION_NOACCEL
} RPIopts;

typedef struct {
	Bool noAccel;
	Bool hwCursor;
	unsigned char* fbmem;
	unsigned char* fbstart;	
	EntityInfoPtr EntityInfo;
	CloseScreenProcPtr CloseScreen;
	OptionInfoPtr Options;
   	EGLDisplay display;
   	EGLSurface surface;
   	EGLContext context;
} RPIRec, *RPIPtr, *FBDevPtr;

static void RPIIdentify(int);
static Bool RPIProbe(DriverPtr,int);
static const OptionInfoRec* RPIAvailableOptions(int,int);
static MODULESETUPPROTO(rpiSetup);
static Bool RPIPreInit(ScrnInfoPtr,int);
static Bool RPIModeInit(ScrnInfoPtr,DisplayModePtr);
static Bool RPIScreenInit(int, ScreenPtr, int, char** );
static Bool RPISwitchMode(int, DisplayModePtr, int);
static void RPIAdjustFrame(int, int, int, int); 
static Bool RPIEnterVT(int, int);
static void RPILeaveVT(int, int);
static void RPIFreeScreen(int, int);

#define RPIPTR(p) ((RPIPtr)((p)->driverPrivate))
#define FBDEVPTR(p) ((RPIPtr)((p)->driverPrivate))
#endif
