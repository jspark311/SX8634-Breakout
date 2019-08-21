/*
File:   MCP3918.cpp
Author: J. Ian Lindsay
*/

#include "SX8634.h"
#if defined(CONFIG_MANUVR_SX8634)


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

volatile static SX8634* INSTANCE = nullptr;

/* Offsets of off-limits values in the SPM */
static const uint8_t _reserved_spm_offsets[31] = {
  0x00, 0x01, 0x02, 0x03, 0x08, 0x20, 0x2A, 0x31, 0x32, 0x55, 0x6A, 0x6B, 0x6C,
  0x6D, 0x6E, 0x6F, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A,
  0x7B, 0x7C, 0x7D, 0x7E, 0x7F
};


const char* SX8634::getModeStr(SX8634OpMode x) {
  switch (x) {
    case SX8634OpMode::ACTIVE:   return "ACTIVE";
    case SX8634OpMode::DOZE:     return "DOZE";
    case SX8634OpMode::SLEEP:    return "SLEEP";
    default:                     return "UNDEF";
  }
}

const char* SX8634::getFSMStr(SX8634_FSM x) {
  switch (x) {
    case SX8634_FSM::NO_INIT:    return "NO_INIT";
    case SX8634_FSM::RESETTING:  return "RESETTING";
    case SX8634_FSM::SPM_READ:   return "SPM_READ";
    case SX8634_FSM::SPM_WRITE:  return "SPM_WRITE";
    case SX8634_FSM::READY:      return "READY";
    case SX8634_FSM::NVM_BURN:   return "NVM_BURN";
    case SX8634_FSM::NVM_VERIFY: return "NVM_VERIFY";
    default:                     return "UNDEF";
  }
}

/*
* Given the 128-byte input buffer, strip all the values that are either
*   reserved or readonly. The new buffer is written back into the input buffer.
* Length of valid data in the buffer after function runs is always 97-bytes.
*/
int8_t SX8634::render_stripped_spm(uint8_t* buf) {
  if (nullptr == buf) {
    uint8_t rsvd_idx  = 0;
    uint8_t given_idx = 0;
    for (uint8_t i = 0; i < 128; i++) {
      if (i == _reserved_spm_offsets[rsvd_idx]) {
        // Skip the copy. Increment the reserved pointer.
        rsvd_idx++;
      }
      else {
        // This is a desired config byte.
        if (buf[i] != buf[given_idx]) {
          buf[i] = buf[given_idx];
        }
        given_idx++;
      }
    }
  }
  return 0;
}


/*******************************************************************************
* .-. .----..----.    .-.     .--.  .-. .-..----.
* | |{ {__  | {}  }   | |    / {} \ |  `| || {}  \
* | |.-._} }| .-. \   | `--./  /\  \| |\  ||     /
* `-'`----' `-' `-'   `----'`-'  `-'`-' `-'`----'
*
* Interrupt service routine support functions. Everything in this block
*   executes under an ISR. Keep it brief...
*******************************************************************************/



/*
* This is an ISR.
*/
void sx8634_isr() {
  ((SX8634*)INSTANCE)->read_irq_registers();
}


int8_t SX8634::read_irq_registers() {
  if (!_sx8634_flag(SX8634_FLAG_IRQ_INHIBIT)) {
    // If IRQ service is not inhibited, read all the IRQ-related
    //   registers in one shot.
    I2CBusOp* nu = _bus->new_op(BusOpcode::RX, this);
    if (nullptr != nu) {
      nu->dev_addr = _opts.i2c_addr;
      nu->sub_addr = 0x00;
      nu->buf      = _registers;
      nu->buf_len  = 10;
      _bus->queue_io_job(nu);
      return 0;
    }
  }
  return -1;
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
* Constructor
*/
SX8634::SX8634(const SX8634Opts* o) : I2CDevice(o->i2c_addr), _opts{o} {
  INSTANCE = this;
  _clear_registers();
  _slider_msg.repurpose(MANUVR_MSG_USER_SLIDER_VALUE);
  _slider_msg.incRefs();
  _ll_pin_init();
}

/*
* Destructor
*/
SX8634::~SX8634() {
}



/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

int8_t SX8634::io_op_callahead(BusOp* _op) {
  return 0;
}


int8_t SX8634::io_op_callback(BusOp* _op) {
  I2CBusOp* completed = (I2CBusOp*) _op;
  #if defined(CONFIG_SX8634_DEBUG)
    StringBuilder output;
  #endif

  switch (completed->get_opcode()) {
    case BusOpcode::TX_CMD:
      _sx8634_clear_flag(SX8634_FLAG_PING_IN_FLIGHT);
      _sx8634_set_flag(SX8634_FLAG_DEV_FOUND, (!completed->hasFault()));
      if (!completed->hasFault()) {
        Kernel::log("SX8634 found\n");
        #if defined(CONFIG_SX8634_CONFIG_ON_FAITH)
          // Depending on build parameters, we might not read the SPM.
          if (nullptr != _opts.conf) {
            // Copy over the conf and cheat the state machine.
            _sx8634_set_flag(SX8634_FLAG_SPM_SHADOWED);
            _set_fsm_position(SX8634_FSM::READY);
            uint8_t rsvd_idx  = 0;
            uint8_t given_idx = 0;
            for (uint8_t i = 0; i < sizeof(_spm_shadow); i++) {
              if (i == _reserved_spm_offsets[rsvd_idx]) {
                // Skip the comparison. Increment the reserved pointer.
                rsvd_idx++;
              }
              else {
                // This is a comparable config byte.
                if (_spm_shadow[i] != *(_opts.conf + given_idx)) {
                  _spm_shadow[i] = *(_opts.conf + given_idx);
                }
                given_idx++;
              }
            }
            _class_state_from_spm();
          }
        #else
          _read_full_spm();
        #endif   // CONFIG_SX8634_CONFIG_ON_FAITH
      }
      else {
        Kernel::log("SX8634 not found\n");
      }
      break;

    case BusOpcode::TX:
      switch (completed->sub_addr) {
        case SX8634_REG_COMP_OP_MODE:
          _sx8634_set_flag(SX8634_FLAG_COMPENSATING, (*(completed->buf) & 0x04));
          break;
        case SX8634_REG_GPO_CTRL:
          break;
        case SX8634_REG_GPP_PIN_ID:
        case SX8634_REG_GPP_INTENSITY:
          // Update our local shadow to reflect the new GPP state.
          // SX8634_REG_GPP_INTENSITY always follows.
          if (_is_valid_pin(*(completed->buf))) {
            _gpo_levels[*(completed->buf)] = *(completed->buf + 1);
          }
          _sx8634_clear_flag(SX8634_FLAG_PWM_CHANGE_IN_FLIGHT);
          _proc_waiting_pwm_changes();
          break;
        case SX8634_REG_SPM_CONFIG:
          _sx8634_set_flag(SX8634_FLAG_SPM_WRITABLE, (0x00 == (*(completed->buf) & 0x08)));
          _sx8634_set_flag(SX8634_FLAG_SPM_OPEN, (0x10 == (*(completed->buf) & 0x30)));
          _set_shadow_reg_val(SX8634_REG_SPM_BASE_ADDR, 0);
          if (_sx8634_flag(SX8634_FLAG_SPM_OPEN)) {
            _write_register(SX8634_REG_SPM_BASE_ADDR, 0x00);
          }
          break;
        case SX8634_REG_SPM_BASE_ADDR:
          if (SX8634_FSM::NVM_BURN == _fsm) {
            #if defined(CONFIG_SX8634_PROVISIONING)
              // We are about to burn the NVM.
              _set_fsm_position(SX8634_FSM::READY);
              switch (*(completed->buf)) {
                case 0xA5:
                  #if defined(CONFIG_SX8634_DEBUG)
                    Kernel::log("0xA5 --> SPM_BASE_ADDR\n");
                  #endif
                  _write_register(SX8634_REG_SPM_BASE_ADDR, 0x5A);
                  break;
                case 0x5A:
                  #if defined(CONFIG_SX8634_DEBUG)
                    Kernel::log("0x5A --> SPM_BASE_ADDR\n");
                  #endif
                  break;
              }
            #endif // CONFIG_SX8634_PROVISIONING
          }
          else {
            if (_sx8634_flag(SX8634_FLAG_SPM_OPEN)) {
              // If the SPM is open, and this register was just written, we take
              //   the next step and read or write 8 bytes at the address.
              if (_sx8634_flag(SX8634_FLAG_SPM_WRITABLE)) {
                // Write the next 8 bytes if needed.
                _write_block8(_get_shadow_reg_val(SX8634_REG_SPM_BASE_ADDR));
              }
              else {
                // Read the next 8 bytes if needed.
                _read_block8(_get_shadow_reg_val(SX8634_REG_SPM_BASE_ADDR));
              }
            }
          }
          break;

        #if defined(CONFIG_SX8634_PROVISIONING)
          case SX8634_REG_SPM_KEY_MSB:
            #if defined(CONFIG_SX8634_DEBUG)
              Kernel::log("0x62 --> SPM_KEY_MSB\n");
            #endif
            _write_register(SX8634_REG_SPM_KEY_LSB, 0x9D);
            break;
          case SX8634_REG_SPM_KEY_LSB:
            #if defined(CONFIG_SX8634_DEBUG)
              Kernel::log("0x9D --> SPM_KEY_LSB\n");
            #endif
            _write_register(SX8634_REG_SPM_BASE_ADDR, 0xA5);
            break;
        #endif // CONFIG_SX8634_PROVISIONING

        case SX8634_REG_SOFT_RESET:
          switch (*(completed->buf)) {
            case 0x00:
              _reset_callback();
              break;
            case 0xDE:
              _write_register(SX8634_REG_SOFT_RESET, 0x00);
              break;
            default:
              break;
          }
          break;

        case SX8634_REG_IRQ_SRC:     // These registers are all read-only.
          if (_sx8634_flag(SX8634_FLAG_SPM_OPEN)) {
            int spm_base_addr = _get_shadow_reg_val(SX8634_REG_SPM_BASE_ADDR);
            if (spm_base_addr <= 0x78) {
              // We're writing our shadow of the SPM. Write the next base address.
              _wait_for_reset(30);
              _write_register(SX8634_REG_SPM_BASE_ADDR, spm_base_addr + 8);
            }
            else {
              _sx8634_clear_flag(SX8634_FLAG_SPM_DIRTY);
              _close_spm_access();
            }
          }
        case SX8634_REG_CAP_STAT_MSB:
        case SX8634_REG_CAP_STAT_LSB:
        case SX8634_REG_SLIDER_POS_MSB:
        case SX8634_REG_SLIDER_POS_LSB:
        case SX8634_REG_RESERVED_0:
        case SX8634_REG_RESERVED_1:
        case SX8634_REG_GPI_STAT:
        case SX8634_REG_SPM_STAT:
        case SX8634_REG_RESERVED_2:
        default:
          break;
      }
      break;

    case BusOpcode::RX:
      switch (completed->sub_addr) {
        case SX8634_REG_IRQ_SRC:
          if (_sx8634_flag(SX8634_FLAG_SPM_OPEN)) {
            int spm_base_addr = _get_shadow_reg_val(SX8634_REG_SPM_BASE_ADDR);
            if (spm_base_addr <= 0x78) {
              // We're shadowing the SPM. Write the next base address.
              _wait_for_reset(30);
              _write_register(SX8634_REG_SPM_BASE_ADDR, spm_base_addr + 8);
            }
            else {
              _sx8634_set_flag(SX8634_FLAG_SPM_SHADOWED);
              _class_state_from_spm();
              if (1 == _compare_config()) {
                // Since we apparently want to make changes to the SPM, we enter
                //   that state and start the write operation.
                _wait_for_reset(30);
                _write_full_spm();
              }
              else {
                // If we won't be writing SPM config
                _close_spm_access();
                _set_fsm_position(SX8634_FSM::READY);
              }
            }
          }
          else {
            // We're going to process an IRQ.
            /*  0x00:  IRQ_SRC
                0x01:  CapStatMSB
                0x02:  CapStatLSB
                0x03:  SliderPosMSB
                0x04:  SliderPosLSB
                0x05:  <reserved>
                0x06:  <reserved>
                0x07:  GPIStat
                0x08:  SPMStat
                0x09:  CompOpMode      */
            bool first_irq = _sx8634_flag(SX8634_FLAG_INITIAL_IRQ_READ);
            if (first_irq || (0x01 & _registers[0])) {  // Operating mode interrupt
              SX8634OpMode current = (SX8634OpMode) (_registers[9] & 0x04);
              _sx8634_set_flag(SX8634_FLAG_COMPENSATING, (_registers[9] & 0x04));
              if (current != _mode) {
                #if defined(CONFIG_SX8634_DEBUG)
                  output.concatf("-- SX8634 is now in mode %s\n", getModeStr(current));
                #endif   // CONFIG_SX8634_DEBUG
                _mode = current;
              }
            }
            if (0x02 & _registers[0]) {  // Compensation completed
              _compensations++;
            }
            if (0x04 & _registers[0]) {  // Button interrupt
              uint16_t current = (((uint16_t) (_registers[1] & 0x0F)) << 8) | ((uint16_t) _registers[2]);
              if (current != _buttons) {
                #if defined(CONFIG_SX8634_DEBUG)
                  output.concatf("-- Buttons: %u\n", current);
                #endif
                // Bitshift the button values into discrete messages.
                uint16_t diff = current ^ _buttons;
                for (uint8_t i = 0; i < 12; i++) {
                  if (diff & 0x01) {
                    _send_button_event(i, (0x01 & (current >> i)));
                  }
                  diff = diff >> 1;
                }
                _buttons = current;
              }
            }
            if (0x08 & _registers[0]) {  // Slider interrupt
              _sx8634_set_flag(SX8634_FLAG_SLIDER_TOUCHED,   (_registers[1] & 0x10));
              _sx8634_set_flag(SX8634_FLAG_SLIDER_MOVE_DOWN, (_registers[1] & 0x20));
              _sx8634_set_flag(SX8634_FLAG_SLIDER_MOVE_UP,   (_registers[1] & 0x40));
              uint16_t current = (((uint16_t) _registers[3]) << 8) | ((uint16_t) _registers[4]);
              if (current != _slider_val) {
                #if defined(CONFIG_SX8634_DEBUG)
                  output.concatf("-- Slider: %u\n", current);
                #endif
                _slider_val = current;
                _send_slider_event();  // Send slider value.
              }
            }
            if (first_irq || (0x10 & _registers[0])) {  // GPI interrupt
              _process_gpi_change(_registers[7]);
            }
            if (0x20 & _registers[0]) {  // SPM stat interrupt
            }
            _sx8634_set_flag(SX8634_FLAG_CONF_IS_NVM, (_registers[8] & 0x08));
            _nvm_burns = (_registers[8] & 0x07);
            if (0x40 & _registers[0]) {  // NVM burn interrupt
              // Burn appears to have completed. Enter the verify phase.
              _set_fsm_position(SX8634_FSM::READY);
              Kernel::log("-- SX8634 NVM burn completed.\n");
            }
          }
          break;

        case SX8634_REG_CAP_STAT_MSB:
        case SX8634_REG_CAP_STAT_LSB:
        case SX8634_REG_SLIDER_POS_MSB:
        case SX8634_REG_SLIDER_POS_LSB:
        case SX8634_REG_RESERVED_0:
        case SX8634_REG_RESERVED_1:
          break;
        case SX8634_REG_GPI_STAT:
          _process_gpi_change(*(completed->buf));
          break;
        case SX8634_REG_SPM_STAT:
          // Right now, for simplicity's sake, the driver reads all 10 IRQ-related
          //   registers in one shot and updates everything.
          break;

        case SX8634_REG_COMP_OP_MODE:
        case SX8634_REG_GPO_CTRL:
          break;

        case SX8634_REG_GPP_PIN_ID:
        case SX8634_REG_GPP_INTENSITY:
          // TODO: Update our local shadow to reflect the new GPP state.
          // SX8634_REG_GPP_INTENSITY always follows.
          break;
        case SX8634_REG_SPM_BASE_ADDR:
        case SX8634_REG_SPM_KEY_MSB:
        case SX8634_REG_SPM_KEY_LSB:
          break;
        case SX8634_REG_RESERVED_2:
        case SX8634_REG_SOFT_RESET:
        default:
          break;
      }
      break;

    default:
      break;
  }

  #if defined(CONFIG_SX8634_DEBUG)
    if (output.length() > 0) Kernel::log(&output);
  #endif
  return 0;
}



/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void SX8634::printDebug(StringBuilder* output) {
  output->concatf("Touch sensor (SX8634)\t%s%s\n", getModeStr(operationalMode()), PRINT_DIVIDER_1_STR);
  output->concatf("-- Found:          %c\n", (deviceFound() ? 'y':'n'));
  output->concatf("-- IRQ Inhibit:    %c\n", (_sx8634_flag(SX8634_FLAG_IRQ_INHIBIT) ? 'y': 'n'));
  output->concatf("-- PWM sync'd:     %c\n", (_sx8634_flag(SX8634_FLAG_PWM_CHANGE_IN_FLIGHT) ? 'n': 'y'));
  output->concatf("-- Compensations:  %u\n", _compensations);
  output->concatf("-- FSM Position:   %s\n", getFSMStr(_fsm));

  output->concat("--\n-- Registers:\n--\t  ");
  StringBuilder::printBuffer(output, _registers, sizeof(_registers), "--\t  ");
  output->concat("--\n-- SPM/NVM:\n");

  output->concatf("--\tConf source:    %s\n", (_sx8634_flag(SX8634_FLAG_CONF_IS_NVM) ? "NVM" : "QSM"));
  output->concatf("--\tSPM shadowed:   %c\n", (_sx8634_flag(SX8634_FLAG_SPM_SHADOWED) ? 'y': 'n'));
  if (_sx8634_flag(SX8634_FLAG_SPM_SHADOWED)) {
    StringBuilder::printBuffer(output, _spm_shadow, sizeof(_spm_shadow), "--\t  ");
    output->concatf("--\tSPM Dirty:      %c\n", (_sx8634_flag(SX8634_FLAG_SPM_DIRTY) ? 'y': 'n'));
  }
  output->concatf("--\tSPM open:       %c\n", (_sx8634_flag(SX8634_FLAG_SPM_OPEN) ? 'y': 'n'));
  if (_sx8634_flag(SX8634_FLAG_SPM_OPEN)) {
    output->concatf("--\tSPM writable:   %c\n", (_sx8634_flag(SX8634_FLAG_SPM_WRITABLE) ? 'y': 'n'));
  }
  output->concatf("--\tNVM burns:      %u\n", _nvm_burns);

  output->concat("--\n-- GPIO:");
  output->concat(PRINT_DIVIDER_1_STR);
  for (uint8_t i = 0; i < 8; i++) {
    output->concatf("--\t%u:\t%u\t%s\n", i, getGPIOValue(i), Platform::getPinModeStr(getGPIOMode(i)));
  }
}



/*******************************************************************************
* Shadow register access functions
*******************************************************************************/

int8_t SX8634::_get_shadow_reg_mem_addr(uint8_t addr) {
  uint8_t maximum_idx = 15;
  switch (addr) {
    case SX8634_REG_IRQ_SRC:         maximum_idx--;
    case SX8634_REG_CAP_STAT_MSB:    maximum_idx--;
    case SX8634_REG_CAP_STAT_LSB:    maximum_idx--;
    case SX8634_REG_SLIDER_POS_MSB:  maximum_idx--;
    case SX8634_REG_SLIDER_POS_LSB:  maximum_idx--;
    case SX8634_REG_GPI_STAT:        maximum_idx--;
    case SX8634_REG_SPM_STAT:        maximum_idx--;
    case SX8634_REG_COMP_OP_MODE:    maximum_idx--;
    case SX8634_REG_GPO_CTRL:        maximum_idx--;
    case SX8634_REG_GPP_PIN_ID:      maximum_idx--;
    case SX8634_REG_GPP_INTENSITY:   maximum_idx--;
    case SX8634_REG_SPM_CONFIG:      maximum_idx--;
    case SX8634_REG_SPM_BASE_ADDR:   maximum_idx--;
    case SX8634_REG_SPM_KEY_MSB:     maximum_idx--;
    case SX8634_REG_SPM_KEY_LSB:     maximum_idx--;
    case SX8634_REG_SOFT_RESET:
      return (maximum_idx & 0x0F);
    case SX8634_REG_RESERVED_0:
    case SX8634_REG_RESERVED_1:
    case SX8634_REG_RESERVED_2:
    default:
      break;
  }
  return -1;
}


uint8_t SX8634::_get_shadow_reg_val(uint8_t addr) {
  int idx = _get_shadow_reg_mem_addr(addr);
  return (idx >= 0) ? _registers[idx] : 0;
}


void SX8634::_set_shadow_reg_val(uint8_t addr, uint8_t val) {
  int idx = _get_shadow_reg_mem_addr(addr);
  if (idx >= 0) {
    _registers[idx] = val;
  }
}


void SX8634::_set_shadow_reg_val(uint8_t addr, uint8_t* buf, uint8_t len) {
  int idx = _get_shadow_reg_mem_addr(addr);
  if ((idx + len) <= sizeof(_registers)) {
    memcpy(&_registers[idx], buf, len);
  }
}


int8_t SX8634::_write_register(uint8_t addr, uint8_t val) {
  int idx = _get_shadow_reg_mem_addr(addr);
  if (idx >= 0) {
    _registers[idx] = val;
    I2CBusOp* nu = _bus->new_op(BusOpcode::TX, this);
    if (nullptr != nu) {
      nu->dev_addr = _opts.i2c_addr;
      nu->sub_addr = addr;
      nu->buf      = &_registers[idx];
      nu->buf_len  = 1;
      return _bus->queue_io_job(nu);
    }
  }
  return -1;
}


/*******************************************************************************
* Class-specific functions
*******************************************************************************/

/*
* Reset the chip. By hardware pin (if possible) or by software command.
* TODO: Gernealize delay.
*/
int8_t SX8634::reset() {
  int8_t ret = -1;
  if (_opts.haveIRQPin()) {
    _sx8634_set_flag(SX8634_FLAG_IRQ_INHIBIT);
  }
  _flags = 0;
  _clear_registers();
  _set_fsm_position(SX8634_FSM::RESETTING);

  if (_opts.haveResetPin()) {
    setPin(_opts.reset_pin, false);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    setPin(_opts.reset_pin, true);
    ret = _reset_callback();
  }
  else {
    ret = _write_register(SX8634_REG_SOFT_RESET, 0xDE);
  }
  return ret;
}


/*
* We split this apart from the reset function so that it could be called from
*   either the pin-case or the software-case.
*/
int8_t SX8634::_reset_callback() {
  int8_t ret = _wait_for_reset(300);
  if (0 == ret) {
    _sx8634_clear_flag(SX8634_FLAG_IRQ_INHIBIT);
    _sx8634_set_flag(SX8634_FLAG_INITIAL_IRQ_READ);
    ret = ping_device() ? 0 : -2;
  }
  return ret;
}


int8_t SX8634::setMode(SX8634OpMode m) {
  return _write_register(SX8634_REG_COMP_OP_MODE, (uint8_t) m);
}


/*******************************************************************************
* GPIO functions
*******************************************************************************/

/*
* TODO: Finish this.
* Changing the mode of a GPIO pin requires that we have the SPM shadowed, and
*   that we take the chip out of operation for a moment to re-write a dirty
*   SPM. This is not nearly as fast as changing the pin mode on a platform pin,
*   since it involves a few hundred bytes of i2c traffic spread over at least
*   12 operations. Make sure your circuit can tolerate all modes, or write the
*   desired config into the NVM permanently.
*/
int8_t SX8634::setGPIOMode(uint8_t pin, GPIOMode m) {
  if (pin < 8) {
    if (_sx8634_flag(SX8634_FLAG_SPM_SHADOWED)) {
      switch (m) {
        case GPIOMode::ANALOG_OUT:
        case GPIOMode::OUTPUT:
        case GPIOMode::INPUT:
        case GPIOMode::INPUT_PULLUP:
        case GPIOMode::INPUT_PULLDOWN:
        default:
          return -3;
      }
    }
    return -2;
  }
  return -1;
}


/*
* Returns the GPIO pin mode.
*/
GPIOMode SX8634::getGPIOMode(uint8_t pin) {
  if (pin < 8) {
    if (_sx8634_flag(SX8634_FLAG_SPM_SHADOWED)) {
      uint8_t mode_val = _spm_shadow[(pin > 3) ? 64 : 65];
      uint8_t pull_val = _spm_shadow[(pin > 3) ? 101 : 102];
      switch (0x03 & (mode_val >> (2 * ((pin > 3) ? (pin-4) : pin)))) {
        case 0:
          return GPIOMode::OUTPUT;
        case 1:
          return GPIOMode::ANALOG_OUT;
        case 2:
          switch (0x03 & (pull_val >> (2 * ((pin > 3) ? (pin-4) : pin)))) {
            case 0:    return GPIOMode::INPUT;
            case 1:    return GPIOMode::INPUT_PULLUP;
            case 2:    return GPIOMode::INPUT_PULLDOWN;
            case 3:    return GPIOMode::INPUT;   // This is a reserved case...
          }
        case 3:
          return GPIOMode::UNINIT;
      }
    }
  }
  // Without the SPM, or with an out-of-range pin, who can say?
  return GPIOMode::UNINIT;
}


/*
* If the pin is an input, returns the last-know state of the pin.
* If the pin is an output, returns the last-confirmed write to the pin.
*/
uint8_t SX8634::getGPIOValue(uint8_t pin) {
  pin = pin & 0x07;
  switch (getGPIOMode(pin)) {
    case GPIOMode::ANALOG_OUT:      return _pwm_buffer[pin];
    case GPIOMode::OUTPUT:          return _gpo_levels[pin];
    case GPIOMode::INPUT:
    case GPIOMode::INPUT_PULLUP:
    case GPIOMode::INPUT_PULLDOWN:  return (0 != (_gpi_levels >> pin)) ? 255 : 0;
    default:                        return 0;
  }
}


int8_t SX8634::setGPOValue(uint8_t pin, uint8_t value) {
  if (pin < 8) {
    switch (getGPIOMode(pin)) {
      case GPIOMode::ANALOG_OUT:
        _pwm_buffer[pin] = value;
        return _proc_waiting_pwm_changes();
      case GPIOMode::OUTPUT:          return _write_gpo_register(pin, (value != 0));
      default:                        return -2;
    }
  }
  return -1;
}


/*
*
*/
int8_t SX8634::_write_gpo_register(uint8_t pin, bool value) {
  uint8_t pin_mask  = 1 << pin;
  // TODO: We should be setting our shadow when the bus-op completes. Not here.
  _gpo_levels[pin] = value ? 255 : 0;
  uint8_t w_val = value ? (_registers[SX8634_REG_GPO_CTRL] | pin_mask) : (_registers[SX8634_REG_GPO_CTRL] & ~pin_mask);
  if (w_val != _registers[SX8634_REG_GPO_CTRL]) {
    return _write_register(SX8634_REG_GPO_CTRL, w_val);
  }
  return 0;
}


/*
*
* Returns zero on success, -2 on busy, -1 on bus error.
*/
int8_t SX8634::_write_pwm_value(uint8_t pin, uint8_t value) {
  int8_t ret = -2;
  if (false == _sx8634_flag(SX8634_FLAG_PWM_CHANGE_IN_FLIGHT)) {
    int idx = _get_shadow_reg_mem_addr(SX8634_REG_GPP_PIN_ID);
    _registers[idx + 0] = pin;
    _registers[idx + 1] = value;
    I2CBusOp* nu = _bus->new_op(BusOpcode::TX, this);
    if (nullptr != nu) {
      nu->dev_addr = _opts.i2c_addr;
      nu->sub_addr = SX8634_REG_GPP_PIN_ID;
      nu->buf      = &_registers[idx];
      nu->buf_len  = 2;
      if (0 == _bus->queue_io_job(nu)) {
        _sx8634_set_flag(SX8634_FLAG_PWM_CHANGE_IN_FLIGHT);
        ret = 0;
      }
      else {
        ret = -1;
      }
    }
  }
  return ret;
}


/*
* Since the SX8634 doesn't provide us discrete registers for each PWM pin, we
*   need to access them via a two-register window. This function makes sure that
*   asynchronous writes to PWM by the application all end up making it to the
*   hardware.
* Dispatches a bus operation for the first difference it finds (if any).
* Sets class flags to indicate state of PWM change operation.
*
* Returns zero on success, -2 on PWM busy, -1 on bus error.
*/
int8_t SX8634::_proc_waiting_pwm_changes() {
  int8_t ret = 0;
  for (uint8_t i = 0; i < 8; i++) {
    if (GPIOMode::ANALOG_OUT == getGPIOMode(i)) {
      if (_pwm_buffer[i] != _gpo_levels[i]) {
        ret = _write_pwm_value(i, _pwm_buffer[i]);
      }
    }
  }
  return ret;
}



/*******************************************************************************
* Event pitching functions
*******************************************************************************/

/*
* Send an event for the given button and state.
*/
void SX8634::_send_button_event(uint8_t button, bool pushed) {
  ManuvrMsg* msg = Kernel::returnEvent(pushed ? MANUVR_MSG_USER_BUTTON_PRESS : MANUVR_MSG_USER_BUTTON_RELEASE);
  msg->addArg((uint8_t) button);
  Kernel::staticRaiseEvent(msg);
}

/*
* Send an event for notice of slider update.
*/
void SX8634::_send_slider_event() {
  bool queued = platform.kernel()->containsPreformedEvent(&_slider_msg);
  if (!queued) {
    if (_slider_msg.argCount() > 0) {
      // TODO: Avoiding needless heap-thrash requires hackery. The Argument class
      //   should be abstracting this ugliness away from client classes.
      //   Requires deeper changes to fix.
      _slider_msg.getArgs()->target_mem = (uintptr_t*) (uint32_t) _slider_val;
    }
    else {
      // Should only allocate once.
      _slider_msg.addArg((uint16_t) _slider_val);
    }
    Kernel::staticRaiseEvent(&_slider_msg);
  }
}

/*
* This is the handler function for changes in GPI values.
*/
int8_t SX8634::_process_gpi_change(uint8_t new_val) {
  int8_t ret = 0;
  if (new_val != _gpi_levels) {
    // Bitshift the GPI values into discrete messages.
    for (uint8_t i = 0; i < 8; i++) {
      uint8_t mask = 0x01 << i;
      if ((new_val & mask) ^ (_gpi_levels & mask)) {
        ManuvrMsg* msg = Kernel::returnEvent(MANUVR_MSG_GPI_CHANGE);
        msg->addArg((uint8_t) i);
        msg->addArg((bool) (0 != (new_val & mask)));
        Kernel::staticRaiseEvent(msg);
        ret++;
      }
    }
    _gpi_levels = new_val;  // Store the new value;
  }
  return ret;
}


/*******************************************************************************
* SPM related functions.
*******************************************************************************/

/*
* Calling this function sets off a long chain of async bus operations that
*   ought to end up with the SPM fully read, and the state machine back to
*   READY.
*/
int8_t SX8634::_read_full_spm() {
  _set_fsm_position(SX8634_FSM::SPM_READ);
  if (!(_sx8634_flag(SX8634_FLAG_SPM_OPEN) & !_sx8634_flag(SX8634_FLAG_SPM_WRITABLE))) {
    return _open_spm_access_r();
  }
  return -1;
}

/*
* Calling this function sets off a long chain of async bus operations that
*   ought to end up with the SPM fully written, and the state machine back to
*   READY.
*/
int8_t SX8634::_write_full_spm() {
  _set_fsm_position(SX8634_FSM::SPM_WRITE);
  if (!(_sx8634_flag(SX8634_FLAG_SPM_OPEN) & _sx8634_flag(SX8634_FLAG_SPM_WRITABLE))) {
    return _open_spm_access_w();
  }
  return -1;
}

/*
* Inhibits interrupt service.
* Opens the SPM for reading.
*/
int8_t SX8634::_open_spm_access_r() {
  _sx8634_set_flag(SX8634_FLAG_IRQ_INHIBIT);
  if (SX8634OpMode::SLEEP != _mode) {
    setMode(SX8634OpMode::SLEEP);
  }
  return _write_register(SX8634_REG_SPM_CONFIG, 0x18);
}

int8_t SX8634::_open_spm_access_w() {
  _sx8634_set_flag(SX8634_FLAG_IRQ_INHIBIT);
  if (SX8634OpMode::SLEEP != _mode) {
    setMode(SX8634OpMode::SLEEP);
  }
  return _write_register(SX8634_REG_SPM_CONFIG, 0x10);
}

int8_t SX8634::_close_spm_access() {
  _sx8634_clear_flag(SX8634_FLAG_IRQ_INHIBIT);
  setMode(SX8634OpMode::ACTIVE);
  return _write_register(SX8634_REG_SPM_CONFIG, 0x08);
}

/*
* Read 8-bytes of SPM into shadow from the device.
* This should only be called when the SPM if open. Otherwise, it will read ISR
*   registers and there is no way to discover the mistake.
*/
int8_t SX8634::_read_block8(uint8_t idx) {
  #if defined(CONFIG_SX8634_DEBUG)
    StringBuilder output;
    output.concatf("_read_block8(%u)\n", idx);
    Kernel::log(&output);
  #endif
  I2CBusOp* nu = _bus->new_op(BusOpcode::RX, this);
  if (nu) {
    nu->dev_addr = _dev_addr;
    nu->sub_addr = 0;
    nu->buf      = (_spm_shadow + idx);
    nu->buf_len  = 8;
    return _bus->queue_io_job(nu);
  }
  return -1;
}

/*
* Write 8-bytes of SPM shadow back to the device.
* This should only be called when the SPM if open. Otherwise, it will read ISR
*   registers and there is no way to discover the mistake.
*/
int8_t SX8634::_write_block8(uint8_t idx) {
  #if defined(CONFIG_SX8634_DEBUG)
    StringBuilder output;
    output.concatf("_write_block8(%u)\n", idx);
    Kernel::log(&output);
  #endif
  I2CBusOp* nu = _bus->new_op(BusOpcode::TX, this);
  if (nu) {
    nu->dev_addr = _dev_addr;
    nu->sub_addr = 0;
    nu->buf      = (_spm_shadow + idx);
    nu->buf_len  = 8;
    return _bus->queue_io_job(nu);
  }
  return -1;
}

/*
* Compares the existing SPM shadow against the desired config. Then copies the
*   differences into the SPM shadow in preparation for write.
* Returns...
*   1 on Valid and different. Pushes state machine into SPM_WRITE.
*   0 on equality
*   -1 on invalid provided
*   -2 SPM not shadowed
*   -3 no desired config to compare against
*/
int8_t SX8634::_compare_config() {
  int8_t ret = -3;
  if (nullptr != _opts.conf) {
    ret++;
    if (_sx8634_flag(SX8634_FLAG_SPM_SHADOWED)) {
      ret++;
      uint8_t rsvd_idx  = 0;
      uint8_t given_idx = 0;
      for (uint8_t i = 0; i < sizeof(_spm_shadow); i++) {
        if (i == _reserved_spm_offsets[rsvd_idx]) {
          // Skip the comparison. Increment the reserved pointer.
          rsvd_idx++;
        }
        else {
          // This is a comparable config byte.
          if (_spm_shadow[i] != *(_opts.conf + given_idx)) {
            _sx8634_set_flag(SX8634_FLAG_SPM_DIRTY);
            _spm_shadow[i] = *(_opts.conf + given_idx);
          }
          given_idx++;
        }
      }
      ret = (_sx8634_flag(SX8634_FLAG_SPM_DIRTY)) ? 1 : 0;
    }
  }
  return ret;
}

/*
* If the SPM is shadowed, set the state of the class to reflect it.
* Copies the default GPO values to the local shadow.
*
* Returns 0 on success, -1 on no SPM shadow, -2 on bogus config.
*/
int8_t SX8634::_class_state_from_spm() {
  if (_sx8634_flag(SX8634_FLAG_SPM_SHADOWED)) {
    uint8_t dev_levels = _spm_shadow[66];
    for (uint8_t i = 0; i < 8; i++) {
      if (GPIOMode::OUTPUT == getGPIOMode(i)) {
        _gpo_levels[i] = ((dev_levels >> i) & 0x01) ? 255 : 0;
      }
    }
    return 0;
  }
  return -1;
}


/*******************************************************************************
* Low-level stuff
*******************************************************************************/

int8_t SX8634::_wait_for_reset(uint timeout_ms) {
  int8_t ret = -1;
  #if defined(CONFIG_SX8634_DEBUG)
    Kernel::log("Waiting for SX8634...\n");
  #endif
  if (_opts.haveIRQPin()) {
    uint8_t tries = 40;
    while ((--tries > 0) && (0 == readPin(_opts.irq_pin))) {
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    if (0 < tries) {
      ret = 0;
    }
  }
  else {
    vTaskDelay(timeout_ms / portTICK_PERIOD_MS);
    ret = 0;
  }
  return ret;
}


/*
* Clears all stateful data in the class.
*/
int8_t SX8634::_clear_registers() {
  _sx8634_clear_flag(
    SX8634_FLAG_PING_IN_FLIGHT | SX8634_FLAG_DEV_FOUND |
    SX8634_FLAG_SPM_DIRTY | SX8634_FLAG_COMPENSATING |
    SX8634_FLAG_CONF_IS_NVM | SX8634_FLAG_SLIDER_TOUCHED |
    SX8634_FLAG_SLIDER_MOVE_DOWN | SX8634_FLAG_SLIDER_MOVE_UP |
    SX8634_FLAG_SPM_WRITABLE | SX8634_FLAG_SPM_OPEN |
    SX8634_FLAG_SPM_SHADOWED | SX8634_FLAG_PWM_CHANGE_IN_FLIGHT
  );
  _mode = SX8634OpMode::RESERVED;
  _slider_val = 0;
  _buttons    = 0;
  _compensations = 0;
  _nvm_burns     = 0;
  _gpi_levels    = 0;
  memset(_registers,  0, sizeof(_registers));
  memset(_spm_shadow, 0, sizeof(_spm_shadow));
  memset(_gpo_levels, 0, sizeof(_gpo_levels));
  memset(_pwm_buffer, 0, sizeof(_pwm_buffer));
  return 0;
}



int8_t SX8634::_start_compensation() {
  return _write_register(SX8634_REG_COMP_OP_MODE, 0x04);
}


int8_t SX8634::_ll_pin_init() {
  if (_opts.haveResetPin()) {
    gpioDefine(_opts.reset_pin, GPIOMode::OUTPUT);
  }
  if (_opts.haveIRQPin()) {
    _sx8634_set_flag(SX8634_FLAG_IRQ_INHIBIT);
    gpioDefine(_opts.irq_pin, GPIOMode::INPUT_PULLUP);
    setPinFxn(_opts.irq_pin, FALLING, sx8634_isr);
  }
  return 0;  // Both pins are technically optional.
}


/*
* Pings the device.
*/
int8_t SX8634::ping() {
  if (!_sx8634_flag(SX8634_FLAG_PING_IN_FLIGHT)) {
    _sx8634_set_flag(SX8634_FLAG_PING_IN_FLIGHT);
    return ping_device();
  }
  return -1;
}



/*******************************************************************************
* These functions are optional at build time and relate to programming the NVM.
* Per the datasheet: Programming the NVM _must_ be done while the supply voltage
*   is [3.6, 3.7v].
* Also, the NVM can only be written 3 times. After which the chip will have to
*   be configured manually on every reset.
* Burning capability should probably only be built into binaries that are
*   dedicated to provisioning new hardware, and never into the end-user binary.
*******************************************************************************/

#if defined(CONFIG_SX8634_PROVISIONING)

int8_t SX8634::burn_nvm() {
  int8_t ret = -4;
  if (_sx8634_flag(SX8634_FLAG_SPM_SHADOWED)) {
    // SPM is shadowed. Whatever changes we made to it will be propagated into
    //   the NVM so that they become the defaults after resst.
    ret++;
    _set_fsm_position(SX8634_FSM::NVM_BURN);
    if (SX8634OpMode::DOZE != _mode) {
      #if defined(CONFIG_SX8634_DEBUG)
        Kernel::log("SX8634 moving to doze mode.\n");
      #endif
      setMode(SX8634OpMode::DOZE);
    }
    ret = _write_register(SX8634_REG_SPM_KEY_MSB, 0x62);
  }
  return ret;
}


/*
* Buffer is assumed to be 128-bytes long. Copy the SPM verbatim.
*/
int8_t SX8634::copy_spm_to_buffer(uint8_t* buf) {
  int8_t ret = -2;
  if (_sx8634_flag(SX8634_FLAG_SPM_SHADOWED)) {
    ret++;
    if (!_sx8634_flag(SX8634_FLAG_SPM_DIRTY)) {
      memcpy(buf, _spm_shadow, 128);
      ret++;
    }
  }
  return ret;
}


/*
* Buffer is assumed to be 128-bytes long. Copy the SPM verbatim.
*/
int8_t SX8634::load_spm_from_buffer(uint8_t* buf) {
  _sx8634_set_flag(SX8634_FLAG_SPM_DIRTY | SX8634_FLAG_SPM_SHADOWED);
  memcpy(_spm_shadow, buf, 128);
  return 0;
}


#endif  // CONFIG_SX8634_PROVISIONING
#endif  // CONFIG_MANUVR_SX8634
