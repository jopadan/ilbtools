# [ilbtools](http://github.com/jopadan/ilbtools)
Collection of ILB format image tools applicable to Age of Wonders 1 

- [Age of Wonders Heaven Forum](https://aow.heavengames.com/cgi-bin/forums/display.cgi?action=ct&f=14,2666,,60)
Topic for everything about ilbtools

- [Age of Wonders Manual Addendum](https://github.com/HiPhish/Game-Source-Documentation/blob/master/Age%20of%20Wonders/Addendum.rst)
Manual addendum
  
## About

The ilbtools source code is based on the information provided by the following sources

- [ilb2png](http://github.com/socks-the-fox/ilb2png)
Converts the Image Library format for Age of Wonders 1 to a set of PNGs

- [dumpilb](http://www.jongware.com/aow/binaries/dumpilb.zip)
Dumps information of ILB (Image Library format) files to stdout

- [aow-patch](http://www.github.com/int19h/aow-patch)
This *unofficial* patch for Age of Wonders 1.36 tentatively fixes the "Exception during MapViewer.ShowScene" 

- [Age of Wonders 1: The ILB Format](http://www.jongware.com/aow/aow1.html)
General information about the ILB format and example implementations of it.

## Build

```c
autoreconf -i -s
./configure
make install
```

## Usage

```c
ilb2png <image.ilb>
```

```c
dumpilb <image.ilb>
```

```c
aowpatch Ilpack.dpl
```
