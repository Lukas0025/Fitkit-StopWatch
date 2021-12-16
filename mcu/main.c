/*******************************************************************************
   main.c: StopWtach for fitkit
   Copyright (C) 2021 Brno University of Technology,
                      Faculty of Information Technology
   Author(s): Lukas Plevac <xpleva07 AT stud.fit.vutbr.cz>

   LICENSE TERMS

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
   3. All advertising materials mentioning features or use of this software
      or firmware must display the following acknowledgement:

        This product includes software developed by the University of
        Technology, Faculty of Information Technology, Brno and its
        contributors.

   4. Neither the name of the Company nor the names of its contributors
      may be used to endorse or promote products derived from this
      software without specific prior written permission.

   This software or firmware is provided ``as is'', and any express or implied
   warranties, including, but not limited to, the implied warranties of
   merchantability and fitness for a particular purpose are disclaimed.
   In no event shall the company or contributors be liable for any
   direct, indirect, incidental, special, exemplary, or consequential
   damages (including, but not limited to, procurement of substitute
   goods or services; loss of use, data, or profits; or business
   interruption) however caused and on any theory of liability, whether
   in contract, strict liability, or tort (including negligence or
   otherwise) arising in any way out of the use of this software, even
   if advised of the possibility of such damage.

   $Id$


*******************************************************************************/

#include <fitkitlib.h>
#include <keyboard/keyboard.h>
#include <lcd/display.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

uint32_t timer_sec[10]; // in fixed point last 6 bin numbers is unber point
                        // log 2 64
bool     counting;      // current system state
uint8_t  page;          // current page
uint8_t  w_page;        // current page for write
char     prefix;        // prefix for write
uint8_t  prefix_timer;  // timer for prefix

unsigned i;

#define SHIFT_AMOUNT 6
#define SHIFT_MASK ((1 << SHIFT_AMOUNT) - 1) 
#define TIMER_MAX 5999 << SHIFT_AMOUNT | SHIFT_MASK //99*60 + 59 => 99:59.MM

/**
 * @brief Called when help command sended from PC
 */
void print_user_help(void) {
    term_send_crlf();
    term_send_str("Application stopwatch for fitkit");
    term_send_crlf();
    term_send_crlf();
    term_send_str("keys:");
    term_send_crlf();
    term_send_str("    A    Start/Pause stopwatch");
    term_send_crlf();
    term_send_str("    B    Next saved time (in circle mod 10)");
    term_send_crlf();
    term_send_str("    #    restart stopwatch");
    term_send_crlf();
    term_send_str("    *    save current time is success it disply * else E");
}

/**
 * @brief Render time on disply in format mm:ss.ss
 * 
 * @param time time to render in fixed point (last 6 bits is under point)
 * @param prefix prefic char to print before time (0 for not print prefrix)
 */
void render_time(uint32_t time, char prefix) {
    char str[12];

    uint16_t time_ms = (time & SHIFT_MASK) * 100 / (1 << SHIFT_AMOUNT); //two decimal places
    uint16_t time_s  = time >> SHIFT_AMOUNT;
    uint16_t time_m  = time_s / 60;
    time_s           = time_s % 60;

    if (prefix) {
        sprintf(str, "%c.  %02u:%02u.%02u", prefix, time_m, time_s, time_ms);
    } else {
        sprintf(str, "    %02u:%02u.%02u", time_m, time_s, time_ms);
    }
    
    LCD_write_string(str);
}

/**
 * @brief Restart state of stopwatch
 * 
 */
void stopwatch_restart() {
    counting = false;
    
    timer_sec[0] = 0;

    w_page = 1;
    page   = 0;

    render_time(timer_sec[0], '\0');
}


/**
 * @brief Steni z kalvestnice
 * @return int 
 */
int keyboard_idle() {
    char ch;
    ch = key_decode(read_word_keyboard_4x4());

    if (ch != 0) {

        //is valid key press? no vibration or etc
        delay_ms(50);
        if (key_decode(read_word_keyboard_4x4()) != ch) {
            return 0; // enviroment vibration
        }

        // start/pause action
        if (ch == 'A') {

            counting = !counting;
            page     = 0;

            //render front page
            render_time(timer_sec[page], 0);

        // next page action (Next saved time)
        } else if (ch == 'B') {

            if (!counting) {
                page = (page + 1) % w_page;
            }

            char page_ch = page;
            if (page != 0) {
                page_ch += 48;
            }

            render_time(timer_sec[page], page_ch);

        // save current time action
        } else if (ch == '*') {

            if (counting && !prefix_timer) {

                prefix_timer = 20;

                if (w_page < 10) {
                    timer_sec[w_page] = timer_sec[0];
                    prefix = '*';
                    w_page++;
                } else {
                    prefix = 'E';
                }
            }

        //restart action
        } else if (ch == '#') {
            stopwatch_restart();
        }

        //wait for button relese
        do {
            delay_ms(10);
        } while (key_decode(read_word_keyboard_4x4()) == ch);
    
    }

    return 0;
}

/**
 * @brief Init timer set all registers for start send setup timer
 * 
 */
void init_timer() {
    CCTL0 = CCIE;            // enable interupt from timer
    CCR0  = 0x200;           // set how many tiscks for interupt (1/64s)
                             // 512 ticks for interupt subrutine
    TACTL = TASSEL_1 + MC_2; // ACLK (f = 32768 Hz) continuous mode
}

/**
 * @brief Iterupt from timer (once per 1/64s)
 * 
 */
interrupt (TIMERA0_VECTOR) Timer_A (void) {
    flip_led_d5();                      //timer heart beep

    if (timer_sec[0] >= TIMER_MAX) {    //Stop counting when time cant be displayd
        counting = false;
    }


    if (!counting) {
        CCR0      += 0x200;             // set how many tiscks for next interupt (1/64s)
        return;
    }

    timer_sec[0] += 1;                  // add 1/64

    render_time(timer_sec[0], prefix);  // render time on disply

    if (prefix_timer) {
        prefix_timer--;

        if (!prefix_timer) {
            prefix = '\0';
        }
    }

    CCR0      += 0x200;                 // set how many tiscks for next interupt (1/64s)
}

/**
 * @brief Decode command from pc (Unused)
 * 
 * @param cmd_ucase 
 * @param cmd 
 * @return unsigned char 
 */
unsigned char decode_user_cmd(char *cmd_ucase, char *cmd) {
    return CMD_UNKNOWN;
}

/**
 * @brief Initialize perifers after pragramed FPGA
 * 
 */
void fpga_initialized() {
    LCD_init();
    LCD_clear();
    stopwatch_restart();
    init_timer();
}


/**
 * @brief Main function (with main loop)
 * 
 * @return int 
 */
int main(void) {    
    initialize_hardware();
    keyboard_init();
    
    set_led_d5(1);                         // start D5 timer heart beep
    set_led_d6(1);                         // start D6 power led
    
    while (true) {
        keyboard_idle();                   // keyborad handler
        terminal_idle();                   // terminal handler
    }         
}
