#include <Wire.h>
#include <SX8634.h>


SX8634Opts _touch_opts(
  SX8634_DEFAULT_I2C_ADDR,  // i2c address
  10,                       // Reset pin. Output. Active low.
  11                        // IRQ pin. Input. Active low. Needs pullup.
);

SX8634 touch(&_touch_opts);


void cb_button(int button, bool pressed) {
  Serial.print("cb_button() ");
  Serial.print(button, DEC);
  Serial.print(" --> ");
  Serial.println(pressed ? "pressed" : "released");
}


void cb_slider(int slider, int value) {
  Serial.print("cb_slider() ");
  Serial.print(slider, DEC);
  Serial.print(" --> ");
  Serial.println(value, DEC);
}


void cb_gpi(int pin, int state) {
  Serial.print("cb_gpi() ");
  Serial.print(pin, DEC);
  Serial.print(" --> ");
  Serial.println(state, DEC);
}


void setup() {
  Serial.begin(115200);
  Wire.setSDA(18);
  Wire.setSCL(19);
  Wire.begin();

  touch.setButtonFxn(cb_button);
  touch.setSliderFxn(cb_slider);
  touch.setGPIFxn(cb_gpi);

  touch.reset();
  touch.setMode(SX8634OpMode::ACTIVE);
}


void loop() {
  int8_t t_res = touch.poll();
  if (0 < t_res) {
    // Something changed in the hardware.
  }
  else if (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case 'i':   // Debug prints.
        touch.printDebug();
        break;


      /* SX8634 control options */
      case 'R':   // Reset
        /*
        * Resetting the part will cause a chain of initialization events which
        *   might take a long time. There are ways to mitigate this, if desired.
        * See the doc surrounding the provisioner.
        */
        if (0 == touch.reset()) {
          Serial.println("touch.reset()");
        }
        else {
          Serial.println("Reset failed.");
        }
        break;

      case 'A':   // Set power mode to ACTIVE.
        if (0 == touch.setMode(SX8634OpMode::ACTIVE)) {
          Serial.println("SX8634 set to active.");
        }
        else {
          Serial.println("SX8634 failed to set active.");
        }
        break;

      case 'D':   // Set power mode to DOZE.
        if (0 == touch.setMode(SX8634OpMode::DOZE)) {
          Serial.println("SX8634 set to doze.");
        }
        else {
          Serial.println("SX8634 failed to set doze.");
        }
        break;

      case 'S':   // Set power mode to SLEEP.
        if (0 == touch.setMode(SX8634OpMode::SLEEP)) {
          Serial.println("SX8634 set to sleep.");
        }
        else {
          Serial.println("SX8634 failed to set sleep.");
        }
        break;


      /* SX8634 pin functions */
      case 'P':   // Set GPO pin
      case 'p':   // Clear GPO pin
        /*
        * General purpose output pins might be 8-bit PWM. We use a common API
        *   for both cases, with any non-zero interger construed as "asserted".
        */
        if (0 == touch.setGPOValue(3, ('P' == c) ? 255 : 0)) {
          Serial.print(('P' == c) ? "Set" : "Cleared");
          Serial.println(" pin 3.");
        }
        else {
          Serial.println("Failed to write GPO.");
        }
        break;

      case 'r':   // Read GPIO pin
        /*
        * GPIO pins can be read. If the pin is configured as an output, the
        *   currently set value will be returned.
        */
        Serial.print("getGPIOValue(3) --> ");
        Serial.print(touch.getGPIOValue(3), DEC);
        break;

      case '0':   // Read GPIO pin
        touch.read_irq_registers();
        break;

      case 'c':   // Print an application config blob from the current SPM.
        {
          uint8_t buf[128];
          memset(buf, 0, 128);
          if (0 == touch.copy_spm_to_buffer(buf)) {
            if (0 == SX8634::render_stripped_spm(buf)) {
              Serial.print("Application config blob:");
              Serial.print(PRINT_DIVIDER_1_STR);
              //printBuffer(buf, 97, "");
            }
          }
        }
        break;


      #if defined(CONFIG_SX8634_PROVISIONING)
        case 'B':   // Burn current config to NVM
          ret = touch.burn_nvm();
          Serial.print("burn_nvm() returns %d\n", ret);
          Serial.println(ret, DEC);
          break;
      #endif  // CONFIG_SX8634_PROVISIONING
    }
  }
}
