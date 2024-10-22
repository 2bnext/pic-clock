#define FCY 3276800 

#include <libpic30.h>
#include <xc.h>

#define DIGITS_DIN _RB7
#define DIGITS_DOUT _LATB7
#define DIGITS_CLK _LATB8

#define ADDR_AUTO 0x40
#define ADDR_FIXED 0x44
#define BRIGHTNESS 0x88
#define SET_ADDRESS 0xC0
#define BRIGHT_DARKEST 0
#define BRIGHT_TYPICAL 2
#define BRIGHT_HIGHEST 7
// -----------------------------------------------------------------------------
static unsigned char HexDigits[] = 
    {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 
        0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71};
// -----------------------------------------------------------------------------
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
        __delay_ms(1);
        
        if (DIGITS_DIN)
        {
            _TRISB7 = 0;
            DIGITS_DOUT = 0;
            _TRISB7 = 1;
        }
    }
    
    _TRISB7 = 0;
}
// -----------------------------------------------------------------------------
void DigitsStart()
{
    DIGITS_CLK = 1;
    DIGITS_DOUT = 1;
    DIGITS_DOUT = 0;
    DIGITS_CLK = 0;
}
// -----------------------------------------------------------------------------
void DigitsStop()
{
    DIGITS_CLK = 0;
    DIGITS_DOUT = 0;
    DIGITS_CLK = 1;
    DIGITS_DOUT = 1;
}
// -----------------------------------------------------------------------------
void DigitsShow(unsigned char Data[], int ShowDots)
{
    int i;
    
    DigitsStart();
    DigitsWriteByte(ADDR_AUTO, 0);
    DigitsStop();

    DigitsStart();
    DigitsWriteByte(SET_ADDRESS, 0);
        
    for (i = 0; i < 4; i++)
    {
        unsigned char d = Data[i];
        
        if (d == 0xff)
            d = 0;
        else
            d = HexDigits[Data[i]];
        
        DigitsWriteByte(d, i == 1 ? ShowDots : 0);
    }
    
    DigitsStop();
    
    DigitsStart();
    DigitsWriteByte(BRIGHTNESS | BRIGHT_TYPICAL, 0);
    DigitsStop();
}
// -----------------------------------------------------------------------------
void DigitsClear()
{
    int i;
    
    DigitsStart();
    DigitsWriteByte(ADDR_AUTO, 0);
    DigitsStop();

    DigitsStart();
    DigitsWriteByte(SET_ADDRESS, 0);
        
    for (i = 0; i < 4; i++)
        DigitsWriteByte(0, 0);
    
    DigitsStop();
    
    DigitsStart();
    DigitsWriteByte(BRIGHTNESS | BRIGHT_TYPICAL, 0);
    DigitsStop();
}
