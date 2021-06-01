# esp8266-outlet-timer
Ever tried to get one of [these things](https://www.amazon.com/Century-Indoor-24-Hour-Mechanical-Outlet/dp/B01LPSGBZS/) to switch on or off at sunrise / sunset?
You might have wanted to have a lamp automatically turn on when it gets dark outside or have something turn off during the night.
If so, you will have probably realised, that there are things called seasons on our planet and that the time of sunrises and sunsets shifts over the course of a year.

To not have to manually change the time on a suboptimally user-friendly dial, this application uses an ESP8266 microcontroller to switch a power outlet on and off.
Since the chip has WiFi connectivity, it is able to connect to a specified network, retrieve the current time in your local timezone via SNTP, calculate the time of sunset and sunrise and toggle the electrical outlet accordingly.

## Hardware Setup
**WARNING: This project involves connecting a relay up to mains voltage.
Please only attempt to build this thing if you know what you're doing.
I am not responsible for any accidents that you may cause.**

### Prerequisites
- An board with an ESP8266, such as [this one](https://www.ebay.com/itm/313549008122)
- Any D-type flip-flop IC capable of operating at 3.3V
- A mains voltage relay for switching the power outlet
- Input and output sockets for connecting the relay up to mains voltage
- Optional: A case and a way to power the esp8266 board inside it

### Wiring
Connect the ESP8266's GPIOs 4 and 5 to the flip-flop's clock and data pins respectively.
**Be aware that the ESP8266 GPIO numbers are probably different from the GPIO numbering on your board.
If you have a NodeMCU devboard, for example, refer to [this pin diagram](https://www.teachmemicro.com/wp-content/uploads/2018/04/NodeMCUv3.0-pinout.jpg).**
If GPIOs 4 and 5 are inaccessible in your configuration, feel free to change them in the `main/outlet.c` file.

Also connect GPIO 16, the pin with a `Wake` function to the reset (or `RST`) connection on the board. This will enable the chip to wake up from deep sleep.

Next, connect the output of the flip-flop (`Q`) to the low-voltage input of the relay.
Also make sure to connect the ESP8266 board's ground to the flip-flop's and relay's ground connections.

Finally, connect the high-voltage side of the relay to act as a switch for the output power outlet and put the circuit in a case if you have one.

## Running the code
You need to have the [Espressif ESP8266 RTOS SDK](https://github.com/espressif/ESP8266_RTOS_SDK) installed in order to compile the code.

If you want to compile from a Docker container, this is a Dockerfile that will install the dependencies for the SDK:
```Dockerfile
FROM debian

RUN apt-get update && apt-get install -y git python3 python3-pip python3-setuptools python3-wheel flex bison gperf libncurses5-dev libncursesw5-dev
RUN ln -s /usr/bin/python3 /usr/bin/python

WORKDIR /
```
To create & start the container:

	docker run --name esp8266-builder --device /dev/<ESP8266>:/dev/<ESP8266> -v `pwd`:/build -it esp-build /bin/bash

Where `dev/<ESP8266>` should be replaced with the path to the USBtty device of your ESP8266 board.
The command mounts your current working directory into the docker container under `/build`.

### Configuring
You need to configure a few settings before building:

	make menuconfig

Some important things to change are:
- `Serial flasher config -> Default serial port` to your device path
- `Wifi Connection Information` in order to input your WiFi's name (SSID) and password
- `Outlet Switching Settings` in order to configure when you want the outlet to be on or off
- `Compiler options -> Optimization Level` to `Release (-Os)`

If you do not know what a certain setting does, you can press `?` on the item.

### Compiling
Now, to compile, simply type:

	make all
    
This will compile the bootloader and application for the ESP8266 and could take a while depending on your computer's speed

### Flashing
To flash the ESP8266, run:

	make flash

This will flash the chip and reset it, thereby running your code.

If you want debug output from the chip, run:

	make monitor
    
To start a serial monitor that will display messages from the chip.