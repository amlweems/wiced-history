=====================================================================
Broadcom WICED Software Development Kit 2.4.1 - README
=====================================================================

The WICED SDK provides a full compliment of application level APIs, 
libraries and tools needed to design & implement secure embedded wireless
networking applications. 

Major features of the WICED SDK include ...
  - Low-footprint embedded Wi-Fi Driver with Client (STA), softAP and Wi-Fi Direct
  - Wi-Fi <-> Bluetooth SmartBridge 
  - Various RTOS/TCP stack options including
    - ThreadX/NetX (IPv4), ThreadX/NetX Duo (IPv6), FreeRTOS/LwIP (IPv4)
  - Various MCU host platforms
    - STM32F1xx, STM32F2xx, STM32F4xx, AT91SAM4S16B, Freescale K60
  - RTOS & Network abstraction layer with a simple API for UDP, TCP, HTTP, HTTPS communications
  - SSL/TLS Security Library integrated with an HTTPS library for secure web transactions
  - WICED Application Framework including Bootloader, OTA Upgrade and Factory Reset 
  - Automated Wi-Fi Easy Setup using one of several methods
    - SoftAP & Secure Webserver, Wi-Fi Protected Setup, Apple MFi iAP via Bluetooth, Cooee(TM)
  - Simple API to provide access to MCU peripherals including UART, SPI, I2C, Timers, RTC, ADCs, DACs, etc
  - Support for multiple toolchains including GNU and IAR 
 
The WICED SDK release is structured as follows:
  Apps          : Example & Test Applications
  Doc           : API & Reference Documentation, Eval Board & Module Schematics
  Drivers       : Drivers for WICED evaluation boards
  Include       : WICED API, constants, and defaults 
  Library       : Daemons, servers, protocols and peripheral libraries
  Resources     : Resources used by the WICED webserver including scripts, styles, images and HTML.
  Tools         : Build tools, compilers, debugger, programming tools etc.
  Wiced         : WICED core components (RTOS, Network Stack, Wi-Fi Driver, Security & Platform definitions)
  Wiced/WWD     : The WICED Wi-Fi Driver (equivalent to the Wiced directory in previous SDK-1.x releases) 
  README.txt    : This file
  CHANGELOG.txt : A log of changes for each SDK revision
 

Getting Started
---------------------------------------------------------------------
If you are unfamiliar with the WICED SDK, please refer to the 
WICED Quickstart Guide located here: <WICED-SDK>/Doc/WICED-QSG2xx-R.pdf
The WICED Quickstart Guide documents the process to setup a computer for
use with the WICED SDK, IDE and WICED Evaluation Board. 

The WICED SDK includes lots of sample applications in the <WICED-SDK>/Apps directory.
Applications included with the SDK are outlined below.
 Apps/demo : Demonstration Applications
   - Applications demonstrating how to integrate various WICED API features 
 Apps/snip : Application Snippets
   - Various applications to demonstrate usage of individual WICED APIs          
 Apps/test : Test & Utility Applications
   - console      : Provides various test features including Iperf for throughput testing 
   - mfg_test     : Manufacturing Test application to enable radio performance and certification testing
 Apps/waf  : WICED Application Framework
   - bootloader   : Bootloader application used in conjunction with the WICED Application Framework
   - ota_upgrade  : Over the Air Upgrade application
   - sflash_write : Serial flash library used to configure a serial flash for factory reset  
 Apps/wwd : Wiced Wi-Fi Driver Applications to demonstrate advanced usage of the low layer Wi-Fi driver
    
To obtain a complete list of build commands and options, enter the following text in the
base WICED SDK directory on a command line:
$> make

To compile, download and run the Wi-Fi scan application on the Broadcom BCM943362WCD4 evaluation platform, 
enter the following text on a command line (a period character is used to reference applications 
in sub-directories) :
$> make snip.scan-BCM943362WCD4 download run

Note : The RTOS, Network Stack & I/O bus components are now defined in a platform makefile in 
       the <WICED-SDK>/Wiced/Platform/<PLATFORM_NAME> directory. The default components may 
       be bypassed by specifying the component as part of the build string if desired.
       
Header files and reference information for supported platforms is available 
in the <WICED-SDK>/include/platforms directory.
Platform implementations are available in the <WICED-SDK>/Wiced/Platform directory.


Supported Features
---------------------------------------------------------------------
Wi-Fi & Bluetooth SmartBridge Features
 * Scan and associate to Wi-Fi access points
 * Authenticate to Wi-Fi Access Points with the following security types:
   Open, WEP-40, WEP-104, WPA (AES & TKIP), WPA2 (AES, TKIP & Mixed mode)
 * AP mode with support for security types : Open, WPA, WPA2
 * Concurrent AP & STA mode (AP mode limited to 5 concurrent connected clients)
 * Wi-Fi Direct
 * WPS 1.0 & 2.0 Enrollee & Registrar (Internal Registrar only)
 * Wi-Fi APIs : Network keep alive, packet filters
 * Host <-> Wi-Fi SDIO & SPI interface
 * Bluetooth SmartBridge with multiple connections including the
   following features: Whitelist, Bond Storage, Attribute Caching, 
   GATT Procedures, Configurable Maximum Concurrent Connections, Directed 
   Advertisements, Device address initialisation

RTOS & Network Stack Support
 * FreeRTOS / LwIP    (full source)
 * ThreadX  / NetX    (object file; free for use with WICED *ONLY*)
 * ThreadX  / NetXDuo (object file; free for use with WICED *ONLY*)

Networking Features (IPv4 & IPv6)
 * ICMP (Ping)
 * ARP
 * TCP
 * UDP 
 * IGMP (Multicast)
 * IPv6 NDP, Multicast
 * DHCP (Client & Server)
 * DNS (Client & Redirect Server)
 * mDNS/DNS-SD Zeroconf Network Discovery (Broadcom Gedday)
 * SSL3.0 & TLS1.0/1.1 (object file with host abstraction layer; free for use with WICED *ONLY*)
 * HTTP / HTTPS (Client & Server)
 * SNTP
 * SMTP

Application Features
 * Peripheral interfaces
   * GPIO
   * Timer / PWM
   * UART
   * SPI
   * I2C
   * RTC (Real Time Clock)
 * Xively "Internet of Things" protocol

* WICED Application Framework
   * Bootloader
   * Device Configuration Table (region in flash to store AP, security credentials, TLS certs, serial number, Wi-Fi country code, etc)
   * OTA upgrade
   * Factory reset
   * Automated configuration via softAP & webserver
   * Wi-Fi Easy Setup
     * Cooee (BETA)
   * Support for Wi-Fi configuration using MFi iAP via Bluetooth
     The MFi iAP library is available to Apple MFi licensees directly from Apple
   * System Monitor to manage the watchdog

Toolchains
 * GNU make
 * IAR

Hardware Platforms
 * BCM9WCD1EVAL1  : Bare WCD1 WICED evaluation board (generic module)
 * BCM943362WCD2  : Broadcom 43362-based WICED Module with STM32F103 MCU mounted on BCM9WCD1EVAL1
 * BCM943362WCD4  : Broadcom 43362-based WICED Module with STM32F205 MCU mounted on BCM9WCD1EVAL1
 * BCM943362WCD6  : Broadcom 43362-based WICED Module with STM32F415 MCU mounted on BCM9WCD1EVAL1
 * BCM943362WCD8  : Broadcom 43362-based WICED Module with ATSAM4S16B MCU mounted on BCM9WCD1EVAL1
 * BCM9WCDUSI09   : Broadcom 43362-based WICED Module with STM32F205 MCU (includes WM-N-BM-09 WICED SiP) mounted on BCM9WCD1EVAL1
 * BCMUSI11       : USI 43362-based WICED+ Module (STM32F205 MCU, 8Mbit serial flash) mounted on BCM9WCD1MFI1
 * BCM9WCDPLUS114 : WICED+ Eval Board (includes BCM43362+STM32F205 WICED+ Module and BCM20702 Bluetooth module)
 * TWRK60D100M    : Freescale Kinetis K60 connected to a USI WM-N-BM-09 WICED SiP via SPI  


Known Limitations & Notes
---------------------------------------------------------------------

 * Platform Limitations
   -----------------+-----------+-----------+-----------+-----------+-----+ 
   Platform Feature | STM32F1xx | STM32F2xx | STM32F4xx | AT91SAM4S | K60 |
    Implementation  |           |           |           |           |     |
   -----------------|-----------+-----------+-----------+-----------+-----+
   MCU powersave    |     Y     |     Y     |     N     |     Y     |  N  |
   Wi-Fi Powersave  |     N(1)  |     Y     |     Y     |     Y     |  N  |
   I2C API          |     Y     |     Y     |     N     |     N     |  N  |
   ADC/PWM API      |    Y/Y    |    Y/Y    |    Y/Y    |    Y/N    |  N  |
   OTA upgrade      |     N     |     Y     |     N     |     N     |  N  |
   Real Time Clock  |     N     |     Y     |     N     |     N     |  N  |
   -----------------+-----------+-----------+-----------+-----------+-----+  
          
   * BCM943362WCD2 Platform Restrictions
       The STM32F103 MCU on this platform only has 64kB RAM and 512kB Flash.
       Many applications that include more advanced networking features
       will NOT run on this platform! Either the application will not fit into
       Flash, or the application may run out of RAM at runtime and hang.
       Tips to use this platform:
         - Store the Wi-Fi firmware in external serial flash (or use the Wi-Fi 
           firmware inside the Factory Reset image in serial flash)
         - Do not use advanced networking features like TLS & mDNS      
         - Do not build applications using debug mode     
     
   * Wi-Fi Powersave (1)
       The WLAN chip requires an external 32kHz sleep clock input during powersave.
       Platforms that do not support Wi-Fi powersave (per the table above) are
       not capable of driving the WLAN sleep clock. An external 32kHz clock is 
       required for these platforms.
 
 * Wi-Fi Powersave Max. Sleep Time
   The WLAN firmware sleep time is limited to a maxium of 4 second. Attempts to 
   set the Wi-Fi listen interval longer than 4 second using the 
   wiced_wifi_set_listen_interval() function will be ignored. 
 
 * libc does not include support for printing uint64_t (long long)
   
 * RTOS detection may cause OpenOCD to crash in the following situation:
     GDB has an elf containing a known RTOS loaded, and your app is using the 
     RTOS memory for other purposes. This situation may occur while debugging 
     the bootloader
     SOLUTION : Remove " -rtos auto " from the <WICED-SDK>/Tools/OpenCD/OpenOCD 
                .cfg file that matches your hardware debugger 
                (ie. BCM9WCD1EVAL1.cfg for WICED Evaluation Boards) 

 * Support for IAR toolchain is available for STM32F2xx platform only (support for other platforms TBD)

 * AP mode when running with WPA/WPA2 encryption is limited to 4 STA clients

Tools
---------------------------------------------------------------------
The GNU ARM toolchain is from Yagarto, http://yagarto.de

Programming and debugging is enabled by OpenOCD, http://openocd.berlios.de

The standard WICED Evaluation board (BCM9WCD1EVAL1) provides two physical 
programming/debug interfaces for the STM32 host microprocessor: USB-JTAG and direct 
JTAG. The WICED Evaluation board driver additionally provides a single USB-serial 
port for debug printing or UART console purposes.

The USB-JTAG interface is enabled by the libftdi/libusb open source driver,
http://intra2net.com/en/developer/libftdi. 
The direct JTAG interface works with third party JTAG programmers including 
Segger, IAR J-Link and Olimex ARM-USB-TINY series. OpenOCD works with the libftdi/libusb 
USB-JTAG driver shipped with the WICED SDK and commercially available JTAG drivers 
available from third party vendors.

Building, programming and debugging of applications is achieved using either a 
command line interface or the WICED IDE as described in the Quickstart Guide.

Instructions to use the IAR toolchain are provided in a README located in the 
following directory: <WICED-SDK>/Tools/IAR 

                     
WICED Technical Support
---------------------------------------------------------------------
WICED support is available on the Broadcom forum at http://forum.broadcom.com/forum.php 

Sign-up is a two-step process: Firstly, sign up to the general Broadcom support 
forum, then apply to be a member of the WICED User Group. Be sure to identify 
yourself to the forum moderator, access to the WICED forum is restricted to 
bona-fide WICED customers only.

Broadcom provides customer access to a wide range of additional information, including 
technical documentation, schematic diagrams, product bill of materials, PCB layout 
information, and software updates through its customer support portal. For a CSP account, 
contact your Broadcom Sales or Engineering support representative.

                     
Further Information
---------------------------------------------------------------------
Further information about WICED and the WICED Development System is
available on the WICED website at http://broadcom.com/wiced or
by e-mailing Broadcom support at support@broadcom.com
