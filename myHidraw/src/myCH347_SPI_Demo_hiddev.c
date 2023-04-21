#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include "CH347SPILIB.h"

#include <sys/time.h>
// gcc -Wall -g -I/usr/include/hidapi -lhidapi-hidraw ./src/myCH347_SPI_Demo_hiddev.c -I./include -lch347spi -L./lib
// sudo ./myCH347_SPI_Demo_hiddev 


#define CMD_FLASH_SECTOR_ERASE 0x20
#define CMD_FLASH_BYTE_PROG    0x02
#define CMD_FLASH_READ         0x03
#define CMD_FLASH_RDSR         0x05
#define CMD_FLASH_WREN         0x06

#define CMD_FLASH_JEDEC_ID     0x9F

#define SPI_FLASH_PerWritePageSize 256

#ifndef CH34x_DEBUG
#define CH34x_DEBUG
#endif

#ifdef CH34x_DEBUG
#define dbg( format, arg...)   printf(format "\n", ##arg);
#endif
#define err( format, arg... )  printf("error line %d: " format "\n", __LINE__, ##arg)

int iIndex=1;
BOOL FlashDevIsOpened = FALSE;
static struct timeval t1, t2;

BOOL CH347_SPI_init(int dev, int *fd)
{
int dev_fd = -1;
    BOOL ret = FALSE;
    mSpiCfgS spiDev = { 0 };

    system("clear");
    // Open the device
    dev_fd = CH347OpenDevice(dev);
    if (dev_fd < 0)
	 {
        err("Failed to open device");
*fd = -1;
        return FALSE;
    }
printf("11111\n");

*fd = dev_fd;
//memset(&spiDev, 0, sizeof(spiDev));
//CH347SPI_GetCfg(*fd, &spiDev);

    FlashDevIsOpened = TRUE;

    spiDev.iMode = 3;
    spiDev.iClock = 1;     // 30MHz 
    spiDev.iByteOrder = 1; // 1=high position first(MSB)
    spiDev.iSpiOutDefaultData = 0xFF;

//spiDev.iChipSelect = 0x80; // CS1
//spiDev.CS1Polarity = 0;    // active low;

    // Init CH347 SPI 
    ret = CH347SPI_Init(dev_fd, &spiDev);
    if (!ret)
    {
        err("Failed to init device");
        
        return FALSE;
    }
printf("22222\n");

    
/*daiatest
SpiHwCfgS myHwData;
HwConfigToUserSpiCfg(*fd, &myHwData);
printf("SPI_Mode: 0x%04X\n", myHwData.SPIInitCfg.SPI_Mode);
*/


#ifdef ALSO_IIC

   // Init CH347 IIC
   ret = CH347I2C_Set(dev_fd, 3);   // The IIC speed set 750K
   if (!ret)
   {
      err("Failed to init I2C");
      
      return FALSE;
   }
#endif

    return TRUE;
}


#ifdef ALSO_IIC

ULONG EndSwitch(ULONG dwVal)
{
   ULONG SV;

   ((PUCHAR)&SV)[0] = ((PUCHAR)&dwVal)[3];
   ((PUCHAR)&SV)[1] = ((PUCHAR)&dwVal)[2];
   ((PUCHAR)&SV)[2] = ((PUCHAR)&dwVal)[1];
   ((PUCHAR)&SV)[3] = ((PUCHAR)&dwVal)[0];
   return SV;
}

BOOL FLASH_IC_Check(int dev_fd)
{
   unsigned int Flash_ID = 0;
   unsigned int dat = 0;
   unsigned int iLength = 0;

   UCHAR mBuffer[16] = { 0 };
   memset(mBuffer+1, 0xFF, 3);
   mBuffer[0] = CMD_FLASH_JEDEC_ID;
   iLength = 3;
   if (CH347SPI_WriteRead(dev_fd, 0x80, iLength + 1, mBuffer) == FALSE)
     return (0xFFFFFFFF);
   else
   {
      mBuffer[0] = 0;
      memcpy(&dat, mBuffer, 4);
   }

    Flash_ID = EndSwitch(dat);
    printf("  Flash_ID: %X\n", Flash_ID);
    
    return TRUE;//daia non c'era
}

unsigned int FLASH_RD_Block(int dev_fd, unsigned int address, UCHAR *pbuf,
                            unsigned int len)
{
   /* W25 series FLASH, SST series FLASH */
   ULONG iLen = 0;
   UCHAR DBuf[8192] = {0};

   DBuf[0] = CMD_FLASH_READ;
   DBuf[1] = (UCHAR)(address >> 16);
   DBuf[2] = (UCHAR)(address >> 8);
   DBuf[3] = (UCHAR)(address);

   iLen = len;
int ret = CH347SPI_Read(dev_fd, 0x80, 4, &iLen, DBuf);
   if (ret != 0)
   {
      printf("FLASH_RD_Block %d bytes failure.", ret); //daia messo ret
      return 0;
   }
   else
   {
      memcpy(pbuf, DBuf, len);
      return len;
   }
}

// FLASH byte read
BOOL FlashBlockRead(int dev_fd)
{
   double UseT;
   ULONG DataLen, FlashAddr = 0, i;
   UCHAR DBuf[8192] = {0};
   CHAR /*FmtStr[512] = "",*/ FmtStr1[8 * 1024 * 3 + 16] = "";

   if (!FlashDevIsOpened)
   {
      printf("Please turn on the device first");
      return FALSE;
   }

   //Get the start address of FLASH read
   FlashAddr = 0x00;
   //Get the number of bytes read by FLASH, hexadecimal
   DataLen = 0x100;

   gettimeofday(&t1, NULL);
   DataLen = FLASH_RD_Block(dev_fd, FlashAddr, DBuf, DataLen);
   gettimeofday(&t2, NULL);

//   int data_sec = t2.tv_sec - t1.tv_sec;
//   int data_usec = t2.tv_usec - t1.tv_usec;
//   UseT = ((float)data_sec + (float)data_usec / 1000000);
double data_t1 = t1.tv_sec*1000000 + t1.tv_usec;
double data_t2 = t2.tv_sec*1000000 + t2.tv_usec;
UseT = (data_t2 - data_t1); // in us

   if (DataLen < 1)
   {
      printf(">>Flash Read: Reading %ld bytes starting at address [%lX]..."
             "failed.\n", FlashAddr, DataLen);
   }
   else
   {
      printf(">>Flash Read: Read %ld bytes from address [%lX]..."
             "successful. Time spent %.3f ms\n", FlashAddr, DataLen, UseT/1000);
      { //Display FLASH data, hexadecimal display
         for (i = 0; i < DataLen; i++)
            sprintf(&FmtStr1[strlen(FmtStr1)], "%02X ", DBuf[i]);
         printf("Read: %s\n", FmtStr1);
      }
   }
   return TRUE;
}

BOOL FLASH_WriteEnable(int dev_fd)
{
   ULONG iLen = 0;
   UCHAR DBuf[128] = {0};

   DBuf[0] = CMD_FLASH_WREN;
   iLen = 0;
   return CH347SPI_WriteRead(dev_fd, 0x80, iLen + 1, DBuf);
}

BOOL CH34xFlash_Wait(int dev_fd)
{
   ULONG mLen, iChipselect;
   UCHAR mWrBuf[3];
   UCHAR status;
   mLen = 3;
   iChipselect = 0x80;
   mWrBuf[0] = CMD_FLASH_RDSR;
   do{
      mWrBuf[0] = CMD_FLASH_RDSR;
      if (CH347StreamSPI4(dev_fd, iChipselect, mLen, mWrBuf) == FALSE )
         return FALSE;
      status = mWrBuf[1];
   } while( status & 1 );
   return true;
}

BOOL CH34xSectorErase(int dev_fd, ULONG StartAddr)
{
   ULONG mLen, iChipselect;
   UCHAR mWrBuf[4];
   if (FLASH_WriteEnable(dev_fd) == FALSE)
      return FALSE;
   mWrBuf[0] = CMD_FLASH_SECTOR_ERASE;
   mWrBuf[1] = (UCHAR)( StartAddr >> 16 & 0xff );
   mWrBuf[2] = (UCHAR)( StartAddr >> 8 & 0xf0 );
   mWrBuf[3] = 0x00;
   mLen = 4;
   iChipselect = 0x80;
   if (CH347StreamSPI4(dev_fd, iChipselect, mLen, mWrBuf) == FALSE )
      return FALSE;

   if (CH34xFlash_Wait(dev_fd) == FALSE )
      return FALSE;
   return true;
}

BOOL FlashBlockWrite(int dev_fd)
{
   ULONG i = 0;
   ULONG iChipselect = 0x80;
   UCHAR mWrBuf[8192];
   ULONG DataLen, FlashAddr;
   UCHAR DBuf[8 * 1024 + 16] = {0};
   UCHAR FmtStr[8 * 1024 * 3 + 16] = "";
   double UseT = 0;

   //Get the starting address of writing FLASH, hexadecimal
   FlashAddr = 0x00;
   //Get the number of bytes written to FLASH, hexadecimal
   DataLen = 0;

   for (i = 0; i < 256; i++)
   {
      FmtStr[i] = 256 - i;
   }
   for (i = 0; i < 256; i++)
   {
      DBuf[DataLen] = FmtStr[i];
      DataLen++;
   }

   if (!FLASH_WriteEnable(dev_fd))
      return FALSE;

   mWrBuf[0] = CMD_FLASH_BYTE_PROG;
   mWrBuf[1] = (UCHAR)(FlashAddr >> 16);
   mWrBuf[2] = (UCHAR)(FlashAddr >> 8);
   mWrBuf[3] = (UCHAR)FlashAddr;
   memcpy(&mWrBuf[4], DBuf, DataLen);
   DataLen += 4;
 
// messo io per misurare il tempo di esecuzione
gettimeofday(&t1, NULL);
   if (CH347SPI_Write(dev_fd, iChipselect, DataLen, 
                      SPI_FLASH_PerWritePageSize+4,mWrBuf) == FALSE )
      return FALSE;

// messo io per misurare il tempo di esecuzione
gettimeofday(&t2, NULL);
double data_t1 = t1.tv_sec*1000000 + t1.tv_usec;
double data_t2 = t2.tv_sec*1000000 + t2.tv_usec;
UseT = (data_t2 - data_t1); // in us
//int data_sec = t2.tv_sec - t1.tv_sec;
//int data_usec = t2.tv_usec - t1.tv_usec;
//UseT = ((float)data_sec + (float)data_usec / 1000000);
// fine

   memset( mWrBuf, 0, sizeof( UCHAR ) * DataLen);
   if (!CH34xFlash_Wait(dev_fd) )
      return FALSE;

   if (DataLen < 1)
   {
      printf(">>Flash Write: Writing %ld bytes starting at address [%lX]..."
             "failed\n", FlashAddr, DataLen);
   }
   else
   {
      printf(">>Flash Write: Write %ld bytes from [%lX] address...successful."
             " Time spent %.3f ms\n", FlashAddr, DataLen, UseT/1000);
   }
   return TRUE;
}

BOOL EEPROM_Write(int dev_fd)
{
   ULONG i = 0;
   ULONG DataLen = 0;
   UCHAR DBuf[8 * 1024 + 16] = {0};
   BOOL RetVal = FALSE;

   printf("ready to write.\n");
   for (i = 0; i <= 255; i++)
   {
      DBuf[i] = 255 - i;
      DataLen++;
   }
   
   printf("Write EEPROM data:\n");
   RetVal = CH347WriteEEPROM(dev_fd, ID_24C02,0x00,DataLen,DBuf);

   for (i = 0; i <= 255; i++)
   {
      printf("%02x ", DBuf[i]);
      if (((i+1) % 10) == 0) 
         putchar(10);
   }

   putchar(10);

   return RetVal;
}

BOOL EEPROM_Read(int dev_fd)
{
   ULONG i = 0;
   ULONG DataLen = 256;
   UCHAR DBuf[8 * 1024 + 16] = {0};

   BOOL RetVal = FALSE;

   RetVal = CH347ReadEEPROM(dev_fd, ID_24C02, 0, DataLen,DBuf);

   printf("Read EEPROM data:\n");
   for (i = 0; i < 255; i++)
   {
      printf("%02x ", DBuf[i]);
      if (((i+1) % 10) == 0) 
         putchar(10);
   }

   putchar(10);

   return RetVal;
}
#endif




int main(void)
{
    int fd = -1;
    BOOL ret = FALSE;

    // Open the CH347 Device
    // in origine era 0 (open /dev/ch34x_pis error, LINE : 55)
    ret = CH347_SPI_init(iIndex, &fd); 
    if (!ret)
    {
        err("Failed to init CH347 SPI.");

        return -1;
    }
    
    printf("Open OK\n");
    

#ifdef ALSO_IIC
    // Read the Flash ID
    ret = FLASH_IC_Check(fd);
    if (!ret)
    {
        err("Failed to find flash");
CH347CloseDevice(fd);
return -1;
    }

    // Read the Flash data
    ret = FlashBlockRead(fd);
    if (!ret)
    {
        err("Failed to read flash");
CH347CloseDevice(fd);
return -1;
    }

    // Erase the flash data
    ret = CH34xSectorErase(fd, 0x00);
    if (!ret)
    {
        err("Failed to erase flash");
CH347CloseDevice(fd);
return -1;
    }

    // Write the flash data
    ret = FlashBlockWrite(fd);
    if (!ret)
    {
        err("Failed to write flash");
CH347CloseDevice(fd);
return -1;
    }

    // Read the Flash data
    ret = FlashBlockRead(fd);
    if (!ret)
    {
        err("Failed to read flash");
CH347CloseDevice(fd);
return -1;
    }
#endif



#ifdef FLASH_ACCESS
   // EEPROM 
   ret = EEPROM_Read(fd);
   if (!ret) 
   {
        err("Failed to read eeprom");
CH347CloseDevice(fd);
return -1;
   }

   ret = EEPROM_Write(fd);
   if (!ret)
   {
        err("Failed to write eeprom");
CH347CloseDevice(fd);
return -1;
    }

   ret = EEPROM_Read(fd);
	if (!ret)
    {
        err("Failed to read eeprom");
CH347CloseDevice(fd);
return -1;
    }
#endif


   // Close the CH347 Device
   if (CH347CloseDevice(fd))
   {
      FlashDevIsOpened = FALSE;
      printf("Close device successed\n");
   }
   printf("Close OK\n");
   
   return 0;
}
