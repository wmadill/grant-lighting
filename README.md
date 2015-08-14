# grant-lighting

## Description
This repository contains the JeeNode and BlinkM code that controls the
lighting for paintings displayed on low benches.

A full description is in the HTML file.

## Architecture
The BlinkMs are programmed with light scripts that vary the color and
intensity of the LEDs. The BlinkM controllers on each bench are
synchronized by a JeeNode. Each JeeNode communicates via its built-in
radio with the ones on the other benches to coordinate the light
sequence across all the benches.

Because of the power requirements of the LEDs on a bench, they are
powered by a sealed lead acid battery under the bench. The JeeNodes
run in a very low power mode and use a single AA battery for power.
The design goal is to run the benches at least one month from a
set of batteries.

## Hardware
### JeeNode
JeeNodes are a low power derivative of an Arduino using the same
microprocessor but with a different pinout. Its design goals include
low powered autonomous operation with multiple sensors.

### BlinkM
BlinkMs are an integrated LED cluster (red, green, blue) with a 
microcontroller running a "light script" that controls the intensity
of each LED over time. 

