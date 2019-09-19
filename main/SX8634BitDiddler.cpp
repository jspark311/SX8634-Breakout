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

static uint8_t  pf_pins[8]; // The platform GPIO pins that match the SX8634 GPIO.
static uint8_t  pin_transition_values[8];
static uint32_t pin_transition_times[8];

const MessageTypeDef message_defs_list[] = {
  {  MANUVR_MSG_SX8634_BD_SVC_REQ,    MSG_FLAG_EXPORTABLE,  "SX8634_BD_SVC_REQ",    ManuvrMsg::MSG_ARGS_NONE }  //
};


void sx_gpio_0_isr() {
  uint8_t sxpin = 0;
  pin_transition_times[sxpin]  = micros();
  pin_transition_values[sxpin] = readPin(pf_pins[sxpin]) ? 1 : 0;
}

void sx_gpio_1_isr() {
  uint8_t sxpin = 1;
  pin_transition_times[sxpin]  = micros();
  pin_transition_values[sxpin] = readPin(pf_pins[sxpin]) ? 1 : 0;
}

void sx_gpio_2_isr() {
  uint8_t sxpin = 2;
  pin_transition_times[sxpin]  = micros();
  pin_transition_values[sxpin] = readPin(pf_pins[sxpin]) ? 1 : 0;
}

void sx_gpio_3_isr() {
  uint8_t sxpin = 3;
  pin_transition_times[sxpin]  = micros();
  pin_transition_values[sxpin] = readPin(pf_pins[sxpin]) ? 1 : 0;
}

void sx_gpio_4_isr() {
  uint8_t sxpin = 4;
  pin_transition_times[sxpin]  = micros();
  pin_transition_values[sxpin] = readPin(pf_pins[sxpin]) ? 1 : 0;
}

void sx_gpio_5_isr() {
  uint8_t sxpin = 5;
  pin_transition_times[sxpin]  = micros();
  pin_transition_values[sxpin] = readPin(pf_pins[sxpin]) ? 1 : 0;
}

void sx_gpio_6_isr() {
  uint8_t sxpin = 6;
  pin_transition_times[sxpin]  = micros();
  pin_transition_values[sxpin] = readPin(pf_pins[sxpin]) ? 1 : 0;
}

void sx_gpio_7_isr() {
  uint8_t sxpin = 7;
  pin_transition_times[sxpin]  = micros();
  pin_transition_values[sxpin] = readPin(pf_pins[sxpin]) ? 1 : 0;
}




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
SX8634BitDiddler::SX8634BitDiddler(I2CAdapter* i2c, uint8_t _pwr, uint8_t g0, uint8_t g1, uint8_t g2, uint8_t g3, uint8_t g4, uint8_t g5, uint8_t g6, uint8_t g7, const SX8634Opts* sx8634_o)
    : EventReceiver("SX8634BitDiddler"), _PWR_PIN(_pwr), touch(sx8634_o) {
  INSTANCE = this;
  pf_pins[0] = g0;
  pf_pins[1] = g1;
  pf_pins[2] = g2;
  pf_pins[3] = g3;
  pf_pins[4] = g4;
  pf_pins[5] = g5;
  pf_pins[6] = g6;
  pf_pins[7] = g7;
  _platform_gpio_make_safe();
  gpioDefine(_PWR_PIN, GPIOMode::OUTPUT);
  setPin(_PWR_PIN, true);       // Turn on power to the touch board.
  for (uint8_t i = 0; i < 8; i++) {
    pin_transition_times[i] = 0;
  }

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
  local_log.concat("Putting platform GPIO into testing mode.\n");

  _gpio_safety(false);
  for (uint8_t i = 0; i < 8; i++) {
    bool using_isr = false;
    GPIOMode pptm;
    switch (touch.getGPIOMode(i)) {
      case GPIOMode::ANALOG_OUT:
      case GPIOMode::OUTPUT:
        pptm = GPIOMode::INPUT;
        using_isr = true;
        break;
      case GPIOMode::INPUT:
      case GPIOMode::INPUT_PULLUP:
      case GPIOMode::INPUT_PULLDOWN:
        pptm = GPIOMode::OUTPUT;
        break;

      default:
        pptm = GPIOMode::INPUT_PULLUP;
        using_isr = true;
        break;
    }
    local_log.concatf("SX8634 pin %u has mode: %s.  Setting platform pin %u to mode: %s\n",
      i,
      Platform::getPinModeStr(touch.getGPIOMode(i)),
      pf_pins[i],
      Platform::getPinModeStr(pptm)
    );
    if (255 != pf_pins[i]) {
      if (using_isr) {
        unsetPinIRQ(pf_pins[i]);
      }
      gpioDefine(pf_pins[i], pptm);
      if (using_isr) {
        switch (i) {
          case 0:  setPinFxn(pf_pins[i], CHANGE_PULL_UP, sx_gpio_0_isr);  break;
          case 1:  setPinFxn(pf_pins[i], CHANGE_PULL_UP, sx_gpio_1_isr);  break;
          case 2:  setPinFxn(pf_pins[i], CHANGE_PULL_UP, sx_gpio_2_isr);  break;
          case 3:  setPinFxn(pf_pins[i], CHANGE_PULL_UP, sx_gpio_3_isr);  break;
          case 4:  setPinFxn(pf_pins[i], CHANGE_PULL_UP, sx_gpio_4_isr);  break;
          case 5:  setPinFxn(pf_pins[i], CHANGE_PULL_UP, sx_gpio_5_isr);  break;
          case 6:  setPinFxn(pf_pins[i], CHANGE_PULL_UP, sx_gpio_6_isr);  break;
          case 7:  setPinFxn(pf_pins[i], CHANGE_PULL_UP, sx_gpio_7_isr);  break;
        }
      }
    }
  }
  flushLocalLog();
  return 0;
}



int8_t SX8634BitDiddler::_platform_gpio_make_safe() {
  local_log.concat("Putting platform GPIO into INPUT_PULLUP mode.\n");

  for (uint8_t i = 0; i < 8; i++) {
    if (255 != pf_pins[i]) {
      gpioDefine(pf_pins[i], GPIOMode::INPUT_PULLUP);
      unsetPinIRQ(pf_pins[i]);
    }
  }
  _gpio_safety(true);
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
  printPins(output);
}


void SX8634BitDiddler::printPins(StringBuilder* output) {
  local_log.concat("SX8634BitDiddler platform pin assignments\n");
  local_log.concatf("GPIO safety:    %c\n", _gpio_safety() ? 'y':'n');
  local_log.concat("\nSX  PF   Val real micros\n-----------------------------------------\n");
  for (uint8_t i = 0; i < 8; i++) {
    local_log.concatf(
      "%u:  %u   %u    %u   %u\n",
      i,
      pf_pins[i],
      pin_transition_values[i],
      readPin(i)? 1:0,
      pin_transition_times[i]
    );
  }
  flushLocalLog();
}



#if defined(MANUVR_CONSOLE_SUPPORT)
/*******************************************************************************
* Console I/O
*******************************************************************************/

static const ConsoleCommand console_cmds[] = {
  { "i",    "Info" },
  { "i 1",  "SX8634 overview" },
  { "i 2",  "SX8634 GPIO listing" },
  { "i 3",  "SX8634 SPM" },
  { "i 4",  "Platform GPIO listing" },
  { "X/x",  "Enable/Disable power to the connected touch board" },
  { "t",    "Touch board info" },
  { "t 1",  "Set SX8634 to ACTIVE" },
  { "t 2",  "Set SX8634 to DOZE" },
  { "t 3",  "Set SX8634 to SLEEP" },
  { "t 4",  "Ping SX8634" },
  { "R",    "Reset SX8634" },
  { "G/g",  "Reconfigure/Safety the platform GPIO pins" },
  { "O/o",  "Set/Clear GPIO pin on touch board" },
  { "P/p",  "Set/Clear the value of a platform GPO pin" },
  { "S",    "Save current SPM to local storage" },
  { "d",    "Dump given SPM blob to console" },
  { "D",    "Drop given SPM blob from local storage" },
  { "L",    "Load stored SPM blob to SPM" },
  { "l",    "List stored SPM blobs" },
  { "c",    "Print an application config blob from the current SPM" },
  { "B",    "Burn current SPM to SX8634 NVM" }
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
          touch.printOverview(&local_log);
          break;
        case 2:
          touch.printGPIO(&local_log);
          break;
        case 3:
          touch.printSPMShadow(&local_log);
          break;
        case 4:
          printPins(&local_log);
          break;
        default:
          printDebug(&local_log);
          break;
      }
      break;

    /* Jig control options */
    case 'X':   // Power control
    case 'x':   // Power control
      setPin(_PWR_PIN, 'X' == c);
      local_log.concatf("Power %sabled.\n", (('X' == c) ? "En" : "Dis"));
      break;

    /* SX8634 control options */
    case 'O':   // Set GPO pin with optional value.
    case 'o':   // Clear GPO pin
      if (arg0_given && (0 <= arg0) & (8 > arg0)) {
        uint8_t pinval = ('O' == c) ? 255 : 0;
        if (arg1_given && (0 <= arg1) & (8 > arg1)) {
          pinval = arg1;
        }
        ret = touch.setGPOValue(arg0, pinval);
        local_log.concatf("touch.setGPOValue(%u, %u) returns %d\n", arg0, pinval, ret);
      }
      else {
        local_log.concatf("Usage: %c <SX8634 pin>", c);
      }
      break;

    case 'P':   // Set platform GPO pin
    case 'p':   // Clear platform GPO pin
      if (arg0_given && (0 <= arg0) & (8 > arg0)) {
        uint8_t pfpin = pf_pins[arg0];
        switch (touch.getGPIOMode(arg0)) {
          case GPIOMode::INPUT:
          case GPIOMode::INPUT_PULLUP:
          case GPIOMode::INPUT_PULLDOWN:
            setPin(pfpin, 'P' == c);
            local_log.concatf("Platform pin corrosponding to touch pin %u (%u) set to %s.\n", arg0, pfpin, ('P' == c) ? "high":"low");
            break;
          case GPIOMode::OUTPUT:
          case GPIOMode::ANALOG_OUT:
          default:
            local_log.concatf("Platform pin %u is an input.\n", pfpin);
            break;
        }
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

    case 'R':   // Reset the SX8634
      ret = touch.reset();
      local_log.concat("touch.reset()");
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
