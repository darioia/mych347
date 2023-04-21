#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "CH347LIB.h"
// gcc -Wall -g -I/usr/include/hidapi -lhidapi-hidraw ./src/myCH347PAR_Demo_hidapi.c -I./include -lch347 -L./lib
// sudo ./myCH347PAR_Demo_hidapi  

#define CMD_FLASH_SECTOR_ERASE 0x20
#define CMD_FLASH_BYTE_PROG 0x02
#define CMD_FLASH_READ 0x03
#define CMD_FLASH_RDSR 0x05
#define CMD_FLASH_WREN 0x06

#define CMD_FLASH_JEDEC_ID 0x9F

#define SPI_FLASH_PerWritePageSize 256

#ifndef CH34x_DEBUG
#define CH34x_DEBUG
#endif

#ifdef CH34x_DEBUG
#define dbg(format, arg...) printf(format "\n", ##arg);
#endif
#define err(format, arg...) printf("error line %d: " format "\n", __LINE__, ##arg)

ULONG iIndex = 1;
BOOL FlashDevIsOpened = false;
static struct timeval t1, t2;

BOOL CH347_SPI_Init(int fd)
{
   BOOL ret = false;
   mSpiCfgS spiDev = {0};

memset(&spiDev, 0, sizeof(spiDev));
//
CH347SPI_GetCfg(fd, &spiDev);

   // Init the SPI arg
   spiDev.iMode = 3;
   spiDev.iClock = 3;
   spiDev.iByteOrder = 1;
   spiDev.iSpiOutDefaultData = 0xFF;

   // Init CH347 SPI
   ret = CH347SPI_Init(fd, &spiDev);
   if (!ret)
   {
      err("Failed to init device");

      return false;
   }

   return true;
}

BOOL CH347_I2C_Init(int fd)
{
   int ret = -1;

   // Init CH347 I2C
   ret = CH347I2C_Set(fd, 3); // The IIC speed set 750K
   if (!ret)
   {
      err("Failed to init I2C");

      return false;
   }

   return true;
}

ULONG EndSwitch(ULONG dwVal)
{
   ULONG SV;

   ((PUCHAR)&SV)[0] = ((PUCHAR)&dwVal)[3];
   ((PUCHAR)&SV)[1] = ((PUCHAR)&dwVal)[2];
   ((PUCHAR)&SV)[2] = ((PUCHAR)&dwVal)[1];
   ((PUCHAR)&SV)[3] = ((PUCHAR)&dwVal)[0];

   return SV;
}

BOOL FLASH_IC_Check(int fd)
{
   unsigned int Flash_ID = 0;
   unsigned int dat = 0;
   unsigned int iLength = 0;
   UCHAR mBuffer[16] = {0};

   memset(mBuffer + 1, 0xFF, 3);
   mBuffer[0] = CMD_FLASH_JEDEC_ID;
   iLength = 3;
   if (CH347SPI_WriteRead(fd, 0x80, iLength + 1, mBuffer) == false)
      return (0xFFFFFFFF);
   else
   {
      mBuffer[0] = 0;
      memcpy(&dat, mBuffer, 4);
   }

   Flash_ID = EndSwitch(dat);
   printf("  Flash_ID: %X\n", Flash_ID);

   return true; // non c'era
}

unsigned int FLASH_RD_Block(int fd, unsigned int address, UCHAR *pbuf,
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
   if (!CH347SPI_Read(fd, 0x80, 4, &iLen, DBuf))
   {
      printf("FLASH_RD_Block %ld bytes failure.", iLen);

      return 0;
   }
   else
   {
      memcpy(pbuf, DBuf, len);
      return len;
   }
}

// FLASH byte read
BOOL FlashBlockRead(int fd)
{
   double UseT;
   ULONG DataLen, FlashAddr = 0, i;
   UCHAR DBuf[8192] = {0};
   CHAR  FmtStr1[8 * 1024 * 3 + 16] = "";

   if (!FlashDevIsOpened)
   {
      printf("Please turn on the device first\n");

      return false;
   }

   //Get the start address of FLASH read
   FlashAddr = 0x00;
   //Get the number of bytes read by FLASH, hexadecimal
   DataLen = 0x500;

   gettimeofday(&t1, NULL);
   DataLen = FLASH_RD_Block(fd, FlashAddr, DBuf, DataLen);
   gettimeofday(&t2, NULL);

//   int data_sec = t2.tv_sec - t1.tv_sec;
//   int data_usec = t2.tv_usec - t1.tv_usec;
//   UseT = ((float)data_sec + (float)data_usec / 1000000);
double data_t1 = t1.tv_sec*1000000 + t1.tv_usec;
double data_t2 = t2.tv_sec*1000000 + t2.tv_usec;
UseT = (data_t2 - data_t1); // in us

   if (DataLen < 1)
   {
      printf("Flash Read: Reading %ld bytes starting at address [%lX]..."
             "failed.\n", FlashAddr, DataLen);
   }
   else
   {
      printf("Flash Read: Read %ld bytes from address [%lX]..."
             "successful. Time spent %.3f ms\n", FlashAddr, DataLen, UseT/1000);
      { //Display FLASH data, hexadecimal display
         for (i = 0; i < DataLen; i++)
            sprintf(&FmtStr1[strlen(FmtStr1)], "%02X ", DBuf[i]);
         printf("Read: \n%s\n", FmtStr1);
      }
   }

   return true;
}

BOOL FLASH_WriteEnable(int fd)
{
   ULONG iLen = 0;
   UCHAR DBuf[128] = {0};

   DBuf[0] = CMD_FLASH_WREN;
   iLen = 0;

   return CH347SPI_WriteRead(fd, 0x80, iLen + 1, DBuf);
}

BOOL CH34xFlash_Wait(int fd)
{
   ULONG mLen, iChipselect;
   UCHAR mWrBuf[3];
   UCHAR status;

   mLen = 3;
   iChipselect = 0x80;
   mWrBuf[0] = CMD_FLASH_RDSR;
   do
   {
      mWrBuf[0] = CMD_FLASH_RDSR;
      if (CH347StreamSPI4(fd, iChipselect, mLen, mWrBuf) == false)
         return false;
      status = mWrBuf[1];
   } while (status & 1);

   return true;
}

BOOL CH34xSectorErase(int fd, ULONG StartAddr)
{
   ULONG mLen, iChipselect;
   UCHAR mWrBuf[4];

   if (FLASH_WriteEnable(fd /*index*/) == false) //daia: perche' index??
      return false;

   mWrBuf[0] = CMD_FLASH_SECTOR_ERASE;
   mWrBuf[1] = (UCHAR)(StartAddr >> 16 & 0xff);
   mWrBuf[2] = (UCHAR)(StartAddr >> 8 & 0xf0);
   mWrBuf[3] = 0x00;
   mLen = 4;
   iChipselect = 0x80;
   if (CH347StreamSPI4(fd, iChipselect, mLen, mWrBuf) == false)
      return false;

   if (CH34xFlash_Wait(fd) == false)
      return false;

   return true;
}

BOOL W25XXX_WR_Page(int fd, PUCHAR pBuf, ULONG address, ULONG len)
{
   ULONG iChipselect = 0x80;
   UCHAR mWrBuf[8192];

   if (!FLASH_WriteEnable(fd))
      return false;

   mWrBuf[0] = CMD_FLASH_BYTE_PROG;
   mWrBuf[1] = (UCHAR)(address >> 16);
   mWrBuf[2] = (UCHAR)(address >> 8);
   mWrBuf[3] = (UCHAR)address;
   memcpy(&mWrBuf[4], pBuf, len);

   if (CH347SPI_Write(fd, iChipselect, len + 4,
                      SPI_FLASH_PerWritePageSize + 4, mWrBuf) == false)
      return false;

   memset(mWrBuf, 0, sizeof(UCHAR) * len);
   if (!CH34xFlash_Wait(fd))
      return false;

   return true; //non c'era
}

BOOL FlashBlockWrite(int fd)
{
   ULONG i = 0;
   ULONG DataLen, FlashAddr, BeginAddr, NumOfPage, NumOfSingle;
   UCHAR DBuf[8 * 1024 + 16] = {0};
   PUCHAR pbuf;
   double UseT;

   //Get the starting address of writing FLASH, hexadecimal
   FlashAddr = 0x00;
   BeginAddr = FlashAddr;
   //Get the number of bytes written to FLASH, hexadecimal
   DataLen = 0;

   for (i = 0; i < 1280; i++)
   {
      DBuf[i] = 0x89;
      DataLen++;
   }

   pbuf = DBuf;

   NumOfPage = DataLen / SPI_FLASH_PerWritePageSize;
   NumOfSingle = DataLen % SPI_FLASH_PerWritePageSize;

   gettimeofday(&t1, NULL);
   if (NumOfPage == 0)
   {
      W25XXX_WR_Page(fd, DBuf, FlashAddr, DataLen);
   }
   else
   {
      while (NumOfPage--)
      {
         W25XXX_WR_Page(fd, pbuf, FlashAddr, SPI_FLASH_PerWritePageSize);
         pbuf += SPI_FLASH_PerWritePageSize;
         FlashAddr += SPI_FLASH_PerWritePageSize;
      }
      if (NumOfSingle)
         W25XXX_WR_Page(fd, pbuf, FlashAddr, NumOfSingle);
   }
   gettimeofday(&t2, NULL);

//   int data_sec = t2.tv_sec - t1.tv_sec;
//   int data_usec = t2.tv_usec - t1.tv_usec;
//   UseT = ((float)data_sec + (float)data_usec/1000000);
double data_t1 = t1.tv_sec*1000000 + t1.tv_usec;
double data_t2 = t2.tv_sec*1000000 + t2.tv_usec;
UseT = (data_t2 - data_t1); // in us

   if (DataLen < 1)
   {
      printf("Flash Write: Writing %ld bytes starting at address [%lX]..."
             "failed\n", BeginAddr, DataLen);
   }
   else
   {
      printf("Flash Write: Write %ld bytes starting from address [%lX]..."
             "successful. Time spent %.3f ms\n", BeginAddr, DataLen, UseT/1000);
   }

   return true;
}

BOOL EEPROM_Write(int fd)
{
   ULONG i = 0;
   ULONG DataLen = 0;
   UCHAR DBuf[8 * 1024 + 16] = {0};
   BOOL RetVal = false;

   printf("Ready to write.\n");
   for (i = 0; i <= 255; i++)
   {
      DBuf[i] = 0x45;
      DataLen++;
   }

   printf("Write EEPROM data:\n");
   RetVal = CH347WriteEEPROM(fd, ID_24C02, 0, DataLen, DBuf);

   for (i = 0; i <= 255; i++)
   {
      printf("%02x ", DBuf[i]);
      if (((i + 1) % 10) == 0)
         putchar(10);
   }

   putchar(10);

   return RetVal;
}

BOOL EEPROM_Read(int fd)
{
   ULONG i = 0;
   ULONG DataLen = 256;
   UCHAR DBuf[8 * 1024 + 16] = {0};
   BOOL RetVal = false;

   RetVal = CH347ReadEEPROM(fd, ID_24C02, 0, DataLen, DBuf);

   printf("Read EEPROM data:\n");
   for (i = 0; i <= 255; i++)
   {
      printf("%02x ", DBuf[i]);
      if (((i + 1) % 10) == 0)
         putchar(10);
   }

   putchar(10);

   return RetVal;
}

static int get_menu(void)
{
   int choose = -1;

   printf("******************************************\n");
   printf("***   Enter Num Choose Funcation Test ****\n");
   printf("******************************************\n");
   printf("****                                  ****\n");
   printf("****          0 EXIT                  ****\n");
   printf("****          1 FLASH TEST            ****\n");
   printf("****          2 EEPROM TEST           ****\n");
   printf("****          3 JTAG TEST             ****\n");
   printf("****          4 GPIO Test             ****\n");
   printf("****                                  ****\n");
   printf("******************************************\n");
   printf("Your choose:\n");
   choose = getchar() - '0';
   getchar();

   return choose;
}

void FLASH_Test(int fd)
{
   int ret = -1;

   ret = CH347_SPI_Init(fd);
   if (!ret)
   {
      err("Failed to init CH347 SPI.");
      //exit(-1);
return;
   }

   // // Read the Flash ID
   ret = FLASH_IC_Check(fd);
   if (!ret)
   {
      err("Failed to find flash");
      //exit(-1);
return;
   }

   // Read the Flash data
   ret = FlashBlockRead(fd);
   if (!ret)
   {
      err("Failed to read flash");
      //exit(-1);
return;
   }

   // Erase the flash data
   ret = CH34xSectorErase(fd, 0x00);
   if (!ret)
   {
      err("Failed to erase flash");
      //exit(-1);
return;
   }

   // Write the flash data
   ret = FlashBlockWrite(fd);
   if (!ret)
   {
      err("Failed to write flash");
      //exit(-1);
return;
   }

   // Check the flash data
   ret = FlashBlockRead(fd);
   if (!ret)
   {
      err("Failed to read flash");
      //exit(-1);
return;
   }

   return;
}

void EEPROM_Test(int fd)
{
   int ret = -1;
   printf("Enter the EEPROM Test mode.\n");

   ret = CH347_I2C_Init(fd);
   if (!ret)
   {
      err("Failed to init CH347 I2C.");
     //exit(-1);
return;
   }

   ret = EEPROM_Read(fd);
   if (!ret)
   {
      err("Failed to read eeprom");
      //exit(-1);
return;
   }
   ret = EEPROM_Write(fd);
   if (!ret)
   {
      err("Failed to write eeprom");
      //exit(-1);
return;
   }

   ret = EEPROM_Read(fd);
   if (!ret)
   {
      err("Failed to read eeprom");
      //exit(-1);
return;
   }
}

void JTAG_Test(int fd)
{
   UCHAR retValue[32] = "";
   ULONG i, len = 32;

   // Init the TCK clock
   CH347Jtag_INIT(fd, 4);

   // Reset the Target
   CH347Jtag_SwitchTapState(fd, 0);
   // SHIFT-DR Read the Target IDCODE
   CH347Jtag_ByteReadDR(fd, &len, &retValue);

   printf("Target IDCODE: 0x");
   for (i = 0; i < 4; i++)
   {
      printf("%02x", retValue[3 - i]);
   }
   puts("");

   return;
}

void GPIO_Test(int fd)
{
   int i = 0;
   UCHAR iEnable = 0xff;
   UCHAR iOutDir = 0xff;
   UCHAR iOutData = 0x00;

   // init all gpio output low-bit 
   CH347GPIO_Set(fd, iEnable, iOutDir, iOutData);

int cnt = 0;
   for (i = 0; i <= 8; i++)
   {
      iOutData |= (1 << i) & 0xFF;
      CH347GPIO_Set(fd, iEnable, iOutDir, iOutData);
      iOutData = 0x00;
      CH347GPIO_Set(fd, iEnable, iOutDir, iOutData);
printf("%d ", cnt++);
   }
printf("\n");
}

// 2 0x55dc Config 01 (hidapi) ok I2C/GPIO/JTAG nok SPI
// 1 0x55db Config 10          ok I2C/GPIO/JTAG nok SPI
int main(void)
{
   int ret = 0;

   system("clear");

   // Open the device
   int fd = CH347OpenDevice(0);
   if (fd <= 0)
   {
      printf("Failed to open device.\n");

      return -1;
   }

printf("fd=%d\n", fd);
   FlashDevIsOpened = true;
   while (ret == 0)
   {
      switch (get_menu())
      {
      case 1:
         printf("FLASH Test.\n");
   int fd2 = CH347OpenDevice(1);
   if (fd2 <= 0)
   {
      printf("Failed to open device.\n");

      return -1;
   }
         FLASH_Test(fd2 /*+ iIndex*/);
   // Close the CH347 Device
   if (CH347CloseDevice(fd2))
   {
      printf("Close device succesed\n");
   }
         break;
      case 2:
         printf("EEPROM Test.\n");
         EEPROM_Test(fd + iIndex);
         break;
      case 3:
         printf("JTAG Test.\n");
         JTAG_Test(fd + iIndex);
         break;
      case 4:
         printf("GPIO Test.\n");
         GPIO_Test(fd + iIndex);
         break;

      case 0:
         ret = 1;
         break;

      default:
         printf("Please enter the number[0/1/2/3].\n");
         break;
      }
   }

   int retVal = -1;
   // Init CH347 I2C
   // The IIC speed set 750K (daia: no 100KHz)
   retVal = CH347I2C_Set(fd + iIndex, 1); 
   if (!retVal)
   {
      err("Failed to init I2C");

      return false;
   }
   char buf[2] = {0x50, 0xaa};
   CH347StreamI2C(fd + iIndex, 2, buf, 0, NULL);

   // Close the CH347 Device
   if (CH347CloseDevice(fd))
   {
      printf("Close device succesed\n");
   }

   FlashDevIsOpened = false;

   return 0;
}
