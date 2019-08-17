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
  local_log.concat("Putting platform GPIO into testing mode.\n");
  flushLocalLog();
  return 0;
}


int8_t SX8634BitDiddler::_platform_gpio_make_safe() {
  local_log.concat("Putting platform GPIO into INPUT_PULLUP mode.\n");
  if (255 != _GPIO0) {  gpioDefine(_GPIO0, GPIOMode::INPUT_PULLUP);  }
  if (255 != _GPIO1) {  gpioDefine(_GPIO1, GPIOMode::INPUT_PULLUP);  }
  if (255 != _GPIO2) {  gpioDefine(_GPIO2, GPIOMode::INPUT_PULLUP);  }
  if (255 != _GPIO3) {  gpioDefine(_GPIO3, GPIOMode::INPUT_PULLUP);  }
  if (255 != _GPIO4) {  gpioDefine(_GPIO4, GPIOMode::INPUT_PULLUP);  }
  if (255 != _GPIO5) {  gpioDefine(_GPIO5, GPIOMode::INPUT_PULLUP);  }
  if (255 != _GPIO6) {  gpioDefine(_GPIO6, GPIOMode::INPUT_PULLUP);  }
  if (255 != _GPIO7) {  gpioDefine(_GPIO7, GPIOMode::INPUT_PULLUP);  }
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
      return_value++;
      break;

    case MANUVR_MSG_USER_SLIDER_VALUE:
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
  { "t9",   "Force SX8634 re-init" },
  { "g",    "Safety the platform GPIO pins" },
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
  char c    = *str;
  bool arg0_given = false;
  bool arg1_given = false;
  bool arg2_given = false;
  int arg0  = 0;
  int arg1  = 0;
  int arg2  = 0;

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

    case 'O':   // Set GPO pin
    case 'o':   // Clear GPO pin
      if (arg0_given && (0 <= arg0) & (12 > arg0)) {
        touch.setGPOValue(arg0, ('O' == c) ? 255 : 0);
      }
      break;

    case 'p':   // Set GPO pin, exactly
      if (arg0_given && (0 <= arg0) & (12 > arg0)) {
        if (arg1_given) {
          touch.setGPOValue(arg0, arg1);
        }
      }
      break;

    case 't':   // Touch
      switch (arg0) {
        case 1:
          touch.setMode(SX8634OpMode::ACTIVE);
          break;
        case 2:
          touch.setMode(SX8634OpMode::DOZE);
          break;
        case 3:
          touch.setMode(SX8634OpMode::SLEEP);
          break;
        case 4:
          touch.ping();
          break;
        case 5:
          touch.reset();
          break;
        case 9:
          touch.init();
          break;
        default:
          touch.printDebug(&local_log);
          break;
      }
      break;

    case 'B':   // Burn current config to NVM
      touch.burn_nvm();
      break;

    case 'g':   // Safety all the platform GPIO pins.
      _platform_gpio_make_safe();
      break;

    default:
      break;
  }

  flushLocalLog();
}
#endif  //MANUVR_CONSOLE_SUPPORT
