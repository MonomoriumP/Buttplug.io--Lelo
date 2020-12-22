/*
 * Arduino library for emulating the Lelo vibrator remote.
 * Requires a CC2500 radio attached via SPI.
 *
 * Micah Elizabeth Scott <beth@scanlime.org>
 */

#include <SPI.h>
#include "LeloRemote.h"

LeloRemote::LeloRemote(int chipSelectPin)
    : csn(chipSelectPin) {}


void LeloRemote::spiTable(prog_uchar *table)
{
    while (1) {
        byte length = pgm_read_byte_near(table++);
        if (!length)
            return;

        digitalWrite(csn, LOW);
        while (length--)
            SPI.transfer(pgm_read_byte_near(table++));
        digitalWrite(csn, HIGH);
    }
}

byte LeloRemote::regRead(byte reg)
{
    digitalWrite(csn, LOW);
    SPI.transfer(0x80 | reg);
    byte result = SPI.transfer(0);
    digitalWrite(csn, HIGH);
    return result;
}   

byte LeloRemote::statusRead()
{
    // Dummy read from register 0
    digitalWrite(csn, LOW);
    byte result = SPI.transfer(0x80);
    digitalWrite(csn, HIGH);
    return result;
} 

void LeloRemote::reset()
{
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
    SPI.setClockDivider(SPI_CLOCK_DIV128);

    // Idle bus state
    pinMode(csn, OUTPUT);
    digitalWrite(csn, HIGH);
    
    // Poll for CHIP_RDYn
    while (statusRead() & 0x80);

    // Table-driven initialization
    static prog_uchar init[] PROGMEM = {
        1, 0x30,            // SRES        Soft reset strobe
        2, 0x0B, 0x0A,      // FSCTRL1     IF frequency of 253.9 kHz
        2, 0x0C, 0x00,      // FSCTRL0     No frequency offset (default)
        2, 0x0D, 0x5D,      // FREQ2       FREQ = 0x5d13b1 = 2420 MHz
        2, 0x0E, 0x13,      // FREQ1
        2, 0x0F, 0xB1,      // FREQ0
        2, 0x10, 0x2D,      // MDMCFG4     CHANBW = 541.666 kHz
        2, 0x11, 0x3B,      // MDMCFG3     DRATE = 249.94 kBaud
        2, 0x12, 0x73,      // MDMCFG2     MSK modulation, 30/32 sync word bits
        2, 0x13, 0x22,      // MDMCFG1     FEC disabled, 2 preamble bytes
        2, 0x14, 0xF8,      // MDMCFG0     CHANSPC = 199.951 kHz
        2, 0x0A, 0x01,      // CHANNR      Channel number 1
        2, 0x15, 0x00,      // DEVIATN     MSK phase change duty cycle
        2, 0x21, 0xB6,      // FREND1
        2, 0x22, 0x10,      // FREND0
        2, 0x18, 0x18,      // MCSM0       Auto calibrate, Poweron timeout
        2, 0x19, 0x1D,      // FOCCFG
        2, 0x1A, 0x1C,      // BSCFG
        2, 0x1B, 0xC7,      // AGCTRL2
        2, 0x1C, 0x00,      // AGCTRL1
        2, 0x1D, 0xB0,      // AGCTRL0
        2, 0x23, 0xEA,      // FSCAL3
        2, 0x24, 0x0A,      // FSCAL2
        2, 0x25, 0x00,      // FSCAL1
        2, 0x26, 0x11,      // FSCAL0
        2, 0x29, 0x59,      // FSTEST      Test only (default)
        2, 0x2C, 0x88,      // TEST2       Test settings (default)
        2, 0x2D, 0x31,      // TEST1       Test settings (default)
        2, 0x2E, 0x0B,      // TEST0       Test settings (default)
        2, 0x00, 0x29,      // IOCFG2      GDO2 used as CHIP_RDYn (default)
        2, 0x02, 0x06,      // IOCFG0      GDO0 used as sync
        2, 0x07, 0x0A,      // PKTCTRL1    Addr check, broadcast, CRC autoflush
        2, 0x08, 0x04,      // PKTCTRL0    No whitening, normal TX/RX mode, CRC on
        2, 0x09, 0x01,      // ADDR        Device address 0x01
        2, 0x06, 0x09,      // PKTLEN      Using 9-byte packet
        9, 0x7E,            // PATABLE     Set maximum output power
           0xFF, 0xFF, 0xFF, 0xFF,
           0xFF, 0xFF, 0xFF, 0xFF,
        1, 0x3A,            // SFRX        strobe: flush RX fifo
        1, 0x3B,            // SFTX        strobe: flush TX fifo
        1, 0x36,            // SIDLE
        0,
    };
    spiTable(init);
}

void LeloRemote::txPacket(const Packet &p)
{
    const byte *data = (const byte *) &p;
    byte length = sizeof p;

    // Prepare for transmit
    static prog_uchar prepare[] PROGMEM = {
        1, 0x3B,            // SFTX        Flush transmit FIFO
        1, 0x36,            // SIDLE
        0,
    };
    spiTable(prepare);

    // Write packet to TX FIFO
    digitalWrite(csn, LOW);
    SPI.transfer(0x7F);
    while (length--)
        SPI.transfer(*(data++));
    digitalWrite(csn, HIGH);

    // Trigger the transmit
    static prog_uchar trigger[] PROGMEM = {
        1, 0x35,            // STX        Enter transmit state
        0,
    };
    spiTable(trigger);
}

void LeloRemote::txMotorPower(byte power)
{
    Packet p;

    p.unk0 = 0x01;
    p.unk1 = 0x00;
    p.unk2 = 0xA5;
    p.motor[0] = power;
    p.motor[1] = power;
    p.unk5 = 0x00;
    p.unk6 = 0x00;
    p.unk7 = 0x00;
    p.unk8 = 0x05;

    txPacket(p);
}
