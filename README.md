# attiny85_dice

I started this project to learn more about how to design a PCB from scratch, so I wrote two different sets of dices in order to flash them into a couple of attiny85's.

- **d1ce** is a single d6 dice that uses 7 leds to show each dot. A **standard 8-bit shifter** is used to control the different sides
- **dic3** is a three-dice-pack that uses different led segments to display numeric values up to 99. It uses a **max7219** to control the numbers and it allows different sides for each dice (so you can "roll" one d20 and two d8, for example)

Both work in a *similar* way:

1. There's a timer running non-stop incrementing a *seed value* that it's used as a seed for the random functions
2. On each "roll" the seed is changed to the current *seed value* and a random value (or values) is picked
3. A second timer handles the "animation" of a dice rolling (or numbers changing)
4. The final value is shown

This was one of my first approach on timers and interrupts for the AVRs micros, so I'm sure there are lots of improvements that can be made.

Maybe using a timer to set a random seed is not the best way to handle it, but for simple picks for a dummy dice I think it's enough; and to prevent having only one "initial value", the seed changes on every roll (otherwise -if we set the seed **only** on the beginning, for example- it's more likely to have two "equal dices" only by matching that initial roll).

## d1ce
The d6 dice was the first one and it's a single *dice.c* file that handles all the interrupts, timers and 8-bit shifter operations (three pins on that: *data*, *clock* and *latch*).

There's also a couple of schematic files and the gerbers I used to order the board on one of those external prototyping services. The PCB seems to work fine, but I made a small mistake when setting the power switch, so it's not properly aligned to the right (but it's fully functional).

## dic3
The three-in-one dice it's similar to the single d6 one (it's based on the same code) but there's an aditional **max7219drv.c** file that implements a couple of methods to control the max7219 that handles the three numeric led displays.

It also has a "config mode" with a secondary button that allows the user to individually change the dice values.

Here there's only the schematic files but not the gerbers, since the PCB I made uses a space for a *2032 battery holder* (a small 3V coin battery, the same as the one used on **d1ce**) but the requirements for the **max7219** demands 5v (more or less). I totally forgot about this, so in return I have a fully functional PCB that needs to be hooked up to an external 5v source or, at least, do some creativity with other components to make it work :_ D

## Useful links
[https://embeddedthoughts.com/2016/06/06/attiny85-introduction-to-pin-change-and-timer-interrupts/](https://embeddedthoughts.com/2016/06/06/attiny85-introduction-to-pin-change-and-timer-interrupts/) Some interrupts and timers info

[https://tinusaur.org/2019/01/06/interfacing-a-max7219-driven-led-matrix-with-attiny85/](https://tinusaur.org/2019/01/06/interfacing-a-max7219-driven-led-matrix-with-attiny85/) **max7219** with attiny85

[https://docs.kicad.org/5.1/en/getting_started_in_kicad/getting_started_in_kicad.html](https://docs.kicad.org/5.1/en/getting_started_in_kicad/getting_started_in_kicad.html) I followed the "Getting started" section from the **KiCad docs** to learn the basics about the whole process of designing and making the boards

