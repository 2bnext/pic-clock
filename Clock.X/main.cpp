/* 
 * File:   main.cpp
 * Author: wino
 *
 * Created on August 12, 2017, 3:21 PM
 */

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

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#define FCY 3276800 
#define LED1 _LATB6
#define DIGITS_DIN _RB7
#define DIGITS_DOUT _LATB7
#define DIGITS_CLK _LATB8

#define ADDR_AUTO 0x40
#define ADDR_FIXED 0x44
#define BRIGHTNESS 0x88
#define STARTADDR 0xC0
#define BRIGHT_DARKEST 0
#define BRIGHT_TYPICAL 2
#define BRIGHT_HIGHEST 7

#define INTERVAL    100L
#define FLASH       (INTERVAL / 2L)
#define ONESECOND   INTERVAL
#define ONEMINUTE   (60L * ONESECOND)
#define ONEHOUR     (60L * ONEMINUTE)
#define ONEDAY      (24L * ONEHOUR)

#include <cstdlib>

using namespace std;

// -----------------------------------------------------------------------------

unsigned char HexDigits[] = {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x77,0x7c,0x39,0x5e,0x79,0x71};

void DigitsWriteByte(unsigned char Data, int ShowDots)
{
    int i;
    
    if (ShowDots)
        Data |= 0x80;
    
    for (i = 0; i < 8; i++)
    {
        DIGITS_CLK = 0;
        
        if (Data & 1)
            DIGITS_DOUT = 1;
        else
            DIGITS_DOUT = 0;
        
        DIGITS_CLK = 1;
        
        Data = Data >> 1;
    }
    
    // wait for ACK
    
    DIGITS_CLK = 0;
    DIGITS_DOUT = 1;
    DIGITS_CLK = 1;
    
    _TRISB7 = 1; // Set DIO as Input
    
    while (DIGITS_DIN)
    {
        // __delay_ms(1);
        
        if (DIGITS_DIN)
        {
            _TRISB7 = 0;
            DIGITS_DOUT = 0;
            _TRISB7 = 1;
        }
    }
    
    _TRISB7 = 0;
}

void DigitsStart()
{
    DIGITS_CLK = 1;
    DIGITS_DOUT = 1;
    DIGITS_DOUT = 0;
    DIGITS_CLK = 0;
}

void DigitsStop()
{
    DIGITS_CLK = 0;
    DIGITS_DOUT = 0;
    DIGITS_CLK = 1;
    DIGITS_DOUT = 1;
}

void DigitsShow(unsigned char Data[], int ShowDots)
{
    int i;
    
    DigitsStart();
    DigitsWriteByte(ADDR_AUTO, 0);
    DigitsStop();

    DigitsStart();
    DigitsWriteByte(STARTADDR, 0);
        
    for (i = 0; i < 4; i++)
    {
        unsigned char d = Data[i];
        
        if (d == 0xff)
            d = 0;
        else
            d = HexDigits[Data[i]];
        
        DigitsWriteByte(d, ShowDots);
    }
    
    DigitsStop();
    
    DigitsStart();
    DigitsWriteByte(BRIGHTNESS | BRIGHT_TYPICAL, 0);
    DigitsStop();
}

void DigitsClear()
{
    int i;
    
    DigitsStart();
    DigitsWriteByte(ADDR_AUTO, 0);
    DigitsStop();

    DigitsStart();
    DigitsWriteByte(STARTADDR, 0);
        
    for (i = 0; i < 4; i++)
        DigitsWriteByte(0, 0);
    
    DigitsStop();
    
    DigitsStart();
    DigitsWriteByte(BRIGHTNESS | BRIGHT_TYPICAL, 0);
    DigitsStop();
}

// -----------------------------------------------------------------------------

unsigned long Timer = ONEHOUR;

int Flash = 0;
int Toggle = 0;
int SetMode = 0;

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
    
    _T1MD = 0;
    
    PR1 = 1024;     // 

    _T1IF = 0;
    _T1IE = 1;
    _TCKPS = 0b01;  // prescaler to 1:1
    _TON = 1;
    
    // Set button
    
    _TRISB0 = 1;    // set as input
    _CN2PUE = 1;    // pull up
    _CN2IE = 1;     // Enable interrupt
    
    // Plus button
    
    _TRISB1 = 1;    // set as input
    _CN3PUE = 1;    // pull up
    _CN3IE = 1;     // Enable interrupt
    
    // Min button
    
    _TRISB2 = 1;    // set as input
    _CN4PUE = 1;    // pull up
    _CN4IE = 1;     // Enable interrupt
    
    _CNIE = 1;    
}

void __attribute__((interrupt, auto_psv)) _T1Interrupt(void) 
{
    unsigned char data[4];
    
    _T1IF = 0;
 
    Timer++;
    
    if (Timer == ONEDAY)
        Timer = 0;

    if (Timer % FLASH == 0)
    {
        unsigned long minutes = (Timer / ONEMINUTE) % 60;
        unsigned long hours = Timer / ONEHOUR;
        
        Flash ^= 1;
        
        data[3] = minutes % 10;
        data[2] = minutes / 10 % 10;
        data[1] = hours % 10;
        data[0] = hours / 10 % 10;

        if (SetMode && Flash)
        {
            if (SetMode == 1)   // hour
            {
                data[1] = 0xff;
                data[0] = 0xff;
            }
            else
            if (SetMode == 2)
            {
                data[3] = 0xff;
                data[2] = 0xff;                
            }
        }            
            
        DigitsShow(data, !Flash);
    }
}

void __attribute__((interrupt, auto_psv)) _CNInterrupt(void) 
{
    _CNIF = 0;
    
    if (_RB0 == 0)  // set
    {
        if (SetMode == 0)
            SetMode = 1;
        else
        if (SetMode == 1)
            SetMode = 2;
        else
            SetMode = 0;        
        
        LED1 = SetMode != 0;
    }
    
    if (SetMode)
    {
        unsigned long timer = Timer;
        
        if (_RB1 == 0)  // plus
        {
            if (SetMode == 1)
                timer += ONEHOUR;
            else
            if (SetMode == 2)
                timer += ONEMINUTE;
        }

        if (_RB2 == 0)  // min
        {
            if (SetMode == 1)
                timer -= ONEHOUR;
            else
            if (SetMode == 2)
                timer -= ONEMINUTE;
        }
        
        if (timer < ONEDAY) // if valid, set
            Timer = timer;
    }
}

int main(void) 
{    
    Initialize();
    
    for (;;)
        Idle();
    
    return 0;
}