# Hackaday Belgrade Badge Official Repository

This repository contains the hardware and firmware files for the
[Hackaday | Belgradge](https://hackaday.io/belgrade/) conference badge.

This conference was hosted in Belgrade, Serbia on April 9th, 2016. The
conference badge is the work of Voja Antonic and was the subject of a
demoscene contest that encourage people to write code that shows off
the hardware in new and interesting ways (or in the case of those at
the conference, they could choose to augment the hardware as well).

Read [more about badge development](https://hackaday.io/project/9509-badge-for-hackaday-belgrade-conference).

## Firmware

The firmware is written in assembly language. The first 0x1000 memory locations are occupied by a USB bootloader. User
code should be compiled to start at 0x1000. The device enters bootloader mode when restarted while attached
via USB. Please see the linked development project page above for more information.

## Hardware

The PCB folder contains layout information for two different hardware designs of the badge.
One is based on using common Anode displays, the other uses common Cathode displays. The EDA software
utilized in this project is "Client 98" (may also be called Protel 98).

## Badge Manual

Voja has written a draft of a user manual for this badge. It is a PDF file located in the same directory as this readme.
