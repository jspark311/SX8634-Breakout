# SX8634-Breakout

A breakout board for Semtech's SX8634 capacitive touch sensor.

#### [Hackaday.io Page](https://hackaday.io/project/166853-sx8634-touch-sensor-breakout)

#### [Order from Tindie](https://www.tindie.com/products/17862/)

#### [Arduino driver](https://github.com/jspark311/Arduino-SX8634)

------------------------

### What is in this repository:

**./doc**:  Location for documentation

**./KiCAD**:  Hardware design files

**./lib**:  Third-party libraries

**./main**:  The optional provisioning program

**./downloadDeps.sh**   A script to download dependencies


------------------------

### Building the provisioning program

The provisioning program is intended to be run on an ESP32 devkit-C or comparable. It can be built by doing...

    ./downloadDeps.sh
    make

Then, flash it to the ESP32 board....

    make flash monitor

------------------------

Front | Back
:-------:|:------:
![Front](osh-render-front.png)  | ![Back](osh-render-back.png)

[<img src="https://oshpark.com/assets/badge-5b7ec47045b78aef6eb9d83b3bac6b1920de805e9a0c227658eac6e19a045b9c.png" alt="Order from OSH Park">](https://oshpark.com/shared_projects/KLEZLMEO)
