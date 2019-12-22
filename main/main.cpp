/**
* SX8634 provisioner
*
* This is Ian's tool for provisioning and testing Manuvr's touch boards. It is
*   being provided to inform the efforts of intrepid users who want to burn a
*   customized configuration to their board's non-volatile memory (NVM).
*
* This process is only needed if...
*   1) The GPIO pins are used as inputs, and the circuit can't abide pin contention.
*   2) The default settings are in some other way inadequate and instant-on is desired.
*
* If these cases don't apply to a given application, the boards can be used as-is.
* The only thing special about this program are the build-time options...
* #define CONFIG_MANUVR_SX8634         // Needed in any application using the touch board.
* #define CONFIG_SX8634_PROVISIONING   // Only needed to provision.
*
* A finished application would use CONFIG_MANUVR_SX8634, and possibly also
*   CONFIG_SX8634_CONFIG_ON_FAITH.
*
* CONFIG_SX8634_CONFIG_ON_FAITH causes the driver to skip the step of reading
*   the configuration from the chip on every boot. During this time, the chip
*   will be unresponsive to touch, and most applications will already know what
*   the loaded configuration will be. Using this option requires the config to
*   be provided to the driver during instantiation. The program will fail at
*   runtime if this configuration is not provided.
*
* Burning the NVM requires that the part be run a little bit hotter than the
*   rest of the system (3.6v). So the hardware needs to respect that fact also.
*   Trying to burn the NVM outside of this voltage range doesn't appear to cause
*   any damage, but it won't actually burn the NVM, and the chip won't tell you
*   why it failed. So it's on the user to ensure the voltage is adequate.
* The provisioning hardware level-shifts the GPIO pins for the sake of testing
*   the boards.
*
* Pins
* -------------
* IO33   SX8634 reset pin via level-shifter
* IO35   SX8634 IRQ pin via level-shifter with 3.3v pullup
* IO32   SCL
* IO25   SDA
*
* IO23   3.6v regulator enable line
* IO13   SX8634 GPIO0 via level-shifter
* IO14   SX8634 GPIO1 via level-shifter
* IO27   SX8634 GPIO2 via level-shifter
* IO26   SX8634 GPIO3 via level-shifter
* IO18   SX8634 GPIO4 via level-shifter
* IO19   SX8634 GPIO5 via level-shifter
* IO22   SX8634 GPIO6 via level-shifter
* IO21   SX8634 GPIO7 via level-shifter
*/

#include <math.h>

#include <Platform/Platform.h>
#include <Platform/Peripherals/I2C/I2CAdapter.h>
#include <XenoSession/Console/ManuvrConsole.h>
#include <Drivers/SX8634/SX8634.h>
#include "SX8634BitDiddler.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "rom/ets_sys.h"
#include "soc/dport_reg.h"
#include "soc/io_mux_reg.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_sig_map.h"
#include "rom/gpio.h"
#include "driver/gpio.h"

#include "esp_system.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"

#include "esp_task_wdt.h"
#include "esp_err.h"
#include "esp_event.h"


const I2CAdapterOptions i2c_opts(
  0,   // Device number
  25,  // sda
  32,  // scl
  0,   // No pullups.
  400000
);

/*
* If we care to setup the SX8634 differently than it comes shipped, we need to
*   provide a binary blob containing the parameters we want. Since this setup
*   takes time, and has implications for circuit safety, this blob ought to be
*   provisioned into the NVM once it is finalized for a given application.
* Only non-reserved values should be placed in the blob (97 bytes). Specifying
*   reserved bytes will have undefined (possibly disasterous) results. Nor
*   should the CRC value be supplied.
* If the NVM has already been written with the desired data, or the existing
*   configuration is the desired one, then this blob, and its reference in the
*   SX8634Opts, can be omitted and class will accept the existing config.
*/
const uint8_t sx8634_conf[97] = {
  0x2b, 0x02, 0x0E, 0x05, 0x01, 0xaa, 0xa5, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0x00, 0x30,
  0x50, 0x50, 0x01, 0x0a, 0x00, 0x00, 0x00, 0x03, 0x01, 0x80, 0x50, 0x50, 0x01, 0x02,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xcd, 0x30, 0x00, 0x00, 0x3f, 0x02,

  0x56, 0xA0, // GPIO mode
  0x00, // pwrup state
  0x00, // autolight
  0x00, // polarity
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // GPIO intensity ON
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // GPIO intensity OFF
  0x00, // GPIO fxn
  0x00, // Increment factor
  0x00, // Decrement factor
  0x00, 0x00, 0x00, 0x00, // Increment time
  0x44, 0x40, 0x00, 0x00, // Decrement time
  0x00, 0x00, 0x00, 0x00, // Off delay
  0x00, 0x00, // Pull resistors
  0x01, 0x70, // Interrupt
  0x00, // Debounce
  0x74
};


/*
* The given i2c address will be used in preference to the value given in the
*   config blob. So give the address that the driver should use. Not the one the
*   driver ought to provision the boards with.
*/
const SX8634Opts sx8634_opts(
  SX8634_DEFAULT_I2C_ADDR,   // i2c addr
  33,     // Reset pin
  17,     // IRQ pin
  nullptr // sx8634_conf
);


/*******************************************************************************
* Main thread                                                                  *
*******************************************************************************/

void manuvr_task(void* pvParameter) {
  Kernel* kernel = platform.kernel();
  unsigned long ms_0 = millis();
  unsigned long ms_1 = ms_0;
  bool odd_even = false;

  I2CAdapter i2c(&i2c_opts);
  kernel->subscribe(&i2c);

  SX8634BitDiddler provisioner(&i2c, 23, 13, 14, 27, 26, 18, 19, 22, 21, &sx8634_opts);
  kernel->subscribe(&provisioner);

  while (1) {
    kernel->procIdleFlags();
    ms_1 = millis();
    kernel->advanceScheduler(ms_1 - ms_0);
    ms_0 = ms_1;
    odd_even = !odd_even;
    if (0 == kernel->procIdleFlags()) {
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
  }
}



/*******************************************************************************
* Main function                                                                *
*******************************************************************************/
void app_main() {
  /*
  * The platform object is created on the stack, but takes no action upon
  *   construction. The first thing that should be done is to call the preinit
  *   function to setup the defaults of the platform.
  */
  platform.platformPreInit();
  platform.bootstrap();

  xTaskCreate(manuvr_task, "_manuvr", 32768, NULL, (tskIDLE_PRIORITY + 2), NULL);
}

#ifdef __cplusplus
}
#endif
