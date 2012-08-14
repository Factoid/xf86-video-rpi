#ifndef __RPI_VIDEO_H__
#define __RPI_VIDEO_H__

#define VERSION 0
#define RPI_NAME "RPI"         /* the name used to prefix messages */
#define RPI_DRIVER_NAME "rpi"  /* the driver name as used in config file */
#define RPI_MAJOR_VERSION 0
#define RPI_MINOR_VERSION 0
#define RPI_PATCHLEVEL    1

#define VIDEOCORE_IV 12345
#define XF86_VERSION_CURRENT 12345

typedef enum {
    OPTION_HW_CURSOR,
    OPTION_NOACCEL
} RPIopts;

static void RPIIdentify(int);
static Bool RPIProbe(DriverPtr,int);
static const OptionInfoRec* RPIAvailableOptions(int,int);
static MODULESETUPPROTO(rpiSetup);

#endif
