/*
File:   SX8634.h
Author: J. Ian Lindsay
Date:   2019.08.10

Copyright 2019 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/

#include "SX8634.h"

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
static volatile bool isr_fired = false;

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

/**
* Issue a human-readable string representing the platform state.
*
* @return A string constant.
*/
const char* getPinModeStr(GPIOMode mode) {
  switch (mode) {
    case GPIOMode::SX_IN:           return "INPUT";
    case GPIOMode::SX_OUT:          return "OUTPUT";
    case GPIOMode::SX_OUT_OD:       return "OUTPUT_OD";
    case GPIOMode::SX_IN_PULLUP:    return "INPUT_PULLUP";
    case GPIOMode::SX_IN_PULLDOWN:  return "INPUT_PULLDOWN";
    case GPIOMode::SX_ANA_OUT:      return "ANALOG_OUT";
    case GPIOMode::SX_ANA_IN:       return "ANALOG_IN";
    case GPIOMode::SX_UNINIT:
    default:
      break;
  }
  return "UNINIT";
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


/* Static utility function for dumping buffers for humans to read. */
static void printBuffer(uint8_t* buf, unsigned int len, const char* indent) {
  if (buf) {
    const char* ind = (indent) ? indent : "\t";
    unsigned int i = 0;
    if (len >= 16) {
      for (i = 0; i < (len - 16); i+=16) {
        Serial.print(ind);
        Serial.print("0x");
        Serial.print(i, HEX);
        Serial.print(": ");
        for (uint8_t n = 0; n < 16; n++) {
          Serial.print(*(buf + i), HEX);
          Serial.print(" ");
        }
        Serial.print("\n");
      }
    }
    if (i < len) {
      Serial.print(ind);
      Serial.print("0x");
      Serial.print(i, HEX);
      Serial.print(": ");
      for (; i < len; i++) {
        Serial.print(*(buf + i), HEX);
        Serial.print(" ");
      }
      Serial.print("\n");
    }
  }
  else {
    if (indent) {
      Serial.print(indent);
    }
    Serial.print("(NULL BUFFER)\n");
  }
}


/*
* This is the ISR. Flags the class for attention in idle time.
*/
void sx8634_isr() {  isr_fired = true;  }


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
SX8634::SX8634(const SX8634Opts* o) : _opts{o} {
  INSTANCE = this;
  _clear_registers();
  _ll_pin_init();
}

/*
* Destructor
*/
SX8634::~SX8634() {
}


/**
* Debug support method.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void SX8634::printOverview() {
  Serial.print("Touch sensor (SX8634)");
  Serial.print(PRINT_DIVIDER_1_STR);
  Serial.print("-- Mode:           ");
  Serial.println(getModeStr(operationalMode()));
  Serial.print("-- Found:          ");
  Serial.println(deviceFound() ? 'y':'n');
  Serial.print("-- IRQ Inhibit:    ");
  Serial.println(_sx8634_flag(SX8634_FLAG_IRQ_INHIBIT) ? 'y': 'n');
  Serial.print("-- PWM sync'd:     ");
  Serial.println(_sx8634_flag(SX8634_FLAG_PWM_CHANGE_IN_FLIGHT) ? 'n': 'y');
  Serial.print("-- Compensations:  ");
  Serial.println(_compensations, DEC);
  Serial.print("-- FSM Position:   ");
  Serial.println(getFSMStr(_fsm));
  Serial.println("--\n-- Registers:");
  printBuffer(_registers, sizeof(_registers), "--\t  ");
}

/**
* Debug support method.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void SX8634::printSPMShadow() {
  Serial.print("-- SPM/NVM:\n");
  Serial.print("--\tConf source:    ");
  Serial.println(_sx8634_flag(SX8634_FLAG_CONF_IS_NVM) ? "NVM" : "QSM");

  Serial.print("--\tSPM shadowed:   ");
  Serial.println(_sx8634_flag(SX8634_FLAG_SPM_SHADOWED) ? 'y': 'n');

  if (_sx8634_flag(SX8634_FLAG_SPM_SHADOWED)) {
    printBuffer(_spm_shadow, sizeof(_spm_shadow), "--\t  ");
    Serial.print("--\tSPM Dirty:      ");
    Serial.println(_sx8634_flag(SX8634_FLAG_SPM_DIRTY) ? 'y': 'n');
  }
  Serial.print("--\tSPM open:       ");
  Serial.println(_sx8634_flag(SX8634_FLAG_SPM_OPEN) ? 'y': 'n');
  if (_sx8634_flag(SX8634_FLAG_SPM_OPEN)) {
    Serial.print("--\tSPM writable:   ");
    Serial.println(_sx8634_flag(SX8634_FLAG_SPM_WRITABLE) ? 'y': 'n');
  }
  Serial.print("--\tNVM burns:      ");
  Serial.println(_nvm_burns);
}


/**
* Debug support method.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void SX8634::printGPIO() {
  Serial.print("-- GPIO:\n");
  for (uint8_t i = 0; i < 8; i++) {
    Serial.print("--\t");
    Serial.print(i, DEC);
    Serial.print(":\t");
    Serial.print(getPinModeStr(getGPIOMode(i)));
    Serial.print("\t");
    Serial.println(getGPIOValue(i), DEC);
  }
}

/**
* Debug support method.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void SX8634::printDebug() {
  printOverview();
  Serial.print("--\n");
  printSPMShadow();
  Serial.print("--\n");
  printGPIO();
}



/*******************************************************************************
* Shadow register access functions
*******************************************************************************/

int8_t SX8634::_get_shadow_reg_mem_addr(uint8_t addr) {
  uint8_t maximum_idx = 18;
  switch (addr) {
    case SX8634_REG_IRQ_SRC:         maximum_idx--;
    case SX8634_REG_CAP_STAT_MSB:    maximum_idx--;
    case SX8634_REG_CAP_STAT_LSB:    maximum_idx--;
    case SX8634_REG_SLIDER_POS_MSB:  maximum_idx--;
    case SX8634_REG_SLIDER_POS_LSB:  maximum_idx--;
    case SX8634_REG_RESERVED_0:      maximum_idx--;
    case SX8634_REG_RESERVED_1:      maximum_idx--;
    case SX8634_REG_GPI_STAT:        maximum_idx--;
    case SX8634_REG_SPM_STAT:        maximum_idx--;
    case SX8634_REG_COMP_OP_MODE:    maximum_idx--;
    case SX8634_REG_GPO_CTRL:        maximum_idx--;
    case SX8634_REG_GPP_PIN_ID:      maximum_idx--;
    case SX8634_REG_GPP_INTENSITY:   maximum_idx--;
    case SX8634_REG_SPM_CONFIG:      maximum_idx--;
    case SX8634_REG_SPM_BASE_ADDR:   maximum_idx--;
    case SX8634_REG_RESERVED_2:      maximum_idx--;
    case SX8634_REG_SPM_KEY_MSB:     maximum_idx--;
    case SX8634_REG_SPM_KEY_LSB:     maximum_idx--;
    case SX8634_REG_SOFT_RESET:
      return (maximum_idx & 0x0F);
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
  if (idx >= 0) {
    if (((uint8_t) idx + len) <= sizeof(_registers)) {
      memcpy(&_registers[idx], buf, len);
    }
  }
}


int8_t SX8634::_write_register(uint8_t addr, uint8_t val) {
  int idx = _get_shadow_reg_mem_addr(addr);
  if (idx >= 0) {
    _registers[idx] = val;
    return _write_device(addr, &_registers[idx], 1);
  }
  return -1;
}


/*
*/
int8_t SX8634::_read_device(uint8_t reg, uint8_t* buf, uint8_t len) {
  int8_t return_value = -1;
  Wire.beginTransmission(_opts.i2c_addr);
  Wire.write(reg);
  if (0 == Wire.endTransmission(false)) {
    Wire.requestFrom(_opts.i2c_addr, len, (uint8_t) 0);
    for (uint8_t i = 0; i < len; i++) {
      *(buf + 1) = Wire.receive();
    }
    return_value = 0;
  }
  return return_value;
}


/*
*/
int8_t SX8634::_write_device(uint8_t reg, uint8_t* buf, uint8_t len) {
  int8_t return_value = -1;
  Wire.beginTransmission(_opts.i2c_addr);
  Wire.write(reg);
  for (uint8_t i = 0; i < len; i++) {
    Wire.write(*(buf + i));
  }
  return_value = Wire.endTransmission();
  return return_value;
}


/*******************************************************************************
* Class-specific functions
*******************************************************************************/

/*
* Reset the chip. By hardware pin (if possible) or by software command.
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
    digitalWrite(_opts.reset_pin, LOW);
    delay(10);
    digitalWrite(_opts.reset_pin, HIGH);
    ret = _reset_callback();
  }
  else {
    ret = _write_register(SX8634_REG_SOFT_RESET, 0xDE);
    if (0 == ret) {
      ret = _write_register(SX8634_REG_SOFT_RESET, 0x00);
      if (0 == ret) {
        ret = _reset_callback();
      }
    }
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
    ret = ping();
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
        case GPIOMode::SX_ANA_OUT:
        case GPIOMode::SX_OUT:
        case GPIOMode::SX_IN:
        case GPIOMode::SX_IN_PULLUP:
        case GPIOMode::SX_IN_PULLDOWN:
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
          return GPIOMode::SX_OUT;
        case 1:
          return GPIOMode::SX_ANA_OUT;
        case 2:
          switch (0x03 & (pull_val >> (2 * ((pin > 3) ? (pin-4) : pin)))) {
            case 0:    return GPIOMode::SX_IN;
            case 1:    return GPIOMode::SX_IN_PULLUP;
            case 2:    return GPIOMode::SX_IN_PULLDOWN;
            case 3:    return GPIOMode::SX_IN;   // This is a reserved case...
          }
        case 3:
          return GPIOMode::SX_UNINIT;
      }
    }
  }
  // Without the SPM, or with an out-of-range pin, who can say?
  return GPIOMode::SX_UNINIT;
}


/*
* If the pin is an input, returns the last-know state of the pin.
* If the pin is an output, returns the last-confirmed write to the pin.
*/
uint8_t SX8634::getGPIOValue(uint8_t pin) {
  pin = pin & 0x07;
  switch (getGPIOMode(pin)) {
    case GPIOMode::SX_ANA_OUT:      return _pwm_buffer[pin];
    case GPIOMode::SX_OUT:          return _gpo_levels[pin];
    case GPIOMode::SX_IN:
    case GPIOMode::SX_IN_PULLUP:
    case GPIOMode::SX_IN_PULLDOWN:  return (0 != (_gpi_levels >> pin)) ? 255 : 0;
    default:                        return 0;
  }
}


int8_t SX8634::setGPOValue(uint8_t pin, uint8_t value) {
  if (pin < 8) {
    switch (getGPIOMode(pin)) {
      case GPIOMode::SX_ANA_OUT:
        _pwm_buffer[pin] = value;
        return _proc_waiting_pwm_changes();
      case GPIOMode::SX_OUT:          return _write_gpo_register(pin, (value != 0));
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
  uint8_t sr_val = _get_shadow_reg_val(SX8634_REG_GPO_CTRL);
  uint8_t w_val = value ? (sr_val | pin_mask) : (sr_val & ~pin_mask);
  if (w_val != sr_val) {
    if (0 == _write_register(SX8634_REG_GPO_CTRL, w_val)) {
      if (GPIOMode::SX_OUT == getGPIOMode(pin)) {
        _gpo_levels[pin] = (value) ? 255 : 0;
      }
    }
  }
  return 0;
}


/*
* Note that we can't write the two bytes at the same time. They must be separate
*   bus operations.
*
* Returns zero on success, -2 on busy, -1 on bus error.
*/
int8_t SX8634::_write_pwm_value(uint8_t pin, uint8_t value) {
  int8_t ret = -2;
  if (false == _sx8634_flag(SX8634_FLAG_PWM_CHANGE_IN_FLIGHT)) {
    int idx = _get_shadow_reg_mem_addr(SX8634_REG_GPP_PIN_ID);
    _set_shadow_reg_val(SX8634_REG_GPP_PIN_ID, pin);
    _set_shadow_reg_val(SX8634_REG_GPP_INTENSITY, value);
    ret = -1;
    if (0 == _write_device(SX8634_REG_GPP_PIN_ID, &_registers[idx], 1)) {
      _sx8634_set_flag(SX8634_FLAG_PWM_CHANGE_IN_FLIGHT);
      if (0 == _write_device(SX8634_REG_GPP_INTENSITY, &_registers[idx+1], 1)) {
        // Update our local shadow to reflect the new GPP state.
        _gpo_levels[_get_shadow_reg_val(SX8634_REG_GPP_PIN_ID)] = _registers[idx+1];
        _sx8634_clear_flag(SX8634_FLAG_PWM_CHANGE_IN_FLIGHT);
        ret = 0;
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
    if (GPIOMode::SX_ANA_OUT == getGPIOMode(i)) {
      if (_pwm_buffer[i] != _gpo_levels[i]) {
        return _write_pwm_value(i, _pwm_buffer[i]);
      }
    }
  }
  return ret;
}



/*******************************************************************************
* Event pitching functions
*******************************************************************************/

/*
* When the main program sees IRQ assertion, this function should be called, but
*   probably not from the ISR directly. Dispatches additional I/O.
*/
int8_t SX8634::read_irq_registers() {
  int8_t ret = -1;
  if (!_sx8634_flag(SX8634_FLAG_IRQ_INHIBIT)) {
    ret = 0;
    // If IRQ service is not inhibited, read all the IRQ-related
    //   registers in one shot.
    if (0 == _read_device(0x00, _registers, 10)) {
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
              Serial.print("-- SX8634 is now in mode %s\n");
              Serial.println(getModeStr(current));
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
              Serial.print("-- Buttons: ");
              Serial.println(current, DEC);
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
              Serial.print("-- Slider: ");
              Serial.println(current, DEC);
            #endif   // CONFIG_SX8634_DEBUG
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
          Serial.print("-- SX8634 NVM burn completed.\n");
        }
        if (first_irq) {
          _sx8634_clear_flag(SX8634_FLAG_INITIAL_IRQ_READ);
        }
      }
    }
  }
  #if defined(CONFIG_SX8634_DEBUG)
    Serial.print("read_irq_registers() returns ");
    Serial.println(ret, DEC);
  #endif
  return ret;
}


/*
* Fire a callback for the given button and state.
*/
uint8_t SX8634::poll() {
  uint8_t ret = 0;
  if (isr_fired) {
    ret = read_irq_registers();
    isr_fired = false;
  }
  return ret;
}


/*
* Fire a callback for the given button and state.
*/
void SX8634::_send_button_event(uint8_t button, bool pushed) {
  //msg->addArg((uint8_t) button);
}

/*
* Fire a callback for notice of slider update.
*/
void SX8634::_send_slider_event() {
  //_slider_msg.addArg((uint16_t) _slider_val);
}

/*
* This is the handler function for changes in GPI values.
*/
int8_t SX8634::_process_gpi_change(uint8_t new_val) {
  int8_t ret = 0;
  if (new_val != _gpi_levels) {
    // Bitshift the GPI values and fire callbacks.
    for (uint8_t i = 0; i < 8; i++) {
      uint8_t mask = 0x01 << i;
      if ((new_val & mask) ^ (_gpi_levels & mask)) {
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
* Calling this function sets off a long chain of bus operations that
*   ought to end up with the SPM fully read, and the state machine back to
*   READY.
*/
int8_t SX8634::_read_full_spm() {
  int8_t ret = -1;
  _set_fsm_position(SX8634_FSM::SPM_READ);
  if (!(_sx8634_flag(SX8634_FLAG_SPM_OPEN) & !_sx8634_flag(SX8634_FLAG_SPM_WRITABLE))) {
    if (0 == _open_spm_access_r()) {
      _set_shadow_reg_val(SX8634_REG_SPM_BASE_ADDR, 0);
      ret = _write_register(SX8634_REG_SPM_BASE_ADDR, 0x00);
      while (0 == ret) {
        ret = _read_block8(_get_shadow_reg_val(SX8634_REG_SPM_BASE_ADDR));
        if (0 == ret) {
          ret = _incremenet_spm_window();
        }
      }
      if (1 == ret) {
        _set_fsm_position(SX8634_FSM::READY);
        _sx8634_set_flag(SX8634_FLAG_SPM_SHADOWED);
        ret = 0;
      }
    }
  }
  return ret;
}

/*
* Calling this function sets off a long chain of bus operations that
*   ought to end up with the SPM fully written, and the state machine back to
*   READY.
*/
int8_t SX8634::_write_full_spm() {
  int8_t ret = -1;
  _set_fsm_position(SX8634_FSM::SPM_WRITE);
  if (!(_sx8634_flag(SX8634_FLAG_SPM_OPEN) & _sx8634_flag(SX8634_FLAG_SPM_WRITABLE))) {
    if (0 == _open_spm_access_w()) {
      _set_shadow_reg_val(SX8634_REG_SPM_BASE_ADDR, 0);
      ret = _write_register(SX8634_REG_SPM_BASE_ADDR, 0x00);
      while (0 == ret) {
        ret = _write_block8(_get_shadow_reg_val(SX8634_REG_SPM_BASE_ADDR));
        if (0 == ret) {
          ret = _incremenet_spm_window();
        }
      }
      if (1 == ret) {
        _set_fsm_position(SX8634_FSM::READY);
        ret = 0;
      }
    }
  }
  return ret;
}

/*
* Inhibits interrupt service.
* Opens the SPM for reading.
*/
int8_t SX8634::_open_spm_access_r() {
  int8_t ret = -1;
  _sx8634_set_flag(SX8634_FLAG_IRQ_INHIBIT);
  if (SX8634OpMode::SLEEP != _mode) {
    setMode(SX8634OpMode::SLEEP);
  }
  if (0 == _write_register(SX8634_REG_SPM_CONFIG, 0x18)) {
    _sx8634_set_flag(SX8634_FLAG_SPM_WRITABLE, false);
    _sx8634_set_flag(SX8634_FLAG_SPM_OPEN, true);
    ret = 0;
  }
  #if defined(CONFIG_SX8634_DEBUG)
    Serial.print("_open_spm_access_r() returns ");
    Serial.print(ret, DEC);
    Serial.print("\n");
  #endif
  return ret;
}


int8_t SX8634::_open_spm_access_w() {
  int8_t ret = -1;
  _sx8634_set_flag(SX8634_FLAG_IRQ_INHIBIT);
  if (SX8634OpMode::SLEEP != _mode) {
    setMode(SX8634OpMode::SLEEP);
  }
  if (0 == _write_register(SX8634_REG_SPM_CONFIG, 0x10)) {
    _sx8634_set_flag(SX8634_FLAG_SPM_WRITABLE, true);
    _sx8634_set_flag(SX8634_FLAG_SPM_OPEN, true);
    ret = 0;
  }
  #if defined(CONFIG_SX8634_DEBUG)
    Serial.print("_open_spm_access_w() returns ");
    Serial.print(ret, DEC);
    Serial.print("\n");
  #endif
  return ret;
}


int8_t SX8634::_close_spm_access() {
  int8_t ret = -1;
  _sx8634_clear_flag(SX8634_FLAG_IRQ_INHIBIT);
  setMode(SX8634OpMode::ACTIVE);
  if (0 == _write_register(SX8634_REG_SPM_CONFIG, 0x08)) {
    _sx8634_set_flag(SX8634_FLAG_SPM_WRITABLE, false);
    _sx8634_set_flag(SX8634_FLAG_SPM_OPEN, false);
    ret = 0;
  }
  #if defined(CONFIG_SX8634_DEBUG)
    Serial.print("_close_spm_access() returns ");
    Serial.print(ret, DEC);
    Serial.print("\n");
  #endif
  return ret;
}

/*
* Read 8-bytes of SPM into shadow from the device.
* This should only be called when the SPM if open. Otherwise, it will read ISR
*   registers and there is no way to discover the mistake.
*/
int8_t SX8634::_read_block8(uint8_t idx) {
  #if defined(CONFIG_SX8634_DEBUG)
    Serial.print("_read_block8(");
    Serial.print(idx, DEC);
    Serial.print(")\n");
  #endif
  return _read_device(0, (_spm_shadow + idx), 8);
}

/*
* Write 8-bytes of SPM shadow back to the device.
* This should only be called when the SPM if open. Otherwise, it will read ISR
*   registers and there is no way to discover the mistake.
*/
int8_t SX8634::_write_block8(uint8_t idx) {
  #if defined(CONFIG_SX8634_DEBUG)
    Serial.print("_write_block8(");
    Serial.print(idx, DEC);
    Serial.print(")\n");
  #endif
  return _write_device(0, (_spm_shadow + idx), 8);
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
      if (GPIOMode::SX_OUT == getGPIOMode(i)) {
        _gpo_levels[i] = ((dev_levels >> i) & 0x01) ? 255 : 0;
      }
    }
    return 0;
  }
  return -1;
}


int8_t SX8634::_incremenet_spm_window() {
  int8_t ret = -1;
  int spm_base_addr = _get_shadow_reg_val(SX8634_REG_SPM_BASE_ADDR);
  if (spm_base_addr <= 0x78) {
    // We're writing our shadow of the SPM. Write the next base address.
    _wait_for_reset(30);
    ret = _write_register(SX8634_REG_SPM_BASE_ADDR, spm_base_addr + 8);
  }
  else {
    _sx8634_clear_flag(SX8634_FLAG_SPM_DIRTY);
    ret = (0 == _close_spm_access()) ? 1 : -2;
  }
  return ret;
}



/*******************************************************************************
* Low-level stuff
*******************************************************************************/

int8_t SX8634::_wait_for_reset(uint timeout_ms) {
  int8_t ret = -1;
  #if defined(CONFIG_SX8634_DEBUG)
    Serial.print("Waiting for SX8634...\n");
  #endif
  if (_opts.haveIRQPin()) {
    uint8_t tries = 40;
    while ((--tries > 0) && (0 == digitalRead(_opts.irq_pin))) {
      delay(10);
    }
    if (0 < tries) {
      ret = 0;
    }
  }
  else {
    delay(timeout_ms);
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
  int8_t ret = _write_register(SX8634_REG_COMP_OP_MODE, 0x04);
  if (0 == ret) {
    _sx8634_set_flag(SX8634_FLAG_COMPENSATING, true);
  }
  return ret;
}


int8_t SX8634::_ll_pin_init() {
  if (_opts.haveResetPin()) {
    pinMode(_opts.reset_pin, OUTPUT);
  }
  if (_opts.haveIRQPin()) {
    _sx8634_set_flag(SX8634_FLAG_IRQ_INHIBIT);
    pinMode(_opts.irq_pin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(_opts.irq_pin), sx8634_isr, FALLING);
  }
  return 0;  // Both pins are technically optional.
}


/*
* Pings the device.
*/
int8_t SX8634::ping() {
  if (!_sx8634_flag(SX8634_FLAG_PING_IN_FLIGHT)) {
    _sx8634_set_flag(SX8634_FLAG_PING_IN_FLIGHT);
    Wire.beginTransmission(_opts.i2c_addr);
    if (0 == Wire.endTransmission()) {
      Serial.print("SX8634 found\n");
      _sx8634_clear_flag(SX8634_FLAG_PING_IN_FLIGHT);
      _sx8634_set_flag(SX8634_FLAG_DEV_FOUND, true);

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
          return _class_state_from_spm();
        }
      #endif   // CONFIG_SX8634_CONFIG_ON_FAITH
      // We have no option but to configure the class from the real SPM.
      return _read_full_spm();
    }
    else {
      Serial.print("SX8634 not found\n");
    }
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


#if defined(CONFIG_SX8634_PROVISIONING)

int8_t SX8634::burn_nvm() {
  int8_t ret = -6;
  if (_sx8634_flag(SX8634_FLAG_SPM_SHADOWED)) {
    // SPM is shadowed. Whatever changes we made to it will be propagated into
    //   the NVM so that they become the defaults after resst.
    ret++;
    _set_fsm_position(SX8634_FSM::NVM_BURN);
    if (SX8634OpMode::DOZE != _mode) {
      #if defined(CONFIG_SX8634_DEBUG)
        Serial.print("SX8634 moving to doze mode.\n");
      #endif
      setMode(SX8634OpMode::DOZE);
    }
    ret++;
    if (0 == _write_register(SX8634_REG_SPM_KEY_MSB, 0x62)) {
      Serial.print("0x62 --> SPM_KEY_MSB\n");
      ret++;
      if (0 == _write_register(SX8634_REG_SPM_KEY_LSB, 0x9D)) {
        Serial.print("0x9D --> SPM_KEY_LSB\n");
        ret++;
        if (0 == _write_register(SX8634_REG_SPM_BASE_ADDR, 0xA5)) {
          Serial.print("0xA5 --> SPM_BASE_ADDR\n");
          ret++;
          if (0 == _write_register(SX8634_REG_SPM_BASE_ADDR, 0x5A)) {
            Serial.print("0x5A --> SPM_BASE_ADDR\n");
            ret = 0;
          }
        }
      }
    }
  }
  return ret;
}

#endif  // CONFIG_SX8634_PROVISIONING
