/* 
 * File:   clock.h
 * Author: wino
 *
 * Created on August 12, 2017, 3:32 PM
 */

#ifndef CLOCK_H
#define	CLOCK_H

#ifdef	__cplusplus
extern "C" {
#endif

void DigitsWriteByte(unsigned char Data, int ShowDots);
void DigitsStart();
void DigitsStop();
void DigitsShow(unsigned char Data[], int ShowDots);
void DigitsClear();

#ifdef	__cplusplus
}
#endif

#endif	/* CLOCK_H */

