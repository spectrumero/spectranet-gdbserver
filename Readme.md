# spectranet-gdbserver

A Hardware Debugger for your ZX Spectrum.
It allows you to debug speccy on "C" level with help of
[z88dk-gdb](https://github.com/z88dk/z88dk).
It requires [Spectranet](https://www.bytedelight.com/?page_id=3515) because it operates over the network.

## How does it work?
It's a Spectranet module, once installed, 
a BASIC extension `%gdbserver` becomes available.

1. When such command is run, it overrides NMI button handler. 
2. When NMI is pressed, it takes over and awaits for z88dk-gdb to connect.
The NMI button could be pressed mid-execution of your program to examine stuff.
3. z88dk-gdb can do pretty much anything expected from a gdb-alike client.

## How To Build

Have z88dk installed, and then do `make`. 

Beware that this requires latest Spectranet firmware with necessary fixes!
Then using Spectranet's `ethup` do this to install it on the hardware:

`ethup <speccy ip> build/gdbserver` 

This will install a gdbserver module. Reboot the machine. Done.

# Screenshots

![](images/command.png) 
![](images/run.png)

![](images/z88dk-gdb.png)