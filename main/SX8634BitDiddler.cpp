#include "SX8634BitDiddler.h"
#include <Drivers/SX8634/SX8634.h>


/*******************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
*******************************************************************************/
static SX8634BitDiddler* INSTANCE = nullptr;

const MessageTypeDef message_defs_list[] = {
  {  MANUVR_MSG_SX8634_BD_SVC_REQ,    MSG_FLAG_EXPORTABLE,  "SX8634_BD_SVC_REQ",    ManuvrMsg::MSG_ARGS_NONE }  //
};


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

/*
* Constructor.
*/
SX8634BitDiddler::SX8634BitDiddler(I2CAdapter* i2c, uint8_t g0, uint8_t g1, uint8_t g2, uint8_t g3, uint8_t g4, uint8_t g5, uint8_t g6, uint8_t g7, const SX8634Opts* sx8634_o)
    : EventReceiver("SX8634BitDiddler"),
      _GPIO0(g0), _GPIO1(g1), _GPIO2(g2), _GPIO3(g3),
      _GPIO4(g4), _GPIO5(g5), _GPIO6(g6), _GPIO7(g7),
      touch(sx8634_o) {
  INSTANCE = this;
  _platform_gpio_make_safe();
  int mes_count = sizeof(message_defs_list) / sizeof(MessageTypeDef);
  ManuvrMsg::registerMessages(message_defs_list, mes_count);

  i2c->addSlaveDevice((I2CDevice*) &touch);
}


/*
* Destructor.
*/
SX8634BitDiddler::~SX8634BitDiddler() {
}



/*******************************************************************************
* Touch
*******************************************************************************/

int8_t SX8634BitDiddler::_platform_gpio_reconfigure() {
  const uint8_t* const _addrs[8] = {&_GPIO0, &_GPIO1, &_GPIO2, &_GPIO3, &_GPIO4, &_GPIO5, &_GPIO6, &_GPIO7};
  local_log.concat("Putting platform GPIO into testing mode.\n");

  for (uint8_t i = 0; i < 8; i++) {
    GPIOMode pptm;
    switch (touch.getGPIOMode(i)) {
      case GPIOMode::ANALOG_OUT:
      case GPIOMode::OUTPUT:
        pptm = GPIOMode::INPUT;

        break;
      case GPIOMode::INPUT:
      case GPIOMode::INPUT_PULLUP:
      case GPIOMode::INPUT_PULLDOWN:
        pptm = GPIOMode::OUTPUT;
        break;

      default:
        pptm = GPIOMode::INPUT_PULLUP;
        local_log.concatf("SX8634 pin %u has unhandled mode: %s\n", i, Platform::getPinModeStr(touch.getGPIOMode(i)));
        break;
    }
    local_log.concatf("SX8634 pin %u has mode: %s.  Setting platform pin %u to mode: %s\n",
      i,
      Platform::getPinModeStr(touch.getGPIOMode(i)),
      *_addrs[i],
      Platform::getPinModeStr(pptm)
    );
    if (255 != *_addrs[i]) {  gpioDefine(*_addrs[i], pptm);  }
  }
  flushLocalLog();
  return 0;
}



int8_t SX8634BitDiddler::_platform_gpio_make_safe() {
  const uint8_t* const _addrs[8] = {&_GPIO0, &_GPIO1, &_GPIO2, &_GPIO3, &_GPIO4, &_GPIO5, &_GPIO6, &_GPIO7};
  local_log.concat("Putting platform GPIO into INPUT_PULLUP mode.\n");

  for (uint8_t i = 0; i < 8; i++) {
    if (255 != *_addrs[i]) {  gpioDefine(*_addrs[i], GPIOMode::INPUT_PULLUP);  }
  }
  flushLocalLog();
  return 0;
}





/*******************************************************************************
* ######## ##     ## ######## ##    ## ########  ######
* ##       ##     ## ##       ###   ##    ##    ##    ##
* ##       ##     ## ##       ####  ##    ##    ##
* ######   ##     ## ######   ## ## ##    ##     ######
* ##        ##   ##  ##       ##  ####    ##          ##
* ##         ## ##   ##       ##   ###    ##    ##    ##
* ########    ###    ######## ##    ##    ##     ######
*
* These are overrides from EventReceiver interface...
*******************************************************************************/

/**
* This is called when the kernel attaches the module.
* This is the first time the class can be expected to have kernel access.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t SX8634BitDiddler::attached() {
  if (EventReceiver::attached()) {
    touch.init();
    return 1;
  }
  return 0;
}


/**
* If we find ourselves in this fxn, it means an event that this class built (the argument)
*   has been serviced and we are now getting the chance to see the results. The argument
*   to this fxn will never be NULL.
*
* Depending on class implementations, we might choose to handle the completed Event differently. We
*   might add values to event's Argument chain and return RECYCLE. We may also free() the event
*   ourselves and return DROP. By default, we will return REAP to instruct the Kernel
*   to either free() the event or return it to it's preallocate queue, as appropriate. If the event
*   was crafted to not be in the heap in its own allocation, we will return DROP instead.
*
* @param  event  The event for which service has been completed.
* @return A callback return code.
*/
int8_t SX8634BitDiddler::callback_proc(ManuvrMsg* event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = (0 == event->refCount()) ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->eventCode()) {
    case MANUVR_MSG_SX8634_BD_SVC_REQ:
    default:
      break;
  }

  return return_value;
}


int8_t SX8634BitDiddler::notify(ManuvrMsg* active_event) {
  int8_t return_value = 0;
  uint8_t val0 = 0;

  switch (active_event->eventCode()) {
    case MANUVR_MSG_SX8634_BD_SVC_REQ:
      return_value++;
      break;

    case MANUVR_MSG_USER_BUTTON_PRESS:
      if (0 == active_event->getArgAs(&val0)) {
        local_log.concatf("Button press %u\n", val0);
        switch (val0) {
          case 0:
            break;
          case 1:
            break;
          case 2:
            break;
          case 3:
            break;
          case 4:
            break;
          case 5:
            break;
          case 6:
            break;
          case 7:
            break;
          case 8:
            break;
          case 9:
            break;
          case 10:
            break;
          case 11:
            break;
        }
      }
      return_value++;
      break;

    case MANUVR_MSG_USER_BUTTON_RELEASE:
      if (0 == active_event->getArgAs(&val0)) {
        local_log.concatf("Button release %u\n", val0);
      }
      return_value++;
      break;

    case MANUVR_MSG_GPI_CHANGE:
      if (0 == active_event->getArgAs(&val0)) {
        local_log.concatf("GPI%u is now state %u\n", val0, touch.getGPIOValue(val0));
      }
      return_value++;
      break;

    case MANUVR_MSG_USER_SLIDER_VALUE:
      local_log.concatf("Slider: %u\n", touch.sliderValue());
      return_value++;
      break;

    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }

  flushLocalLog();
  return return_value;
}


/*
* Dump this item to the dev log.
*/
void SX8634BitDiddler::printDebug(StringBuilder* output) {
  EventReceiver::printDebug(output);
}



#if defined(MANUVR_CONSOLE_SUPPORT)
/*******************************************************************************
* Console I/O
*******************************************************************************/

static const ConsoleCommand console_cmds[] = {
  { "i",    "Info" },
  { "t",    "Touch board info" },
  { "t1",   "Set SX8634 to ACTIVE" },
  { "t2",   "Set SX8634 to DOZE" },
  { "t3",   "Set SX8634 to SLEEP" },
  { "t4",   "Ping SX8634" },
  { "t5",   "Reset SX8634" },
  { "S",    "Save current SPM to local storage" },
  { "D",    "Dump given SPM blob to console" },
  { "D",    "Drop given SPM blob from local storage" },
  { "L",    "Load stored SPM blob to SPM" },
  { "l",    "List stored SPM blobs" },
  { "c",    "Print an application config blob from the current SPM" },
  { "G/g",  "Reconfigure/Safety the platform GPIO pins" },
  { "B",    "Burn selected config to SX8634 NVM" },
  { "O/o",  "Set/Clear GPIO pin on touch board" },
  { "p",    "Set the value of a GPP/GPO pin" }
};


uint SX8634BitDiddler::consoleGetCmds(ConsoleCommand** ptr) {
  *ptr = (ConsoleCommand*) &console_cmds[0];
  return sizeof(console_cmds) / sizeof(ConsoleCommand);
}


void SX8634BitDiddler::consoleCmdProc(StringBuilder* input) {
  const char* str = (char *) input->position(0);
  char c          = *str;
  bool arg0_given = false;
  bool arg1_given = false;
  bool arg2_given = false;
  int8_t ret      = 0;
  int arg0        = 0;
  int arg1        = 0;
  int arg2        = 0;

  if (input->count() > 1) {
    // If there is a second token, we proceed on good-faith that it's an int.
    arg0 = input->position_as_int(1);
    arg0_given = true;
  }
  if (input->count() > 2) {
    // If there is a second token, we proceed on good-faith that it's an int.
    arg1 = input->position_as_int(2);
    arg1_given = true;
  }
  if (input->count() > 3) {
    // If there is a second token, we proceed on good-faith that it's an int.
    arg2 = input->position_as_int(3);
    arg2_given = true;
  }

  switch (c) {
    case 'i':   // Debug prints.
      switch (arg0) {
        case 1:
          break;
        default:
          printDebug(&local_log);
          break;
      }
      break;

    /* SX8634 control options */
    case 'O':   // Set GPO pin
    case 'o':   // Clear GPO pin
      if (arg0_given && (0 <= arg0) & (8 > arg0)) {
        ret = touch.setGPOValue(arg0, ('O' == c) ? 255 : 0);
        local_log.concatf("touch.setGPOValue(%u, %u) returns %d\n", arg0, (('O' == c) ? 255 : 0), ret);
      }
      else {
        local_log.concatf("Usage: %c <SX8634 pin>", c);
      }
      break;

    case 'p':   // Set GPO pin, exactly
      if (arg0_given && arg1_given && (0 <= arg0) & (8 > arg0)) {
        ret = touch.setGPOValue(arg0, arg1);
        local_log.concatf("touch.setGPOValue(%u, %u) returns %d\n", arg0, arg1, ret);
      }
      else {
        local_log.concatf("Usage: %c <SX8634 pin> <desired value>", c);
      }
      break;


    case 't':   // Touch
      switch (arg0) {
        case 1:
          ret = touch.setMode(SX8634OpMode::ACTIVE);
          local_log.concat("touch.setMode(ACTIVE)");
          break;
        case 2:
          ret = touch.setMode(SX8634OpMode::DOZE);
          local_log.concat("touch.setMode(DOZE)");
          break;
        case 3:
          ret = touch.setMode(SX8634OpMode::SLEEP);
          local_log.concat("touch.setMode(SLEEP)");
          break;
        case 4:
          ret = touch.ping();
          local_log.concat("touch.ping()");
          break;
        case 5:
          ret = touch.reset();
          local_log.concat("touch.reset()");
          break;
        default:
          touch.printDebug(&local_log);
          break;
      }
      if (0 != ret) {
        local_log.concatf(" operation returns %d\n", ret);
      }
      else {
        local_log.concat("\n");
      }
      break;

    case 'B':   // Burn current config to NVM
      ret = touch.burn_nvm();
      local_log.concatf("burn_nvm() returns %d\n", ret);
      break;

    /* Options involving platform GPIO */
    case 'G':   // Reconfigure all the platform GPIO pins.
    case 'g':   // Safety all the platform GPIO pins.
      ret = ('G' == c) ? _platform_gpio_reconfigure() : _platform_gpio_make_safe();
      local_log.concatf("Platform GPIO operation returns %d\n", ret);
      break;


    /* Options to save, load, and process SPM blobs from platform storage. */
    case 'S':  // Save current SPM to local storage
      if (arg0_given) {
        const char* name = input->position(1);
        uint8_t buf[128];
        memset(buf, 0, 128);
        if (0 == touch.copy_spm_to_buffer(buf)) {
          if (0 == _save_blob_by_name(name, buf)) {
            local_log.concatf("Saved SPM to blob \"%s\".\n", name);
          }
        }
      }
      else {
        local_log.concatf("Usage: %c <blob name>", c);
      }
      break;

    case 'd':  // Dump given SPM blob to console
      if (arg0_given) {
        const char* name = input->position(1);
        uint8_t buf[128];
        if (0 == _load_blob_by_name(name, buf)) {
          StringBuilder::printBuffer(&local_log, buf, 128, "");
          local_log.concat("\n\n");
        }
      }
      else {
        local_log.concatf("Usage: %c <blob name>", c);
      }
      break;

    case 'D':  // Drop given SPM blob from local storage
      if (arg0_given) {
      }
      else {
        local_log.concatf("Usage: %c <blob name>", c);
      }
      break;

    case 'L':  // Load stored SPM blob to SPM
      if (arg0_given) {
        const char* name = input->position(1);
        uint8_t buf[128];
        if (0 == _load_blob_by_name(name, buf)) {
          if (0 == touch.load_spm_from_buffer(buf)) {
            local_log.concatf("SPM loaded from stored blob \"%s\".", name);
          }
        }
      }
      else {
        local_log.concatf("Usage: %c <blob name>", c);
      }
      break;

    case 'l':  // List stored SPM blobs
      local_log.concatf("Existing SPM blobs:%s\n", PRINT_DIVIDER_1_STR);
      for (uint8_t i = 0; i < _blob_index.count(); i++) {
        local_log.concatf("\t %u: %s\n", i, _blob_index.position(i));
      }
      break;

    case 'c':  // Print an application config blob from the current SPM.
      {
        uint8_t buf[128];
        memset(buf, 0, 128);
        if (0 == touch.copy_spm_to_buffer(buf)) {
          if (0 == SX8634::render_stripped_spm(buf)) {
            local_log.concatf("Application config blob:%s\n", PRINT_DIVIDER_1_STR);
            StringBuilder::printBuffer(&local_log, buf, 97, "");
            local_log.concat("\n\n");
          }
        }
      }
      break;

    default:
      break;
  }
  flushLocalLog();
}
#endif  //MANUVR_CONSOLE_SUPPORT


/*
* buf is assumed to be 128-bytes long.
*/
int8_t SX8634BitDiddler::_load_blob_by_name(const char* name, uint8_t* buf) {
  int8_t ret = -3;
  int len = strlen(name);
  if ((3 < len) && (16 > len)) {
    ret++;
    Storage* store = platform.fetchStorage("");
    if (nullptr != store) {
      ret++;
      int rlen = store->persistentRead(name, buf, 128, 0);

      if (128 == rlen) {
        ret++;
      }
      else {
        local_log.concatf("Trying to read SPM blob \"%s\" was the wrong size (%d).\n", name, rlen);
      }
    }
    else {
      local_log.concat("No storage available.\n");
    }
  }
  else {
    local_log.concat("blob name must be between 4 and 15 characters.\n");
  }
  return ret;
}


/*
* buf is assumed to be 128-bytes long.
*/
int8_t SX8634BitDiddler::_save_blob_by_name(const char* name, uint8_t* buf) {
  int8_t ret = -3;
  int len = strlen(name);
  if ((3 < len) && (16 > len)) {
    ret++;
    Storage* store = platform.fetchStorage("");
    if (nullptr != store) {
      ret++;
      int rwri = store->persistentWrite(name, buf, 128, 0);

      if (128 == rwri) {
        ret++;
      }
      else {
        local_log.concatf("Trying to write SPM blob \"%s\" was the wrong size (%d).\n", name, rwri);
      }
    }
    else {
      local_log.concat("No storage available.\n");
    }
  }
  else {
    local_log.concat("blob name must be between 4 and 15 characters.\n");
  }
  return ret;
}


/*
* buf is assumed to be 128-bytes long.
*/
int8_t SX8634BitDiddler::_load_blob_directory() {
  int8_t ret = -2;
  Storage* store = platform.fetchStorage("");
  if (nullptr != store) {
    ret++;
    uint8_t buf[1024];
    memset(buf, 0, sizeof(buf));
    int rlen = store->persistentRead("idx", buf, 128, 0);

    if (0 < rlen) {
      ret++;
      _blob_index.clear();
      _blob_index.concat(buf, rlen);
      _blob_index.split("^!^");
    }
    else {
      local_log.concat("Tried to read SPM blob index and failed.\n");
    }
  }
  else {
    local_log.concat("No storage available.\n");
  }
  return ret;
}


/*
* buf is assumed to be 128-bytes long.
*/
int8_t SX8634BitDiddler::_save_blob_directory() {
  int8_t ret = -2;
  Storage* store = platform.fetchStorage("");
  if (nullptr != store) {
    ret++;
    uint8_t buf[1024];
    memset(buf, 0, sizeof(buf));
    if (0 < _blob_index.count()) {
      _blob_index.implode("^!^");
    }
    int wlen = _blob_index.length();
    int rlen = store->persistentWrite("idx", buf, wlen, 0);
    _blob_index.split("^!^");

    if (rlen == wlen) {
      ret++;
    }
    else {
      local_log.concat("Tried to write SPM blob index and failed.\n");
    }
  }
  else {
    local_log.concat("No storage available.\n");
  }
  return ret;
}
