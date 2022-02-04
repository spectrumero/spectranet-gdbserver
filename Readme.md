# spectranet-gdbserver

A Hardware Debugger stub for your ZX Spectrum.

It allows you to debug speccy on "C" level with help of
[z88dk-gdb](https://github.com/z88dk/z88dk).

It requires [Spectranet](https://www.bytedelight.com/?page_id=3515) because it operates over the network.

# Emulation

Despite Spectranet requirement, it can be run on
[Fuse](https://github.com/speccytools/fuse/releases/tag/gdbserver) emulator.
Note that the link refers to fork that supports gdbserver stub on its own,
so for emulation with Fuse this might be unnecessary.

# Installation

Beware that this requires latest Spectranet firmware with necessary fixes!

### Upgrade Specranet First

1. Go to [Releases](https://github.com/speccytools/spectranet-gdbserver/releases), download `spectranet-patched.firmware` file.
2. Download [Ethup tool](http://spectrum.alioth.net/doc/index.php/Ethup)
3. Launch your spectrum (with spectranet), press the NMI button
4. Select "C"
5. Upload the binary usint ethup: `ethup <spectrum ip> spectranet-patched.firmware 27000`
6. Exit by pressing `X`
7. Enter `RANDOMIZE USR 27000`

Spectranet firmware is upgraded. Note that once
[this pull request](https://github.com/spectrumero/spectranet/pull/16)
is merged, you can go back to vanilla firmware.

### Upload gdbserver module

1. Go to [Releases](https://github.com/speccytools/spectranet-gdbserver/releases), download `spectranet-gdbserver.module` file.
2. Download [Ethup tool](http://spectrum.alioth.net/doc/index.php/Ethup) if you haven't already.
3. Launch your spectrum (with spectranet), press the NMI button
4. Select "B", then select "A"
5. Upload the binary usint ethup: `ethup <spectrum ip> spectranet-gdbserver.module`
6. Restart spectrum

Done, you can launch the gdbserver using basic `%gdbserver` command.

Seeing an error? Make sure you've upgraded your spectranet first!

# How does it work?
It's a Spectranet module, once installed, 
a BASIC extension `%gdbserver` becomes available.

1. When such command is run, it overrides NMI button handler. 
2. When NMI is pressed, it takes over and awaits for z88dk-gdb to connect.
The NMI button could be pressed mid-execution of your program to examine stuff.
3. z88dk-gdb can do pretty much anything expected from a gdb-alike client.

# How To Build Form Source

Have z88dk installed, and then do `make`. 

Then using Spectranet's `ethup` do this to install it on the hardware:

`ethup <speccy ip> build/gdbserver` 

This will install a gdbserver module. Reboot the machine. Done.

# Screenshots

![](images/command.png) 
![](images/run.png)

![](images/z88dk-gdb.png)