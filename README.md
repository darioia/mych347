# mych347 Examples

Remember: The Ch347 module is able to handle only Master mode for SPI and I2C.


In Driver:

     the original driver for ch347 device patched for removed all warning

In myHidraw:

     the first demo working (the SPI has some problem) with warning clean and some changes only library test purpose
 
In demo:

     the userspace application working (without real test on target a part USB adapter) with warning cleanup


# Datasheet CH347, driver and windows demo:

http://www.wch-ic.com/search?t=all&q=ch347


# Links:

https://blog.tindie.com/2022/08/usb-to-everything-chip/

https://github-com.translate.goog/WCHSoftGroup/ch347?_x_tr_sl=auto&_x_tr_tl=en&_x_tr_hl=en

https://community.platformio.org/t/a-cheap-usb2jtag-chip-ch347-for-esp32-debugging/29159

http://www.wch-ic.com/products/CH347.html?

https://twitter.com/JohnnyW11773607/status/1563049957409431553?lang=en

https://github.com/wuxx/USB-HS-Bridge

https://github.com/WCHSoftGroup/ch347.git


# CH347 driver

1) kernel driver path ..../mych347/Driver (tacken from CH347Par_Linux_V1.02_HID.zip):

     cd /usr/src/kernels/linux-\`uname -r\` (test with success on 6.2.10)
     
     make -C ..../mych347/Driver
     
     sudo make -C ..../mych347/Driver install
     
     sudo modprobe -a ch34x-pis


2) add the following line in /etc/modules-load.d/modules.conf

     ch34x-pis


# Userspace

2)  Copy the library files (opt):

     ..../USB-HS-Bridge/doc/CH347Par_Linux_V1.02_HID.zip:/Libs/X86_64/libc347.so -> /usr/local/lib64
     
     ..../USB-HS-Bridge/doc/CH347SPI_LINUX_VTEST.tar:/CH347SPI_LINUX_VTEST/lib/x86_64/libch347spi.so -> /usr/local/lib64
     
     sudo ldconfig

3) Userspace hidraw demo here ..../mych347/myHidraw    (for  I2C/SPI/JTAG/GPIO)

   language translation from chinese language, fixed (I hope) file descriptor handling and all warning removed

4) Userspace ch347_demo here ..../mych347/demo  (for  I2C/SPI/JTAG/GPIO)

   language translation from chinese language and all warning removed


PS: the binary format library files are not present here, I use the files catched from several tar/zip files
