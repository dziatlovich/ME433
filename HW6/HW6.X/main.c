#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro
#include <stdio.h>
#include "i2c_master_noint.h"

// DEVCFG0
#pragma config DEBUG = OFF // disable debugging
#pragma config JTAGEN = OFF // disable jtag
#pragma config ICESEL = ICS_PGx1 // use PGED1 and PGEC1
#pragma config PWP = OFF // disable flash write protect
#pragma config BWP = OFF // disable boot write protect
#pragma config CP = OFF // disable code protect

// DEVCFG1
#pragma config FNOSC = FRCPLL // use fast frc oscillator with pll
#pragma config FSOSCEN = OFF // disable secondary oscillator
#pragma config IESO = OFF // disable switching clocks
#pragma config POSCMOD = OFF // primary osc disabled
#pragma config OSCIOFNC = OFF // disable clock output
#pragma config FPBDIV = DIV_1 // divide sysclk freq by 1 for peripheral bus clock
#pragma config FCKSM = CSDCMD // disable clock switch and FSCM
#pragma config WDTPS = PS1048576 // use largest wdt value, ??
#pragma config WINDIS = OFF // use non-window mode wdt
#pragma config FWDTEN = OFF // wdt disabled
#pragma config FWDTWINSZ = WINSZ_25 // wdt window at 25%

// DEVCFG2 - get the sysclk clock to 48MHz from the 8MHz fast rc internal oscillator
#pragma config FPLLIDIV = DIV_2 // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = MUL_24 // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2 // divide clock after FPLLMUL to get 48MHz

// DEVCFG3
#pragma config USERID = 00000000 // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = OFF // allow multiple reconfigurations
#pragma config IOL1WAY = OFF // allow multiple reconfigurations

#define IODIR 0x00
#define OLAT 0x0A
#define GPIO 0x09

void initPIC();
void WriteI2C(unsigned char ad, unsigned char reg, unsigned char out);
unsigned char ReadI2C(unsigned char ad, unsigned char reg);
void blink();

int main() {

    __builtin_disable_interrupts(); // disable interrupts while initializing things

    initPIC();
    i2c_master_setup(); // setup
 
    WriteI2C(0b0100000, IODIR, 0b01111111); 
    WriteI2C(0b0100000, OLAT, 0b10000000); 

    __builtin_enable_interrupts();
    
    unsigned char a;
    
    while (1) {
        blink(); // do heartbeat thingamajig
        a = ReadI2C(0b0100000, GPIO); 
        if (a & 0b1 == 0b1) {
            WriteI2C(0b0100000, OLAT, 0b00000000); 
        } else {
            WriteI2C(0b0100000, OLAT, 0b10000000); 
        }; 
    }
}

void initPIC() {
    // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);

    // 0 data RAM access wait states
    BMXCONbits.BMXWSDRM = 0x0;

    // enable multi vector interrupts
    INTCONbits.MVEC = 0x1;

    // disable JTAG to get pins back
    DDPCONbits.JTAGEN = 0;

    // do your TRIS and LAT commands here
    TRISAbits.TRISA4 = 0; 
    LATAbits.LATA4 = 0; 
    
    TRISBbits.TRISB4 = 1;
    LATBbits.LATB4 = 0;
}

void WriteI2C(unsigned char ad, unsigned char reg, unsigned char out) {
    i2c_master_start(); 
    i2c_master_send(ad<<1); 
    i2c_master_send(reg); 
    i2c_master_send(out); 
    i2c_master_stop(); 
}

unsigned char ReadI2C(unsigned char ad, unsigned char reg) {
    i2c_master_start(); 
    i2c_master_send(ad << 1);
    i2c_master_send(reg); 
    i2c_master_restart(); 
    i2c_master_send(ad << 1 | 0b1); 
    
    unsigned char a = i2c_master_recv();
    i2c_master_ack(1); 
    i2c_master_stop(); 
    
    return a;
}

void blink() {
    LATAbits.LATA4 = 1; 
    _CP0_SET_COUNT(0); 
    while (_CP0_GET_COUNT() < 12000000 ) {} 
    
    LATAbits.LATA4 = 0; 
    _CP0_SET_COUNT(0); 
    while (_CP0_GET_COUNT() < 12000000 ) {} 
}
