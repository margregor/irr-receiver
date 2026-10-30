This project is an attempt to repurpose the ir reveiver of the Tevion DSS 2100 center speaker.
The center speaker from the Tevion DSS 2100 speaker set has an ir receiver that would normally be connected to the main unit via a standard 3.5 mm TRS jack.
The dev board in use is a Sipeed Tang Nano 20k.
The clock used is the Tang Nano's onboard pll configured to 27MHz.

Connections

Jack Breakout Ring <--> Tang Nano 20k Pin 31

Jack Breakout Tip <--> 3.3V

Jack Breakout Sleeve <--> Ground

If using anything other than the Tevion DSS 2100 center speaker verify what connections you need to make.

## IR receiver UART

The FPGA samples the receiver on pin 31. When it sees a message, it writes a line to its UART at 115200 baud:

```
NEC XXXXXXXX
```

Before building set GOWIN_IDE and GOWIN_PROG paths in the Makefile to wherever you have them installed.

To build it run
```powershell
make
```
To program the fpga run

```powershell
make flash # persistent
```
or 

```powershell
make flash-sram # volatile
```

## Windows volume control

In `apps` is a windowless native Windows listener. It
recognizes codes sent by a remote from Sven HT-200. It maps `VOL+` and `VOL-`
to the Windows volume controls, the Center Start/Pause button to the
Windows Media Play/Pause key, and the number buttons to numpad keys. It reconnects automatically when the FPGA is
unplugged and later plugged back in. 

Build it with Meson from `apps` directory:

```powershell
meson setup build
meson compile -C build
```

The resulting executable is `apps/build/irremote-volume.exe`. It adds an icon
to the system tray: hover it to see COM6 connection status and
right-click it to exit.

You will probably need to change the COM port and button mapping to whatever it is for your setup and remote.
