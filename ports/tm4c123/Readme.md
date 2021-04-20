# MicroPython Port for Tiva Launchpad

<img src="https://raw.githubusercontent.com/rk-exxec/micropython/tiva_from_stable/logo/trans-logo.png" height="200" /><img src="https://www.ti.com/diagrams/med_ek-tm4c123gxl_tivalp_angle_new.jpg" height="200" />


#### Table of contents
1. [Build](#build)
   - [Ubuntu](#ubuntu)
   - [Windows](#windows)
2. [Flashing](#flashing)
3. [Debug](#debug)
   - [TI CodeComposer Studio](#ccs)
   - [VSCode](#vscode)
4. [REPL](#connecting-to-repl)
5. [Usage](#usage)
   - [SD-Card](#sd-card)
   - [Pin](#pin)
   - [UART](#uart)
   - [SPI](#spi)


## Build
### Ubuntu
   Works for Ubuntu / Debian
1. Clone git repo:
   ```bash
   cd <your target directory>
   git clone https://github.com/rk-exxec/micropython.git && git checkout tiva_from_stable
   cd ./micropython
   ```
2. Install Toolchain:
   ```bash
   source tools/ci.sh && ci_tm4c123_setup
   ```
3. Build:
   ```bash
   cd ./ports/tm4c123
   make
   ```
   
### Windows

1. Install [MinGW]( https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/installer/mingw-w64-install.exe/download)  
      **Make sure it is added to your PATH**  
   **OR** install [WSL](https://docs.microsoft.com/de-de/windows/wsl/install-win10#manual-installation-steps)
2. Install [ARM toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads)  
      **Make sure it is added to your PATH**
3. Clone git repo and checkout branch `tiva_from_stable`
4. Download and install [TivaWare](https://www.ti.com/tool/SW-TM4C) or extract contents to `micropython/ports/tm4c123/tivaware`  
   **OR** if you are using WSL, do Step 2 of [Ubuntu Instructions](#ubuntu)  
5. Build using WSL or MinGW terminal:
   ```bash
   cd ./ports/tm4c123
   make
   ```
   Use param `TIVAWARE_LIB="C:\ti\TivaWare_2.2.0.XXX" ` with make if you installed TivaWare
   
## Flashing

1. Download & install UniFlash: https://www.ti.com/tool/UNIFLASH 
   - Create new Configuration
   - Device: EK-TM4C123GXL
   - Connection: Stellaris In-Circuit Debug Interface
   - Start
   - Load Flash image: `build/firmware.bin`
   - Reset Actions: Core Reset, Check `Execute selected reset after program load`
   - Load Image

## Debug

### CCS

   * Download CodeComposerStudio and import the source code as external project
   * Configure the debugger to load the uPY_TM4C123.axf

### VSCode

1. Download and install Visual Studio Code with these extensions:
   - ARM Code from Dan C Underwood
   - Cortex-Debug from marus25
   - Python 
   - C/C++
   - Native Debug from WebFreak
   - LinkerScript from ZixuanWang

2. Download & install drivers: https://www.ti.com/lit/zip/slac632  
   (Windows only, also included in the TivaWare)
   - connect board
   - right click Start button and click Device-Manager
   - there should be 2 unknown devices
   - Click update drivers and use manual select
   - navigate to the top level folder containing the drivers
   - Windows selects the driver on its own
   - repeat for other device
   
3. Install OpenOCD
   - Windows/WSL: https://gnutoolchains.com/arm-eabi/openocd/  
      **Add it to your PATH**
   - Ubuntu/Debian: `sudo apt install openocd`
   
4. In VSCode debugging menu, click new config and replace contents of your `launch.json` with this:
   ```json
   {
      "version": "0.2.0",
      "configurations": [
         {
            "name": "Cortex Debug",
            "cwd": "${workspaceRoot}",
            "executable": "./build/firmware.axf",
            "request": "launch",
            "type": "cortex-debug",
            "servertype": "openocd",
            "runToEntryPoint": "tm4c_main",
            "configFiles": [
                "board/ek-tm4c123gxl.cfg"
            ]
         }
    ]
   }
   ```
      **Make sure to set the `configFiles` path correctly.**
      
 5. Start the debugging (after building the project)

## Connecting to REPL

You can connect to the REPL via UART over USB. You need to install the drivers for this to work!

Use these settings:
* Baudrate = 115200
* Data bits = 8
* Stop bits = 1
* Parity = none
* Flowcontrol = none

I recommend using **minicom** on Linux or **PuTTY** on Windows.

Alternatively you can use [**mpfshell**](https://github.com/wendlers/mpfshell):
```bash
python -m pip install mpfshell
```

## Usage

Currently, these Modules are available:
* umachine
  * Pin
  * SPI
  * UART
  * SDCard
  * IRQ
  * DMA
* uos
---
### SD-Card

Connect the SD-Card in SPI-mode according to this table:  

| Board Pin | SD-Pin (SPI / SDIO)    |
| --------: | :--------------------- |
|       PE4 | Card Detect (CD / DET) |
|       PA2 | Clock (SCK / CLK)      |
|       PA3 | Chip Select (CS / D3)  |
|       PA4 | Data Out (MISO / D0)   |
|       PA5 | Data In (MOSI / CMD)   |

---
### Pin

See also: 
<http://docs.micropython.org/en/latest/library/machine.Pin.html>

    machine.Pin(id, mode, pull, *, value, drive, alt)

| Parameter | Function                                         | value range                                  | default |
| --------: | :----------------------------------------------- | :-------------------------------------------- | :----------- |
|        id | Pin ID as Number, String or Tuple \[port, pin\] | P\[A..F\]\[0..8\]                             | \-           |
|      mode | Pin mode                                        | IN, OUT, OPEN\_DRAIN, ALT                     | \-           |
|      pull | PullUp or  Down                                 | PULL\_NONE, PULL\_UP, PULL\_DOWN, OPEN\_DRAIN | PULL\_NONE   |
|     value | Pin value                                       | 0, 1                                          | 0            |
|     drive | Strength of the pad driver                          | LOW\_POWER, MED\_POWER, HIGH\_POWER           | LOW\_POWER   |
|       alt | ID of the alternative function                    | 0..15                                         | \-           |

#### Special function

    <Pin>.unlock() 

This function unlocks special pins on the Launchpad. This needs to be done to reassign another AF to these pins.
\<Pin\> is the object of the pin you wish to unlock.  
If you try to change a locked pin, you will see an error message.
---
### UART

See also:
<http://docs.micropython.org/en/latest/library/machine.Pin.html>



    machine.UART(id, baudrate, bits, parity, stop, *, ...)

|      Parameter | Function                            | value range | default |
| -------------: | :---------------------------------- | :----------- | :----------- |
|             id | UART ID als Zahl                    | 0..7         | \-           |
|       baudrate | Baudrate                            | bis 5Mbits   | \-           |
|           bits | word length                           | 5..8         | 8            |
|         parity | parity type                        | ODD, EVEN    | NONE         |
|           stop | stop bits                           | 1, 2         | 1            |
|           flow | flowcontrol (currently broken)                        | ~~RTS, CTS~~     | NONE         |
|        timeout | timeout in ms  | >= 0    | 10           |
|  timeout\_char | timeout per char in ms | >= 0    | 0            |
| read\_buf\_len | read buffer size               | 0    | 64           |

UART0 is used by the command line. Changing this unit may break the command line  
UART1 is currently not available
---
### SPI

See also:
<http://docs.micropython.org/en/latest/library/machine.SPI.html>

    SPI(id, mode, *, baudrate, polarity, phase, protocol, bits, firstbit, fss, dma, sck, mosi, miso)

|       Parameter | Function                        | value range               | default |
| --------------: | :------------------------------ | :------------------------- | :----------- |
|              id | Unit number           | 0..3                       | 0            |
|            mode | SPI Mode                       | MASTER, SLAVE, SLAVE\_OD   | MASTER       |
|        baudrate | Clock rate    | \<25M Master, \<6e6 Slave | 5e5          |
|           phase | SPI phase       | FIRST\_EDGE, SECOND\_EDGE  | FIRST\_EDGE  |
|        polarity | SPI polarity      | IDLE\_LOW, IDLE\_HIGH      | IDLE\_LOW    |
|        protocol | SSI protocol                       | SPI0..SPI3, MICROWIRE, TI  | SPI0         |
|            bits | word length                       | 4..16                      | 8            |
|        firstbit | bit order | MSB, LSB                   | MSB          |
|             fss | chip select/ frame control      | FSS\_SOFT, FSS\_HARD       | FSS\_HARD    |
|             ~~dma~~ | use DMA? (not implemented)              | ~~True, False~~                |     False         |
| ~~sck, mosi, miso~~ | (not implemented)       |                            |              |
 
If *protocol* is given, *phase* and *polarity* will be **ignored**!
