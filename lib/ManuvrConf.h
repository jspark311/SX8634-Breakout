/*
File:   ManuvrConf.h
Author: J. Ian Lindsay
Date:   2019.08.16


This is one of the files that the application author is required to provide.
This is where definition of (application or device)-specific parameters ought to go.
*/

#ifndef __MANUVR_FIRMWARE_DEFS_H
#define __MANUVR_FIRMWARE_DEFS_H

/*
* Particulars of this Manuvrable.
*/
#define MANUVR_DEBUG
#define MANUVR_EVENT_PROFILER

#define EVENT_MANAGER_PREALLOC_COUNT     8
#define SCHEDULER_MAX_SKIP_BEFORE_RESET  6
#define PLATFORM_RNG_CARRY_CAPACITY     32
#define WITH_MBEDTLS
#define MANUVR_CONSOLE_SUPPORT
#define MANUVR_STDIO
#define MANUVR_SUPPORT_I2C
  #define I2CADAPTER_MAX_QUEUE_DEPTH 20
  #define I2CADAPTER_PREALLOC_COUNT  8
#define MANUVR_STORAGE
#define MANUVR_CBOR
//#define MANUVR_JSON

/* Driver selection */
#define CONFIG_MANUVR_SX8634
  #define CONFIG_SX8634_PROVISIONING


// This is the string that identifies this Manuvrable to other Manuvrables. In MHB's case, this
//   will select the mEngine.
#define FIRMWARE_NAME     "sx8634-provisioner"

// This would be the version of the Manuvrable's firmware (this program).
#define VERSION_STRING    "0.1.1"

// Hardware is versioned. Manuvrables that are strictly-software should say -1 here.
#define HW_VERSION_STRING "1"


#endif  // __MANUVR_FIRMWARE_DEFS_H
