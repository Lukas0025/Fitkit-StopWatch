# Fitkit-StopWatch

* login: xpleva07 AT stud.fit.vutbr.cz
* project variant: B - LIBOVOLNÝ KIT: Stopky

this is documenation in markdown. Cezech PDF version is in doc.pdf file.

## Project objectives 

* Start/pause button
* Restart button
* space for 9 saved times
* pageing between saved times
* time save button

## Control

Using firkit 4x4 keyboard. Keymapping is:

| KEY | Command                                                                       |
| --- | ----------------------------------------------------------------------------- |
|  A  | (Activate) Start\Pause counting                                               |
|  B  | Next page with saved times (Time 0 is main) rounded using mod 10              |
|  #  | Restart stopwatch (Restart all saved times, main time and stop counting)      |
|  *  | Save current time (can be saved maximaly 9 times) when saved success * else E |

## Implementation

libraries:
* fitkitlib
* mcu/libs/keyboard/
* mcu/libs/lcd/

peripheris:
* TIMER A (MCU)
* SPI_adc (FPGA - VHDL) 2x
* keyboard_controller (FPGA - VHDL)
* lcd_raw_controller (FPGA - VHDL)
* keyboard 4x4 (fitkit)
* LCD disply (fitkit)

Timer A is used as periodic iterupt signal with period 1/64s. Every interupt is 1 added to clock variable wihich is implementing current time [s] from start in fixed point format (last 6 bits is fractional). After addtion is complete new time is displayed on LCD screen using support functions. Led d5 is used as heart beep of this counter. This happens only when counting state varaible is true else nothing happens. Button A inverting counting state varaible and set cuttent screen (page) back to 0. Button B work only when not counting and change current page to (page + 1) mod 10. Page is used for show saved times, page 0 is page with actiual counting time. This saved times is readed from array of size 10 unsiged 32 bits numbers, where number under number 0 is reserved for current counting time. Buttons handling is done using active waiting using while. When button is pressed program firsty test is it not a vibration of enviroment or someting else. If not program do subrutine of button and then wait for button release.

## conclusion

Apliaction is implementing Project objectives and working, but some aspects is not implemented as good as it can be. Keyboard hadling can be done using iterupts to save microcontoller computiong time and save some power. Time when microcontroller only waiting for next timer interupt or key press can microcontroller be in sleep mode.