/*
 * ch34x_lib.h for ch341 in Epp/MEM/I2C/SPI/GPIO
 * Copyright (C) WCH 2019
 * Running Environment: Linux
 * Version:2.3
 */

#ifndef _CH34X_LIB_H
#define _CH34X_LIB_H

#ifndef CHAR
#define CHAR char
#endif

#ifndef UCHAR
#define UCHAR unsigned char
#endif

#ifndef USHORT
#define USHORT unsigned short
#endif

#ifndef ULONG
#define ULONG unsigned long
#endif

#ifndef LONGLONG
#define LONGLONG unsigned long long
#endif

#ifndef PUCHAR
#define PUCHAR unsigned char *
#endif

#ifndef PCHAR
#define PCHAR char *
#endif

#ifndef PUSHORT
#define PUSHORT unsigned short *
#endif

#ifndef PULONG
#define PULONG unsigned long *
#endif

#ifndef VOID
#define VOID void
#endif

#ifndef PVOID
#define PVOID void *
#endif

#define true 1
#define false 0

#define TRUE true
#define FALSE false

#ifndef min
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef max
#define max(x, y) (((x) < (y)) ? (y) : (x))
#endif

typedef enum
{
    FALSE_H = 0,
    TRUE_H = !FALSE_H
} BOOL;

#define MAX_PATH 512

#define CH341_PACKET_LENGTH 32
#define CH341_PKT_LEN_SHORT 8

#define CH341_MAX_NUMBER 16
#define MAX_BUFFER_LENGTH 0x1000
#define DEFAULT_BUFFER_LEN 0x0400

#define mCH341_PACKET_LENGTH 32
#define mCH341_MAX_NUMBER 16

#define CH347_USB_VENDOR 0
#define CH347_USB_HID 2
#define CH347_USB_VCP 3

// USB to SPI CMD
#define SET_CS 0
#define CLR_CS 1

#define USB20_CMD_SPI_INIT 0xC0    // It is used to initialize the SPI interface, set the data bits, clock frequency division, 
                                   // high and low byte order and other parameters of the SPI interface.
#define USB20_CMD_SPI_CONTROL 0xC1 // The SPI interface control command is used to control the SPI interface chip select pin 
                                   // to output high and low levels and the level delay time.
#define USB20_CMD_SPI_RD_WR 0xC2   // SPI interface routine read and write data commands, used for SPI interface general read
                                   // and write operations, generally used for short routine command operations. This command 
                                   // will read back N bytes of data while writing N bytes of data.
#define USB20_CMD_SPI_BLCK_RD 0xC3 // SPI interface batch read data command, used for SPI interface batch read data, generally 
                                   // used for batch data read operation. After enabling this command to read data, the read 
                                   // data will be packaged and uploaded according to the maximum package size until all the
                                   // data has been read and returned.
#define USB20_CMD_SPI_BLCK_WR 0xC4 // SPI interface batch write data command, used for SPI interface batch write data, generally
                                   // used for batch data write operation.SPI
#define USB20_CMD_INFO_RD 0xCA     // Parameter acquisition, used to obtain SPI interface related parameters, etc.

#define mMAX_BUFFER_LENGTH 0x1000
#define USB20_CMD_HEADER 3

#define SPI_CS_ACTIVE 0x00
#define SPI_CS_DEACTIVE 0x01

/* SPI_data_direction */
#define SPI_Direction_2Lines_FullDuplex ((USHORT)0x0000)
#define SPI_Direction_2Lines_RxOnly ((USHORT)0x0400)
#define SPI_Direction_1Line_Rx ((USHORT)0x8000)
#define SPI_Direction_1Line_Tx ((USHORT)0xC000)

/* SPI_mode */
#define SPI_Mode_Master ((USHORT)0x0104)
#define SPI_Mode_Slave ((USHORT)0x0000)

/* SPI_data_size */
#define SPI_DataSize_16b ((USHORT)0x0800)
#define SPI_DataSize_8b ((USHORT)0x0000)

/* SPI_Clock_Polarity */
#define SPI_CPOL_Low ((USHORT)0x0000)
#define SPI_CPOL_High ((USHORT)0x0002)

/* SPI_Clock_Phase */
#define SPI_CPHA_1Edge ((USHORT)0x0000)
#define SPI_CPHA_2Edge ((USHORT)0x0001)

/* SPI_Slave_Select_management */
#define SPI_NSS_Soft ((USHORT)0x0200)
#define SPI_NSS_Hard ((USHORT)0x0000)

#define mWIN32_COMMAND_HEAD 32 // Header length of the WIN32 command interface

/* SPI_MSB_LSB_transmission */
#define SPI_FirstBit_MSB ((USHORT)0x0000)
#define SPI_FirstBit_LSB ((USHORT)0x0080)

#define mMAX_BUFFER_LENGTH 0x1000 // The maximum length of the data buffer is 4096

#define mMAX_COMMAND_LENGTH (mWIN32_COMMAND_HEAD + mMAX_BUFFER_LENGTH) //Maximum data length plus the length of the command
                                                                       // structure header

#define mDEFAULT_BUFFER_LEN 0x0400 // The default length of the data buffer is 1024

#define mDEFAULT_COMMAND_LEN (mWIN32_COMMAND_HEAD + mDEFAULT_BUFFER_LEN) // The default data length plus the length of the
                                                                         // command structure header

/* SPI Init structure definition */
typedef struct _SPI_InitTypeDef
{
    USHORT SPI_Direction; /* Specifies the SPI unidirectional or bidirectional data mode.
                             This parameter can be a value of @ref SPI_data_direction */

    USHORT SPI_Mode; /* Specifies the SPI operating mode.
                        This parameter can be a value of @ref SPI_mode */

    USHORT SPI_DataSize; /* Specifies the SPI data size.
                            This parameter can be a value of @ref SPI_data_size */

    USHORT SPI_CPOL; /* Specifies the serial clock steady state.
                        This parameter can be a value of @ref SPI_Clock_Polarity */

    USHORT SPI_CPHA; /* Specifies the clock active edge for the bit capture.
                        This parameter can be a value of @ref SPI_Clock_Phase */

    USHORT SPI_NSS; /* Specifies whether the NSS signal is managed by
                       hardware (NSS pin) or by software using the SSI bit.
                       This parameter can be a value of @ref SPI_Slave_Select_management */

    USHORT SPI_BaudRatePrescaler; /* Specifies the Baud Rate prescaler value which will be
                                     used to configure the transmit and receive SCK clock.
                                     This parameter can be a value of @ref SPI_BaudRate_Prescaler.
                                     @note The communication clock is derived from the master
                                     clock. The slave clock does not need to be set. */

    USHORT SPI_FirstBit; /* Specifies whether data transfers start from MSB or LSB bit.
                              This parameter can be a value of @ref SPI_MSB_LSB_transmission */

    USHORT SPI_CRCPolynomial; /* Specifies the polynomial used for the CRC calculation. */
} SPI_InitTypeDef;

typedef struct _SpiUSBCFG
{
    SPI_InitTypeDef SPIInitCfg;
    USHORT SpiWriteReadInterval; // SPI interface conventional read and write data command (DEF_CMD_SPI_RD_WR)), the unit is uS
    UCHAR SpiOutDefaultData;     // When SPI reads data, the default output data
    UCHAR OtherCfg;              // 1 byte miscellaneous control;
                                 // Bit 7: chip select CS1 polarity control: 0: active low; 1: active active;
                                 // Bit 6: chip select CS2 polarity control: 0: active low; 1: active active;
                                 // Bit 5: IIC clock extension function control: 0: disabled; 1: enabled;
                                 // Bit 4: IIC generates or does not generate a NACK when the last 1 byte is completed;
                                 // Bits 3-0: Reserved;
    UCHAR Reserved[4];           // reserved
} SpiHwCfgS, *PSpiHwCfgS;

typedef struct _CH347_USB_CMD_S
{
    UCHAR mFunction;
    USHORT mLength;
    UCHAR mBuffer[512];
} CH347SPI_CMD, *mPCH347SPI_CMD;

typedef struct _StreamUSBCFG
{
    SPI_InitTypeDef SPIInitCfg;
    USHORT SpiWriteReadInterval; // SPI interface conventional read and write data command (DEF_CMD_SPI_RD_WR)), the unit is uS
    UCHAR SpiOutDefaultData;     // When SPI reads data, the default output data
    UCHAR OtherCfg;              // 1 byte miscellaneous control;
                                 // Bit 7: chip select CS1 polarity control: 0: active low; 1: active active;
                                 // Bit 6: chip select CS2 polarity control: 0: active low; 1: active active;
                                 // Bit 5: IIC clock extension function control: 0: disabled; 1: enabled;
                                 // Bit 4: IIC generates or does not generate a NACK when the last 1 byte is completed;
                                 // Bits 3-0: Reserved;
    UCHAR Reserved[4];           // reserved
} StreamHwCfgS, *PStreamHwCfgS;

#pragma pack(1)
// SPI controller configuration
typedef struct _SPI_CONFIG
{
    UCHAR iMode;                  // 0-3:SPI Mode0/1/2/3
    UCHAR iClock;                 // 0=60MHz, 1=30MHz, 2=15MHz, 3=7.5MHz, 4=3.75MHz, 5=1.875MHz, 6=937.5KHz,7=468.75KHz
    UCHAR iByteOrder;             // 0=low position first (LSB), 1= high position first(MSB)
    USHORT iSpiWriteReadInterval; // SPI the interface routinely reads and writes data commands, the unit is us
    UCHAR iSpiOutDefaultData;     // SPI output data by default when reading data
    ULONG iChipSelect;            // Chip selection control, if bit 7 is 0, the chip selection control is ignored, 
                                  // if bit 7 is 1, the parameter is valid: bit 1 and bit 0 are 00/01, respectively
                                  // select CS1/CS2 pins as low-level active chip selection
    UCHAR CS1Polarity;            // Bit 0: chip select CS1 polarity control: 0: active low; 1: active high;
    UCHAR CS2Polarity;            // Bit 0: chip select CS2 polarity control: 0: active low; 1: active high;
    USHORT iIsAutoDeativeCS;      // Whether to automatically undo the chip selection after the operation is completed
    USHORT iActiveDelay;          // Set the delay time for reading and writing operations after chip selection, the unit is us
    ULONG iDelayDeactive;         // Delay time for performing read and write operations after canceling chip selection, unit us
} mSpiCfgS, *mPSpiCfgS;

// Device Information
typedef struct _DEV_INFOR
{
    UCHAR iIndex;                // current open serial number
    UCHAR DevicePath[MAX_PATH];  // Device link name, used for CreateFile 
    UCHAR UsbClass;              // 0:CH347_USB_CH341, 2:CH347_USB_HID,3:CH347_USB_VCP
    UCHAR FuncType;              // 0:CH347_FUNC_UART,1:CH347_FUNC_SPI_I2C,2:CH347_FUNC_JTAG_I2C
    CHAR DeviceID[64];           // USB\VID_xxxx&PID_xxxx
    UCHAR ChipMode;              // Chip mode, 0:Mode0(UART0/1); 1:Mode1(Uart1+SPI+I2C); 2:Mode2(HID Uart1+SPI+I2C) 3:Mode3(Uart1+Jtag+IIC)
    int DevHandle;               // device handle
    USHORT BulkOutEndpMaxSize;   // upload endpoint size
    USHORT BulkInEndpMaxSize;    // Download endpoint size
    UCHAR UsbSpeedType;          // USB speed type,0:FS,1:HS,2:SS
    UCHAR CH347IfNum;            // Device interface number:0:UART,1:SPI/IIC/JTAG/GPIO
    UCHAR DataUpEndp;            // endpoint address
    UCHAR DataDnEndp;            // endpoint address
    CHAR ProductString[64];      // USB product string
    CHAR ManufacturerString[64]; // USB vendor string
    ULONG WriteTimeout;          // USB write timeout
    ULONG ReadTimeout;           // USB read timeout
    CHAR FuncDescStr[64];        // Interface Function Descriptor
    UCHAR FirewareVer;           // Firmware version
} mDeviceInforS, *mPDeviceInforS;

typedef struct _DevObj
{
    UCHAR iIndex;                // current open serial number
    UCHAR DevicePath[MAX_PATH];
    UCHAR UsbClass;              // 0:CH341 Vendor; 1:CH347 Vendor; 2:HID
    UCHAR FuncType;              // 0:UART1;        1:SPI+IIC;      2:JTAG+IIC
    CHAR DeviceID[64];           // USB\VID_xxxx&PID_xxxx
    UCHAR Mode;                  // chip mode, 0:Mode0(UART*2); 1:Mode1(Uart1+SPI+IIC); 2:Mode2(HID Uart1+SPI+IIC); 3:Mode3(Uart1+Jtag+IIC)
    USHORT BulkOutEndpMaxSize;   // upload endpoint size
    USHORT BulkInEndpMaxSize;    // Download endpoint size
    UCHAR UsbSpeedType;          // USB speed type 0:FS, 1:HS, 2:SS
    UCHAR CH347IfNum;            // USB interface number
    UCHAR DataUpEndp;            // endpoint address
    UCHAR DataDnEndp;            // endpoint address
    CHAR ProductString[64];      // USB product string
    CHAR ManufacturerString[64]; // USB vendor string
    ULONG WriteTimeout;          // USB write timeout
    ULONG ReadTimeout;           // USB read timeout
    CHAR FuncDescStr[64];
    UCHAR FirewareVer;           // Firmware version
    ULONG CmdDataMaxSize;
} mDevObjS, *mPDevObj;
#pragma pack()

//CH347 mode public function, support open, close, USB read, USB write in all modes of CH347, including HID CH347
// open usb device
int CH347OpenDevice(ULONG DevI);

// turn off the usb device
BOOL CH347CloseDevice(ULONG iIndex);

// read USB data block
BOOL CH347ReadData(ULONG iIndex,     // Specify the serial number of the device
                   PVOID oBuffer,    // Points to a buffer large enough to hold the read data
                   PULONG ioLength); // Point to the length unit, the length to be read when input, and the actual read length after return

// Write and fetch USB data block
BOOL CH347WriteData(ULONG iIndex,     // Specify the serial number of the device
                    PVOID iBuffer,    // Points to a buffer where the data to be written is placed
                    PULONG ioLength); // Point to the length unit, the length to be written is the length to be written when input, and the 
                                      // actual written length is returned after return

/***************SPI********************/
// SPI Controller initialization
BOOL CH347SPI_Init(ULONG iIndex, mSpiCfgS *SpiCfg);

// Get SPI controller configuration information
BOOL CH347SPI_GetCfg(ULONG iIndex, mSpiCfgS *SpiCfg);

//To set the chip selection state, you need to call CH347SPI_Init to set the CS before use
BOOL CH347SPI_ChangeCS(ULONG iIndex,   // Specify the serial number of the device
                       UCHAR iStatus); // 0=Cancel chip selection, 1=Set chip selection

// Set SPI chip select
BOOL CH347SPI_SetChipSelect(ULONG iIndex,           // Specify the serial number of the device
                            USHORT iEnableSelect,   // The lower eight bits are CS1, and the upper eight bits are CS2; the byte value is 
                                                    // 1 = set CS, 0 = ignore this CS setting
                            USHORT iChipSelect,     // The lower eight bits are CS1, and the upper eight bits are CS2; chip selection 
                                                    // output, 0=cancel chip selection, 1=set chip selection
                            ULONG iIsAutoDeativeCS, // The lower 16 bits are CS1, and the upper 16 bits are CS2; whether to cancel the
                                                    // chip selection automatically after the operation is completed
                            ULONG iActiveDelay,     // The lower 16 bits are CS1, and the upper 16 bits are CS2; set the delay time 
                                                    // for reading and writing operations after chip selection, the unit is us
                            ULONG iDelayDeactive);  // The lower 16 bits are CS1, and the upper 16 bits are CS2; the delay time for 
                                                    // reading and writing operations after canceling the chip selection, the unit is us

// SPI4 write data
BOOL CH347SPI_Write(ULONG iIndex,      // Specify the serial number of the device
                    ULONG iChipSelect, // Chip selection control, if bit 7 is 0, chip selection control is ignored, and bit 7 is 1 for 
                                       // chip selection operation
                    ULONG iLength,     // The number of bytes of data to be transferred
                    ULONG iWriteStep,  // The length of a single block to read
                    PVOID ioBuffer);   // Points to a buffer where the data to be written out from MOSI is placed

// SPI4 read data. No need to write data first, the efficiency is much higher than CH347SPI_WriteRead
BOOL CH347SPI_Read(ULONG iIndex,      // Specify the serial number of the device
                   ULONG iChipSelect, // Chip selection control, if bit 7 is 0, chip selection control is ignored, and bit 7 is 1 for
                                      // chip selection operation
                   ULONG oLength,     // The number of bytes to send
                   PULONG iLength,    // The number of bytes of data to be read
                   PVOID ioBuffer);   // Point to a buffer, place the data to be written from DOUT, and return the data read from DIN

// Handle SPI data stream, 4-wire interface
BOOL CH347SPI_WriteRead(ULONG iIndex,      // Specify the serial number of the device
                        ULONG iChipSelect, // Chip select control, if bit 7 is 0, ignore chip select control, if bit 7 is 1, operate chip select
                        ULONG iLength,     // The number of bytes of data to be transferred
                        PVOID ioBuffer);   // Point to a buffer, place the data to be written from DOUT, and return the data read from DIN

// Handle SPI data stream, 4-wire interface
BOOL CH347StreamSPI4(ULONG iIndex,      // Specify the serial number of the device
                     ULONG iChipSelect, // Chip selection control, if bit 7 is 0, the chip selection control is ignored, if bit 7 is 1, 
                                        // the parameter is valid
                     ULONG iLength,     // The number of bytes of data to be transferred
                     PVOID ioBuffer);   // Point to a buffer, place the data to be written from DOUT, and return the data read from DIN

/********IIC***********/
typedef enum _EEPROM_TYPE
{ // EEPROM type
    ID_24C01,
    ID_24C02,
    ID_24C04,
    ID_24C08,
    ID_24C16,
    ID_24C32,
    ID_24C64,
    ID_24C128,
    ID_24C256,
    ID_24C512,
    ID_24C1024,
    ID_24C2048,
    ID_24C4096
} EEPROM_TYPE;

//Set the serial port stream mode
BOOL CH347I2C_Set(ULONG iIndex, // Specify the serial number of the device
                  ULONG iMode); // Specify the mode, see the next line
// Bit 1-bit 0: I2C interface speed/SCL frequency, 00=low speed/20KHz, 01=standard/100KHz (default value), 10=fast/400KHz, 11=high speed/750KHz
// Other reserved, must be 0

// Set the hardware asynchronous delay, return soon after the call, and delay the specified number of milliseconds before the next stream operation 
BOOL CH347I2C_SetDelaymS(ULONG iIndex,  // Specify the serial number of the device
                         ULONG iDelay); // Specify the number of milliseconds to delay

// Process I2C data flow, 2-wire interface, the clock line is the SCL pin, and the data line is the SDA pin
BOOL CH347StreamI2C(ULONG iIndex,       // Specify the serial number of the device
                    ULONG iWriteLength, // The number of bytes of data to write out
                    PVOID iWriteBuffer, // Point to a buffer where the data to be written is placed, the first byte is usually the I2C device
                                        // address and the read/write direction bit
                    ULONG iReadLength,  // The number of bytes of data to read
                    PVOID oReadBuffer); // Points to a buffer and returns the read data

BOOL CH347ReadEEPROM(                   // Read data block from EEPROM, the speed is about 56K bytes
    ULONG iIndex,                       // Specify the device serial number of CH341
    EEPROM_TYPE iEepromID,              // Specify the EEPROM model
    ULONG iAddr,                        // Specifies the address of the data unit
    ULONG iLength,                      // The number of bytes of data to read
    PUCHAR oBuffer);                    // Points to a buffer and returns the read data

// Write data block to EEPROM
BOOL CH347WriteEEPROM(ULONG iIndex,          // Specify the serial number of the device
                      EEPROM_TYPE iEepromID, // Specify the EEPROM model
                      ULONG iAddr,           // Specifies the address of the data unit
                      ULONG iLength,         // The number of bytes of data to write out
                      PUCHAR iBuffer);       // Points to a buffer where the data to be written is placed

#endif
