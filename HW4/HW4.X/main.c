#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro
#include<stdio.h>
#include"i2c_master_noint.h"

// DEVCFG0
#pragma config DEBUG = OFF // no debugging
#pragma config JTAGEN = OFF // no jtag
#pragma config ICESEL = ICS_PGx1 // use PGED1 and PGEC1
#pragma config PWP = OFF // no write protect
#pragma config BWP = OFF // no boot write protect
#pragma config CP = OFF // no code protect

// DEVCFG1
#pragma config FNOSC = PRIPLL // use primary oscillator with pll
#pragma config FSOSCEN = OFF // turn off secondary oscillator
#pragma config IESO = OFF // no switching clocks
#pragma config POSCMOD = HS // high speed crystal mode
#pragma config OSCIOFNC = OFF // disable secondary osc
#pragma config FPBDIV = DIV_1 // divide sysclk freq by 1 for peripheral bus clock
#pragma config FCKSM = CSDCMD // do not enable clock switch
#pragma config WDTPS = PS1048576 // use slowest wdt
#pragma config WINDIS = OFF // wdt no window mode
#pragma config FWDTEN = OFF // wdt disabled
#pragma config FWDTWINSZ = WINSZ_25 // wdt window at 25%

// DEVCFG2 - get the sysclk clock to 48MHz from the 8MHz crystal
#pragma config FPLLIDIV = DIV_2 // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = MUL_24 // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2 // divide clock after FPLLMUL to get 48MHz

// DEVCFG3
#pragma config USERID = 0 // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = OFF // allow multiple reconfigurations
#pragma config IOL1WAY = OFF // allow multiple reconfigurations

void initExpander(void);
void setExpander(char pin, char level);
char getExpander(void);

int main() {

    __builtin_disable_interrupts();

    // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);

    // 0 data RAM access wait states
    BMXCONbits.BMXWSDRM = 0x0;

    // enable multi vector interrupts
    INTCONbits.MVEC = 0x1;

    // disable JTAG to get pins back
    DDPCONbits.JTAGEN = 0;
    
    // Turn off AN2 and AN3 pins (make B2 and B3 available for I2C)
    ANSELBbits.ANSB2 = 0;
    ANSELBbits.ANSB3 = 0;

    // do your TRIS and LAT commands here
    TRISBbits.TRISB4 = 1;
    TRISAbits.TRISA4 = 0;
    LATAbits.LATA4 = 1;
    
    // Initializing I2C2 with initExpander function created
    initExpander();
    
    __builtin_enable_interrupts();
    
    while(1) {
        setExpander(0,1);   // turn LED ON when the button is pushed
        
        char pushed = getExpander() >> 7;   // right shift by 7 to get logical '1' for unpushed or
                                            // logical '0' for pushed for the GP7 pin
        
        while(getExpander() >> 7) {     // LED is turned OFF when the button NOT pushed
            setExpander(0,0);   // turn LED OFF
            _CP0_SET_COUNT(0);      // Setting Core Timer count to 0
            LATAbits.LATA4 = !LATAbits.LATA4;       // Toggling the Green LED ON or OFF at 5 Hz
            while(_CP0_GET_COUNT() < 480000) { ; }     // 
        }
        _CP0_SET_COUNT(0);      // Setting Core Timer count to 0
        LATAbits.LATA4 = !LATAbits.LATA4;       // Toggling the Green LED ON or OFF at 5 Hz
        while(_CP0_GET_COUNT() < 4800000) { ; }
    }
}

void initExpander(void) {
    i2c_master_setup();     // function to set BAUD of I2C at 400 kHz
    i2c_master_start();     // make the start bit
    i2c_master_send(SLAVE_ADDR << 1 | 0);       // write the address, shifted left by 1, or'ed with a 0 to indicate writing
    i2c_master_send(0x00);      // set the direction of the pins with IODIR register addressed at 0x00
    i2c_master_send(0xF0);      // set GP0 - 3 as OUTPUT pins and GP4 - 7 as INPUT pins
    i2c_master_stop();      // make the stop bit
}

void setExpander(char pin, char level) {
    i2c_master_start();     // make the start bit
    i2c_master_send(SLAVE_ADDR << 1  | 0);      // write the address, shifted left by 1, or'ed with a 0 to indicate writing
    i2c_master_send(0x0A);      // write to OLAT register addressed at 0x0A
    i2c_master_send(level << pin);      // set 'value'  'pin' number of OLAT registers
    i2c_master_stop();      // make the stop bit
}
 
char getExpander(void) {
    i2c_master_start();     // make the start bit
    i2c_master_send(SLAVE_ADDR << 1 | 0);       // write the address, shifted left by 1, or'ed with a 0 to indicate writing
    i2c_master_send(0x09);      // read from GPIO register addressed at 0x09
    i2c_master_restart();       // make the restart bit
    i2c_master_send((SLAVE_ADDR << 1) | 1);     // write the address, shifted left by 1, or'ed with a 1 to indicate reading
    char r = i2c_master_recv();     // save the value returned
    i2c_master_ack(1);      // make the ack bit
    i2c_master_stop();      // make the stop bit
    return r;
}