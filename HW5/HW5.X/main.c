#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro
#include<math.h>

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

void initPIC();
void initSPI();
unsigned char spi_io(unsigned char o);
unsigned short to16bit(unsigned char ch, unsigned char v);


int main() {

    __builtin_disable_interrupts();
    
    initPIC();
    initSPI();
    
    __builtin_enable_interrupts();
    
    float dt = 0;
    float pi = 3.14; // I miss Python
    int isPeak = 0;
    int v_tr = 0;
    unsigned char v;
    unsigned short out1, out2, in1, in2, in3, in4;

    while (1) {
        v = (1 + sin(4 * pi * dt)) * 128; // generating sin wave
        dt = dt + .01; 
        
        // for someone unknown to me reason, if i try to simplify this into
        // if - else if, the triangular wave gets a line in the middle of the
        // peak - idk what's going on there. I don't think there should be
        // any reason this has to stay as two separate if statements, but
        // it is what it is...
        
        // UPD: if-else looks a lot nicer, i'll keep it
        
        // generating triangular wave
        if (!isPeak) {
            v_tr = v_tr + 5.1;
            if (v_tr > 255) {
                isPeak = 1; // reached peak
            }
        } else {
            v_tr = v_tr - 5.1; // alternative behavior here
            if (v_tr < 0) {
                isPeak = 0;
                v_tr = 0;
            }
        }
        
        // output
        out1 = to16bit(0, v);
        LATBbits.LATB6 = 0; 
        in1 = out1 >> 8;
        in2 = out1;
        
        // Do IO thing
        spi_io(in1);
        spi_io(in2);
        LATBbits.LATB6 = 1;
        
        out2 = to16bit(1, v_tr); 
        LATBbits.LATB6 = 0;
        in3 = out2 >> 8;
        in4 = out2;
        
        // Do IO thing
        spi_io(in3);
        spi_io(in4);
        LATBbits.LATB6 = 1;
        
        // A little wait here
        _CP0_SET_COUNT(0);
        while (_CP0_GET_COUNT() < 240000) {} // do 100 Hz      
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
}

//initialize SPI1
void initSPI() {
    // Pin B14 has to be SCK1
    // Turn of analog pins
    ANSELA = 0; // 1 for analog
    // Make an output pin for CS
    TRISBbits.TRISB6 = 0; // CS pin
    LATBbits.LATB6 = 1; // CS pin
    // Set SDO1
    RPA1Rbits.RPA1R = 0b0011;
    // Set SDI1
    SDI1Rbits.SDI1R = 0b0001;

    // setup SPI1
    SPI1CON = 0; // turn off the spi module and reset it
    SPI1BUF; // clear the rx buffer by reading from it
    SPI1BRG = 1000; // 1000 for 24kHz, 1 for 12MHz; // baud rate to 10 MHz [SPI1BRG = (48000000/(2*desired))-1]
    SPI1STATbits.SPIROV = 0; // clear the overflow bit
    SPI1CONbits.CKE = 1; // data changes when clock goes from hi to lo (since CKP is 0)
    SPI1CONbits.MSTEN = 1; // master operation
    SPI1CONbits.ON = 1; // turn on spi 
}

// send a byte via SPI and return response
unsigned char spi_io(unsigned char o) {
    SPI1BUF = o;
    while (!SPI1STATbits.SPIRBF) {
        ; // wait to receive byte
    }
    return SPI1BUF;
}

unsigned short to16bit(unsigned char ch, unsigned char v) {
    unsigned short s = 0; 
    s = ch << 15;
    s = s | (0b111 << 12);
    s = s | (v << 4);
    return s;
}