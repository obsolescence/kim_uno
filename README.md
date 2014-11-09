kim_uno
=======

KIM Uno is a KIM-1 emulator and 6502 programmable calculator for Arduino.
This code is intended for the KIM Uno PCB, but actually runs on any Arduino even without the PCB.

Please check the website for documentation and news:
http://obsolescence.wix.com/obsolescence#!kim-uno-summary/c1uuh

Notes:
- subdirectory KIMUNO contains the Arduino source code.
- subdirectory KIMPC contains a PC version used during debugging.
  it compiles under any plain C compiler and gives the same functionality as the
  Arduino version, except that the 6502 Programmable Calculator mode is more primitive.
  This is code only used during development, but maybe it's of some use to someone.
  note that the cpu.c is the same as for the Arduino version! Only the emulator wrapper is different.
- This is not elegant programming. I'm a tinkerer, not a developer. :)
