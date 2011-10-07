/* DEFINITIONS SHARED BETWEEN USER SPACE AND KERNEL SPACE */

#ifndef _MODULEDEF_H_
#define _MODULEDEF_H_

#include <linux/ioctl.h>

#ifdef __KERNEL__
#include <linux/comedilib.h>
#endif


// *** KERNEL LOGGING MODE ***

// #define RTMODULE_DEBUG
// #define RTMODULE_INFO


// *** FEATURES ***

// #define ENABLE_TRIGGER
// #define ENABLE_TTLPULSE


// *** DEVICE LINUX CONFIGURATION ***

#define RTMODULE_MAJOR 227


// *** DECLARATION OF CONSTANTS ***

//* String length definitions:

// (one byte reserved for null-termination!)
#define PARAM_NAME_MAXLEN 128
#define DEV_NAME_MAXLEN   128


//* default waiting time for neuron to react to injected current
//* #define INJECT_RECORD_DELAY 5000 //nsec 
#define INJECT_RECORD_DELAY 1000 //nsec 


//* maximum supported dynamic clamp frequency ensuring a stable system
#define MAX_FREQUENCY 50000 //Hz


//* Data buffering:

#define FIFO_SIZE   500000 // bytes

//* DAQ-devices:

#define MAXDEV       4
#define MAXSUBDEV    8
#define MAXCHANLIST  128
#define MAXTTLPULSES 5

#define PARAM_CHAN_OFFSET 1000

//* Integration algorithms:

#define EULER       0
#define MIDPOINT    1
#define RK4         2
#define ALGO_PRESET EULER


// *** TYPE DEFINITIONS ***

//* DAQ-devices:

enum subdevTypes { SUBDEV_IN=0, SUBDEV_OUT, SUBDEV_DIO };

struct deviceIOCT {
  unsigned int subdevID;
  char devicename[DEV_NAME_MAXLEN+1];
  unsigned int subdev;
  enum subdevTypes subdevType;
  unsigned int fifoIndex;
  unsigned int fifoSize;
};

#define MAX_CONVERSION_COEFFICIENTS 4
struct converterT {
  double coefficients[MAX_CONVERSION_COEFFICIENTS];
  double expansion_origin;
  unsigned order;
};

struct chanlistIOCT {
  unsigned int subdevID;
  struct converterT conversionlist[MAXCHANLIST];
  float scalelist[MAXCHANLIST];
  unsigned int chanlist[MAXCHANLIST];
  int userDeviceIndex;
  unsigned int chanlistN;
};

struct comediCmdIOCT {
  unsigned int subdevID;
  comedi_cmd cmd;
};

struct syncCmdIOCT {
  unsigned int subdevID;
  unsigned int frequency;
  unsigned long delay;
  unsigned long duration;
  int startsource;
  int continuous;
};

enum dioOps { DIO_CONFIGURE, DIO_READ, DIO_WRITE, DIO_ADD_TTLPULSE, DIO_CLEAR_TTLPULSE };
enum ttlPulses { TTL_START_WRITE=0, TTL_END_WRITE, TTL_START_READ, TTL_END_READ, TTL_UNDEFINED };

struct dioIOCT {
  unsigned int subdevID;
  int bitfield;    /* if true, then treat lines and output as bit-fields. */
  enum dioOps op;
  int lines;
  int output;
  enum ttlPulses pulseType; /* only for op == DIO_ADD_TTLPULSE or DIO_CLEAR_TTLPULSE*/
};

struct triggerIOCT {
  char devname[DEV_NAME_MAXLEN+1];
  int subdev;   // -1: assing the first analog input subdevice.
  unsigned int channel;
  float alevel;
};


//* Trace-data:
enum traceTypes { TRACE_IN, TRACE_OUT, PARAM_IN, PARAM_OUT };

struct traceInfoIOCT {
  enum traceTypes traceType;
  char name[PARAM_NAME_MAXLEN];
  char unit[PARAM_NAME_MAXLEN];
};

struct traceChannelIOCT {
  enum traceTypes traceType;
  int device;
  int channel;
};


// *** IOCTL DEFINITIONS ***

// Give information to user space:

// control devices:

#define IOC_GET_SUBDEV_ID       _IOR(RTMODULE_MAJOR,  1, int)
#define IOC_OPEN_SUBDEV         _IOWR(RTMODULE_MAJOR,  2, int)
#define IOC_CHANLIST            _IOW(RTMODULE_MAJOR,  3, int)
#define IOC_COMEDI_CMD          _IOW(RTMODULE_MAJOR,  4, int)
#define IOC_SYNC_CMD            _IOW(RTMODULE_MAJOR,  5, int)
#define IOC_START_SUBDEV        _IOW(RTMODULE_MAJOR,  6, int)
#define IOC_CHK_RUNNING         _IOWR(RTMODULE_MAJOR, 7, int)
#define IOC_REQ_READ            _IOW(RTMODULE_MAJOR,  8, int)
#define IOC_REQ_WRITE           _IOW(RTMODULE_MAJOR,  9, int)
#define IOC_REQ_CLOSE           _IOW(RTMODULE_MAJOR, 10, int)
#define IOC_STOP_SUBDEV         _IOW(RTMODULE_MAJOR, 11, int)
#define IOC_RELEASE_SUBDEV      _IOW(RTMODULE_MAJOR, 12, int)

#define IOC_DIO_CMD             _IOWR(RTMODULE_MAJOR, 13 ,int)
#define IOC_SET_TRIGGER         _IOW(RTMODULE_MAJOR, 14, int)
#define IOC_UNSET_TRIGGER       _IOW(RTMODULE_MAJOR, 15, int)

// exchange info:

#define IOC_GET_TRACE_INFO      _IOWR(RTMODULE_MAJOR, 16, int)
#define IOC_SET_TRACE_CHANNEL   _IOW(RTMODULE_MAJOR,  17, int)
#define IOC_GETRATE             _IOR(RTMODULE_MAJOR,  18, int)
#define IOC_GETLOOPCNT          _IOR(RTMODULE_MAJOR,  19, int)
#define IOC_GETAOINDEX          _IOR(RTMODULE_MAJOR,  20, int)

#define RTMODULE_IOC_MAXNR 21


// *** KERNEL LOGGING STYLE ***

#ifdef __KERNEL__
#  define ERROR_MSG(msg, args...) printk( KERN_ERR "rtmodule: " msg, ## args )
#else
#  define ERROR_MSG(msg, args...) fprintf( stderr, msg, ## args )
#endif

#ifdef __KERNEL__
#  define WARN_MSG(msg, args...) printk( KERN_WARNING "rtmodule: " msg, ## args )
#else
#  define WARN_MSG(msg, args...) fprintf( stderr, msg, ## args )
#endif

#ifdef RTMODULE_INFO
#  ifdef __KERNEL__
#    define INFO_MSG(msg, args...) printk( "rtmodule: " msg, ## args )
#  else
#    define INFO_MSG(msg, args...) fprintf( stderr, msg, ## args )
#  endif
#else
#  define INFO_MSG(msg, args...)
#endif

#ifdef RTMODULE_DEBUG
#  ifdef __KERNEL__
#    define DEBUG_MSG(msg, args...) printk( "rtmodule: " msg, ## args )
#  else
#    define DEBUG_MSG(msg, args...) fprintf( stderr, msg, ## args )
#  endif
#else
#  define DEBUG_MSG(msg, args...)
#endif

#ifdef __KERNEL__
#  define SDEBUG_MSG(msg, args...) printk( "rtmodule: " msg, ## args )
#else
#  define SDEBUG_MSG(msg, args...) fprintf( stderr, msg, ## args )
#endif


#endif
