/*
 * SSD1306xLED - Drivers for SSD1306 controlled dot matrix OLED/PLED 128x32 displays
 *
 * @created: 2014-08-12
 * @author: Neven Boyanov
 * @date: 2020-04-14
 * @author: Simon Merrett
 *
 * @author Martin Marcinčák
 * @date: 2020-03-08
 * @version: 0.2-light
 *
 *
 * Source code for original version available at: https://bitbucket.org/tinusaur/ssd1306xled
 * Source code for ATtiny 0 and 1 series with megaTinyCore: https://github.com/SimonMerrett/tinyOLED
 * Source code for ATtiny 0 and 1 series without megaTinyCore: https://github.com/Frogieder/tinierOLED
 */

// ----------------------------------------------------------------------------

//#include <stdlib.h> //**
//#include <avr/io.h> //**

//#include <avr/pgmspace.h> //**

#include "tinierOLED.h"
#include "font6x8.h"

#ifndef _nofont_8x16	// tBUG Optional removal to save code space
#include "font8x16.h"
#endif


// ----------------------------------------------------------------------------

// Some code based on "IIC_wtihout_ACK" by http://www.14blog.com/archives/1358

const uint8_t ssd1306_init_sequence [] /*PROGMEM*/ = {	// Initialization Sequence //**
    0xAE,			// Display OFF (sleep mode)
    0x20, 0b00,		// Set Memory Addressing Mode
    // 00=Horizontal Addressing Mode; 01=Vertical Addressing Mode;
    // 10=Page Addressing Mode (RESET); 11=Invalid
    0xB7,			// Set Page Start Address for Page Addressing Mode, 0-7 //** was B7
    0xC8,			// Set COM Output Scan Direction
    0x00,			// ---set low column address
    0x10,			// ---set high column address
    0x40,			// --set start line address
    0x81, 0x3F,		// Set contrast control register //** maybe 0x8F
    0xA1,			// Set Segment Re-map. A0=address mapped; A1=address 127 mapped.
    0xA6,			// Set display mode. A6=Normal; A7=Inverse
    0xA8, 0x1F,		// Set multiplex ratio(1 to 64) //** was 0x3F
    0xA4,			// Output RAM to Display
    // 0xA4=Output follows RAM content; 0xA5,Output ignores RAM content
    0xD3, 0x00,		// Set display offset. 00 = no offset
    0xD5,			// --set display clock divide ratio/oscillator frequency
    0xF0,			// --set divide ratio
    0xD9, 0x22,		// Set pre-charge period
    0xDA, 0x02,		// Set com pins hardware configuration		//** was 0xDA, 0x12
    0xDB,			// --set vcomh
    0x20,			// 0x20,0.77xVcc
    0x8D, 0x14,		// Set DC-DC enable
    0xAF			// Display ON in normal mode

};

// Program:    5248 bytes // not anymore, probably~

SSD1306Device::SSD1306Device(void){
    oledFont, oledX, oledY = 0;
}


void SSD1306Device::begin(void)
{
    TinierI2C.init();
    for (uint8_t i = 0; i < sizeof (ssd1306_init_sequence); i++)
    {
        ssd1306_send_command(ssd1306_init_sequence[i]);
    }
    clear();
}


void SSD1306Device::setFont(uint8_t font)
{
    oledFont = font;
}

void SSD1306Device::ssd1306_send_command_start(void) {
    TinierI2C.start(SSD1306, 0);
    TinierI2C.write(0x00); // write command
}

void SSD1306Device::ssd1306_send_command_stop(void) {
    TinierI2C.stop();
}

void SSD1306Device::ssd1306_send_data_byte(uint8_t byte)
{
    //if(Wire.writeAvailable()){ //**
    //	ssd1306_send_data_stop();
    //	ssd1306_send_data_start();
    //} //**
    TinierI2C.write(byte);
}

void SSD1306Device::ssd1306_send_command(uint8_t command)
{
    ssd1306_send_command_start();
    TinierI2C.write(command);
    ssd1306_send_command_stop();
}

void SSD1306Device::ssd1306_send_data_start(void)
{
    TinierI2C.start(SSD1306, 0);
    TinierI2C.write(0x40); // write data
}

void SSD1306Device::ssd1306_send_data_stop(void)
{
    TinierI2C.stop();
}

void SSD1306Device::setCursor(uint8_t x, uint8_t y)
{
    ssd1306_send_command_start();
    TinierI2C.write(0xb0 + y);
    TinierI2C.write(((x & 0xf0) >> 4) | 0x10); // | 0x10
    TinierI2C.write((x & 0x0f) | 0x01); // | 0x01
    ssd1306_send_command_stop();
    oledX = x;
    oledY = y;
}

void SSD1306Device::clear(void)
{
    fill(0x00);
}

void SSD1306Device::fill(uint8_t fill)
{
    uint8_t m,n;
    for (m = 0; m < 4; m++)
    {
        ssd1306_send_command(0xb0 + m);	// page0 - page1
        ssd1306_send_command(0x00);		// low column start address
        ssd1306_send_command(0x10);		// high column start address
        ssd1306_send_data_start();
        for (n = 0; n < 128; n++)
        {
            ssd1306_send_data_byte(fill);
        }
        ssd1306_send_data_stop();
    }
    setCursor(0, 0);
}

int SSD1306Device::write(char c) {
    uint8_t i = 0;
    uint8_t ci = c - 32;
    if(c == '\r')
        return 1;
    if(c == '\n'){
        if(oledFont == FONT6X8) {	//	tBUG
            oledY++;
//			if ( oledY > 7) // tBUG
//				oledY = 7;
        }
        else {
            oledY+=2;	//tBUG  Large Font up by two
            if ( oledY > 6) // tBUG
                oledY = 6;
        }
        setCursor(0, oledY);
        return 1;
    }

    if(oledFont == FONT6X8){
        if (oledX > 122)
        {
            oledX = 0;
            oledY++;
            if ( oledY > 7) // tBUG
                oledY = 7;
            setCursor(oledX, oledY);
        }

        ssd1306_send_data_start();
        for (i= 0; i < 6; i++)
        {
            ssd1306_send_data_byte(ssd1306xled_font6x8[ci * 6 + i]);
        }
        ssd1306_send_data_stop();
        setCursor(oledX+6, oledY);
    }
#ifndef _nofont_8x16	// tBUG
    else{
		if (oledX > 120)
		{
			oledX = 0;
			oledY+=2;	//tBUG  Large Font up by two
//			oledY++;
			if ( oledY > 6) // tBUG
				oledY = 6;
			setCursor(oledX, oledY);
		}

		ssd1306_send_data_start();
		for (i = 0; i < 8; i++)
		{
			TinierI2C.write(ssd1306xled_font8x16[ci * 16 + i]);
		}
		ssd1306_send_data_stop();
		setCursor(oledX, oledY + 1);
		ssd1306_send_data_start();
		for (i = 0; i < 8; i++)
		{
			TinierI2C.write(ssd1306xled_font8x16[ci * 16 + i + 8]);
		}
		ssd1306_send_data_stop();
		setCursor(oledX + 8, oledY - 1);
	}
#endif // _nofont_8x16
    return 1;
}

void SSD1306Device::drawPixel(uint8_t x0, uint8_t y0)
{
    uint8_t m,n,r;
    m = x0 / 8;
    r = 0x01 << (x0 % 8);
    ssd1306_send_command(0xb0 + m);	// page0 - page1
    ssd1306_send_command(0x00 | (y0&15) );		// low column start address
    ssd1306_send_command(0x10 | (y0>>4));		// high column start address
    ssd1306_send_data_start();
    ssd1306_send_data_byte(r);
    ssd1306_send_data_stop();
    setCursor(0, 0);
}

void SSD1306Device::print(const char *a) {
    unsigned int i = 0;
    while (a[i] != '\0') {
        write(a[i++]);
    }
}

void SSD1306Device::println(const char *a) {
    print(a);
    write('\n');
}

void SSD1306Device::sleep(void)
{
    // placeholder for possible future function.
    // Sounds like reset pin on SSD1306 is necessary, so best method may be to supply/control OLED VCC from GPIO?
}

void SSD1306Device::print_num(uint16_t num)
{
    uint8_t startX = oledX;
    uint8_t startY = oledY;
    auto remaining_digits = n_digits(num);
    do {
        setCursor((--remaining_digits)*6 + startX, startY);
        char c = num % 10 + '0';
        write(c);
        num /= 10;
    } while(num > 0);
}

void SSD1306Device::print_num(uint32_t num)
{
    uint8_t startX = oledX;
    uint8_t startY = oledY;
    auto remaining_digits = n_digits(num);
    do {
        setCursor((--remaining_digits)*6 + startX, startY);
        char c = num % 10 + '0';
        write(c);
        num /= 10;
    } while(num > 0);
}

uint8_t n_digits(uint16_t num)
{
    if (num >= 10000) { //5+ digits
        /*
        if (num < 65535) {
            return 5;
        }
        if (num >= 1000000) { // 7+ digits
            if (num >= 100000000) { // 9+ digits
                if (num >= 1000000000)
                    return 10;
                else
                    return 9;
            } else { // 7 or 8 digits
                if (num >= 10000000)
                    return 8;
                else
                    return 7;
            }
        }
        if (num >= 100000)
            return 6;
        else
            return 5;
        */
        return 5;
    }
    else if (num >= 100) {
        if (num >= 1000)
            return 4;
        else
            return 3;
    } else {
        if (num >= 10)
            return 2;
        else
            return 1;
    }
}

uint8_t n_digits(uint32_t num)
{
    if (num >= 10000) { //5+ digits
        if (num < 65535) {
            return 5;
        }
        if (num >= 1000000) { // 7+ digits
            if (num >= 100000000) { // 9+ digits
                if (num >= 1000000000)
                    return 10;
                else
                    return 9;
            } else { // 7 or 8 digits
                if (num >= 10000000)
                    return 8;
                else
                    return 7;
            }
        }
        if (num >= 100000)
            return 6;
        else
            return 5;
    }
    else if (num >= 100) {
        if (num >= 1000)
            return 4;
        else
            return 3;
    } else {
        if (num >= 10)
            return 2;
        else
            return 1;
    }
}

/*

void SSD1306Device::bitmap(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, const uint8_t bitmap[])
{
	uint16_t j = 0;
	uint8_t y, x;
	// if (y1 % 8 == 0) y = y1 / 8; 	// else y = y1 / 8 + 1;		// tBUG :: this does nothing as y is initialized below
	//	THIS PARAM rule on y makes any adjustment here WRONG   //usage oled.bitmap(START X IN PIXELS, START Y IN ROWS OF 8 PIXELS, END X IN PIXELS, END Y IN ROWS OF 8 PIXELS, IMAGE ARRAY);
 	for (y = y0; y < y1; y++)
	{
		setCursor(x0,y);
		ssd1306_send_data_start();
		for (x = x0; x < x1; x++)
		{
#ifdef TINYWIRE
			ssd1306_send_data_byte(pgm_read_byte(&bitmap[j++]));
		}
		ssd1306_send_data_stop();
	}
	setCursor(0, 0);
}
*/

SSD1306Device oled;

// ----------------------------------------------------------------------------
