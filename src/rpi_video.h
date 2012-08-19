#ifndef __RPI_VIDEO_H__
#define __RPI_VIDEO_H__

#define VERSION 0
#define RPI_NAME "RPI"         /* the name used to prefix messages */
#define RPI_DRIVER_NAME "rpi"  /* the driver name as used in config file */
#define RPI_MAJOR_VERSION 0
#define RPI_MINOR_VERSION 0
#define RPI_PATCHLEVEL    1

#define VIDEOCORE_IV 12345
#define XF86_VERSION_CURRENT 123456790

typedef enum {
	OPTION_HW_CURSOR,
	OPTION_NOACCEL
} RPIopts;

typedef struct {
	Bool noAccel;
	Bool hwCursor;
	CloseScreenProcPtr CloseScreen;
	OptionInfoPtr Options;
} RPIRec, *RPIPtr;

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
#endif
