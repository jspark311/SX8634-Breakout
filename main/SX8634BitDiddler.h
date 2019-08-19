/*
* Ganglion-A is equipped with two 40A SSRs and a four-channel I2C dimmer.
*/

#ifndef __SX8634_PROVISIONER_H__
#define __SX8634_PROVISIONER_H__

#include <inttypes.h>
#include <stdint.h>
#include <Platform/Platform.h>
#include <Drivers/SX8634/SX8634.h>


#define MANUVR_MSG_SX8634_BD_SVC_REQ  0x7C4F

#if !defined(MANUVR_CONSOLE_SUPPORT)
  #error SX8634BitDiddler is useless without MANUVR_CONSOLE_SUPPORT.
#endif

#if !defined(CONFIG_SX8634_PROVISIONING)
  #error SX8634BitDiddler is useless without CONFIG_SX8634_PROVISIONING.
#endif

#if defined(CONFIG_SX8634_CONFIG_ON_FAITH)
  #error SX8634BitDiddler's operation is not compatible with CONFIG_SX8634_CONFIG_ON_FAITH.
#endif

#if !defined(MANUVR_STORAGE)
  #error SX8634BitDiddler needs MANUVR_STORAGE to persist and load SPM blobs.
#endif


class SX8634BitDiddler : public EventReceiver, public ConsoleInterface {
  public:
    SX8634BitDiddler(I2CAdapter*, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, const SX8634Opts*);
    ~SX8634BitDiddler();

    /* Overrides from EventReceiver */
    int8_t notify(ManuvrMsg*);
    int8_t callback_proc(ManuvrMsg*);
    void printDebug(StringBuilder*);

    /* Overrides from ConsoleInterface */
    uint consoleGetCmds(ConsoleCommand**);
    inline const char* consoleName() { return "SX8634BitDiddler";  };
    void consoleCmdProc(StringBuilder* input);


  protected:
    int8_t attached();


  private:
    const uint8_t _GPIO0;  // The platform GPIO pins that match the SX8634 GPIO.
    const uint8_t _GPIO1;  // Used for automated testing.
    const uint8_t _GPIO2;
    const uint8_t _GPIO3;
    const uint8_t _GPIO4;
    const uint8_t _GPIO5;
    const uint8_t _GPIO6;
    const uint8_t _GPIO7;
    StringBuilder _blob_index;
    SX8634 touch;
    ManuvrMsg _msg_service_request;

    /* GPIO and automated testing functions */
    int8_t _platform_gpio_reconfigure();
    int8_t _platform_gpio_make_safe();

    int8_t _load_blob_by_name(const char*, uint8_t*);
    int8_t _save_blob_by_name(const char*, uint8_t*);
    int8_t _load_blob_directory();
    int8_t _save_blob_directory();
};


#endif   // __SX8634_PROVISIONER_H__
