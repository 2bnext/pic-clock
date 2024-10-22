/*
 * File:   clock.c
 * Author: wino
 *
 * Created on March 5, 2017, 10:13 AM
 */

// -----------------------------------------------------------------------------
// DSPIC30F3013 Configuration Bit Settings

// FOSC
#pragma config FOSFPR = XTL             // Oscillator
#pragma config FCKSMEN = CSW_FSCM_OFF   // Clock Switching and Monitor (Sw Disabled, Mon Disabled)

// FWDT
#pragma config FWPSB = WDTPSB_16        // WDT Prescaler B (1:16)
#pragma config FWPSA = WDTPSA_1         // WDT Prescaler A (1:1)
#pragma config WDT = WDT_OFF            // Watchdog Timer (Disabled)

// FBORPOR
#pragma config FPWRT = PWRT_64          // POR Timer Value (64ms)
#pragma config BODENV = BORV20          // Brown Out Voltage (Reserved)
#pragma config BOREN = PBOR_ON          // PBOR Enable (Enabled)
#pragma config MCLRE = MCLR_EN          // Master Clear Enable (Enabled)

// FGS
#pragma config GWRP = GWRP_OFF          // General Code Segment Write Protect (Disabled)
#pragma config GCP = CODE_PROT_OFF      // General Segment Code Protection (Disabled)

// FICD
#pragma config ICS = ICS_PGD            // Comm Channel Select (Use PGC/EMUC and PGD/EMUD)
// -----------------------------------------------------------------------------
#define FCY 3276800 

#include <libpic30.h>
#include <xc.h>

#include "digits.h"
// -----------------------------------------------------------------------------
#define LED1        _LATB6
#define INTERVAL    800L
#define FLASH       (INTERVAL / 2L)
#define ONESECOND   INTERVAL
#define ONEMINUTE   (60L * ONESECOND)
#define ONEHOUR     (60L * ONEMINUTE)
#define ONEDAY      (24L * ONEHOUR)
// -----------------------------------------------------------------------------
typedef unsigned char bool;
typedef enum {ShowTime, ShowCountdown, ShowCountup, SetHours, SetMinutes} Modes;
enum {false = 0, true = 1};
enum {low = 0, high = 1};
// -----------------------------------------------------------------------------
unsigned long Time = ONEHOUR;
unsigned long Countdown = 0;
unsigned long Countup = 0;
bool Flash = false;
bool Toggle = false;
bool Buzz = false;
bool PauseCountup = false;
Modes Mode = ShowTime;
// -----------------------------------------------------------------------------
void Initialize() 
{
    PMD1 = 0xffff;
    PMD2 = 0xffff;

    TRISB = 0;
    TRISC = 0;
    TRISD = 0;
    TRISF = 0;
    
    LATB = 0;
    LATC = 0;
    LATD = 0;
    LATF = 0;
    
    _T1MD = low;
    
    PR1 = 1023;        // 0-1023 = 1024 steps

    _T1IF = low;
    _T1IE = high;
    _TCKPS = 0b00;     // prescaler to 1:16
    _TON = high;
    
    // Set button
    
    _TRISB0 = high;    // set as input
    _CN2PUE = high;    // pull up
    _CN2IE = high;     // Enable interrupt
    
    // Plus button
    
    _TRISB1 = high;    // set as input
    _CN3PUE = high;    // pull up
    _CN3IE = high;     // Enable interrupt
    
    // Min button
    
    _TRISB2 = high;    // set as input
    _CN4PUE = high;    // pull up
    _CN4IE = high;     // Enable interrupt
    
    _CNIE = high;
}
// -----------------------------------------------------------------------------
void __attribute__((interrupt, auto_psv)) _CNInterrupt(void) 
{
    _CNIF = 0;
    
    if (_RB0 == low)  // reset
    {
        if (Mode == ShowTime)
            Mode = SetHours;
        else
        if (Mode == SetHours)
            Mode = SetMinutes;
        else
        if (Mode == ShowCountdown || Mode == ShowCountup)
        {
            Countdown = 0;
            Countup = 0;
            Mode = ShowTime;
            LED1 = low;
        }
        else
            Mode = ShowTime;
    }
    else
    {
        switch (Mode)
        {
        case ShowTime:
            if (_RB1 == low)
            {
                Countdown = 30 * ONESECOND;
                Mode = ShowCountdown;
            }
            else
            if (_RB2 == low)
            {
                Countup = 0;
                Mode = ShowCountup;
            }
            break;

        case ShowCountdown:
            if (_RB1 == low)  // plus
                Countdown += 30 * ONESECOND;
            else
            if (_RB2 == low)
                Countdown -= 30 * ONESECOND;

            if (Countdown > 12 * ONEHOUR)
                Countdown = 0;

            break;

        case ShowCountup:
            if (_RB1 == low)  // plus
            {
                Countup = 0;
                PauseCountup = true;
            }
            else
            if (_RB2 == low)
                PauseCountup ^= true;

            break;

        case SetHours:
            {
                unsigned long timer = Time;

                if (_RB1 == low)  // plus
                    timer += ONEHOUR;
                else
                if (_RB2 == low)  // min
                    timer -= ONEHOUR;

                if (timer < ONEDAY) // if valid, set
                    Time = timer;
            }
            break;

        case SetMinutes:
            {
                unsigned long timer = Time;

                if (_RB1 == low)  // plus
                    timer += ONEMINUTE;
                else
                if (_RB2 == low)  // min
                    timer -= ONEMINUTE;

                if (timer < ONEDAY) // if valid, set
                    Time = timer;
            }
            break;
        }
    }
}
// -----------------------------------------------------------------------------
void __attribute__((interrupt, auto_psv)) _T1Interrupt(void) 
{    
    _T1IF = 0;
 
    Time++;
    
    if (Time == ONEDAY)
        Time = 0;
    
    if (Mode == ShowCountdown)
    {    
        if (Countdown)
            Countdown--;
        else
        {
            Buzz ^= high;    
            LED1 = Buzz;
        }
    }
    else
    if (Mode == ShowCountup)
    {
        if (!PauseCountup)
            Countup++;
    }
}
// -----------------------------------------------------------------------------
int main(void) 
{    
    Initialize();
    
    for (;;)
    {
        Idle();
 
        if (Time % FLASH == 0)
        {        
            unsigned char data[4];
            unsigned long value1, value2;

            Flash ^= true;
            // LED1 ^= high;

            if (Mode == ShowCountdown)
            {
                value2 = Countdown / ONESECOND % 60;
                value1 = Countdown / ONEMINUTE % 60;
            }
            else
            if (Mode == ShowCountup)
            {
                value2 = Countup / ONESECOND % 60;
                value1 = Countup / ONEMINUTE % 60;
            }
            else
            {        
                value2 = Time / ONEMINUTE % 60;
                value1 = Time / ONEHOUR;
            }

            data[3] = value2 % 10;
            data[2] = value2 / 10 % 10;
            data[1] = value1 % 10;
            data[0] = value1 / 10 % 10;

            if (Mode && Flash)
            {
                if (Mode == SetHours)   // hour
                {
                    data[1] = 0xff;
                    data[0] = 0xff;
                }
                else
                if (Mode == SetMinutes)
                {
                    data[3] = 0xff;
                    data[2] = 0xff;                
                }
                else
                if (Mode == ShowCountdown && Countdown == 0)
                {
                    data[0] = 0xff;
                    data[1] = 0xff;
                    data[2] = 0xff;                                
                    data[3] = 0xff;
                }
            }
            
            if (data[0] == 0)
                data[0] = 0xff;

            DigitsShow(data, !Flash);
        }
    }
    
    return 0;
}