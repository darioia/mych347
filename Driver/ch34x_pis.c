/*
 * ch341 multi-IO(Epp/MEM/I2C/SPI/GPIO) driver - Copyright (C) 2020 WCH Corporation.
 * Author: TECH39 <zhangj@wch.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Version: V1.05
 *
 * Update Log:
 * V1.00 - initial version
 * V1.04 - fixed ioctls bugs when copy data from user space
 * V1.05 - fixed write & read & ioctls bugs
 * V1.06 - add CH347 support
 */
 
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/kref.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/usb.h>
#include <linux/version.h>

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(ver, rel, seq) 	((ver << 16) | (rel <<8) | (seq))
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0))
	#include <asm/uaccess.h>
#else
	#include <linux/uaccess.h>
#endif

#define DEBUG 1
//#undef DEBUG

#ifdef DEBUG
#define dbg(format, arg...)	\
    printk(KERN_DEBUG ""format "\n", ## arg)
#else
#define dbg( format, arg... )	do{} while(0)
#endif

#define err( format, arg... )	\
    printk( KERN_ERR "%s %d:" format "\n", __FILE__, __LINE__, ##arg)


#define CH34x_VENDOR_ID			0x1A86	//Vendor Id
#define CH34x_PRODUCT_ID		0x5512	//Product Id
#define DRV_NAME    			"ch34x_pis"
#define DRV_VERSION	    		"WCH CH34x Driver Version V1.05"
    
#define DRIVER_AUTHOR "TECH39"
#define DRIVER_DESC   "Multi-IO driver for usb to multi interface chip ch341."
#define VERSION_DESC  "V1.06 On 2022.03.05"


#define CH34x_MINOR_BASE		200	//
#define WRITES_IN_FLIGHT		8
#define CH34x_READ_SHORT		8
#define CH34x_PACKET_LENGTH		32
#define CH347_PACKET_LENGTH		512
#define MAX_BUFFER_LENGTH		0x1000

//Vendor define
#define VENDOR_WRITE_TYPE		0x40	//vendor write command
#define VENDOR_READ_TYPE		0XC0	//vendor read command

//
#define CH34x_PARA_INIT			0xB1	// Init Parallel	
#define CH34x_I2C_STATUS		0x52    //get I2C status
#define CH34x_I2C_COMMAND		0x53	//send I2C command

#define CH34x_BUF_CLEAR         0xB2	//clear uncompleted data
#define CH34x_I2C_CMD_X			0x54	//send I2C command
#define CH34x_DELAY_MS			0x5E
#define VENDOR_VERSION 			0x5F	//get version of chip

#define CH34x_PARA_CMD_R0		0xAC	//read data0 from Para
#define CH34x_PARA_CMD_R1		0xAD	//read data1 from Para
#define CH34x_PARA_CMD_W0		0xA6	//write data0 to Para
#define CH34x_PARA_CMD_W1		0xA7	//write data1 to Para
#define CH34x_PARA_CMD_STS		0xA0	//get status of Para

//CH341 COMMAND
#define CH34x_CMD_SET_OUTPUT	0xA1	//set Para output
#define CH34x_CMD_IO_ADDR		0xA2	//MEM IO Addr
#define CH34x_CMD_PRINT_OUT		0xA3	//print output
#define CH34X_CMD_SPI_STREAM	0xA8	//SPI command
#define CH34x_CMD_SIO_STREAM	0xA9	//SIO command
#define CH34x_CMD_I2C_STREAM	0xAA	//I2C command
#define CH34x_CMD_UIO_STREAM	0xAB	//UIO command

#define	CH341A_CMD_UIO_STM_IN	0x00	 // UIO Interface In ( D0 ~ D7 )
#define	CH341A_CMD_UIO_STM_DIR	0x40	 // UIO interface Dir( set dir of D0~D5 )
#define	CH341A_CMD_UIO_STM_OUT	0x80	 // UIO Interface Output(D0~D5)
#define	CH341A_CMD_UIO_STM_US	0xC0	 // UIO Interface Delay Command( us )
#define	CH341A_CMD_UIO_STM_END	0x20	 // UIO Interface End Command

// add CH347 CMD
#define MODE1_INTERFACE_NUM		0x02

#define USB20_CMD_UART1_INIT    0xCB	// 串口1初始化,用于串口1初始化
#define USB20_CMD_GPIO_OP       0xCC	// GPIO口操作,用于GPIO口控制操作
#define USB20_CMD_JUMP_IAP      0xCD	// 跳转进入IAP,用于控制跳转进入IAP模式
#define USB20_CMD_BIT_STREAM    0xDA	// GPIO数据流输入输出控制命令

//Single read/write the MAX number of blocks in EPP/MEM
//#define CH34x_EPP_IO_MAX			( CH34x_PACKET_LENGTH - 1 )
#define CH34x_EPP_IO_MAX			( CH347_PACKET_LENGTH )
//CH341A
#define CH34xA_EPP_IO_MAX			0xFF

//request
#define CH34x_DEBUG_READ			0x95  //read two regs
#define CH34x_DEBUG_WRITE			0x9A //write two regs

#define REQUEST_TYPE_READ			( USB_DIR_IN |USB_TYPE_VENDOR | USB_RECIP_OTHER )
#define REQUEST_TYPE_WRITE		( USB_DIR_OUT | USB_TYPE_VENDOR |USB_RECIP_OTHER)

//Ioctl cmd Codes
#define CH34x_GET_DRV_VERSION		0x80000001
#define CH34x_CHIP_VERSION			0x80000003
#define CH34x_FUNCTION_SETPARA_MODE	0x80000004
#define CH34x_FUNCTION_READ_MODE	0x80000005
#define CH34x_FUNCTION_WRITE_MODE	0x80000006
#define CH34x_I2C_READ_MODE			0x80000007
#define CH34x_I2C_WRITE_MODE		0x80000008
#define CH34x_PIPE_DATA_DOWN		0x80000009
#define CH34x_PIPE_WRITE_READ		0x8000000a
#define CH34x_PIPE_DEVICE_CTRL		0x8000000b


static unsigned char Read_Mode;	//Read Data Pipe Mode From Para
static unsigned char Write_Mode;	//Write Data Pipe Mode From Para

struct ch34x_pis{	
	struct usb_device    *udev;		/*the usb device for this device*/
	struct usb_interface *interface;	/*the interface for this device*/	
	struct usb_endpoint_descriptor *interrupt_in_endpoint;

	size_t interrupt_in_size;		/*the size of rec data (interrupt)*/
	unsigned char *interrupt_in_buffer;	/*the buffer of rec data (interface)*/
	struct urb *interrupt_in_urb;

	size_t bulk_in_size;			/*the size of rec data (bulk)*/
	unsigned char *bulk_in_buffer;		/*the buffer of rec data (bulk)*/
	struct urb *read_urb;			/*the urb of bulk_in*/
	__u8	bulk_in_endpointAddr;		/*bulk input endpoint*/
	__u8	bulk_out_endpointAddr;		/*bulk output endpoint*/
	unsigned char *bulk_out_buffer;
	
	struct semaphore limit_sem;		/*semaphore*/
	struct usb_anchor submitted;		/*usb anchor */
	
	unsigned long VenIc;			/*Chip Version(CH341A : 0x0030)*/
	int errors;
	int open_count;				/*count the number of openers*/
	spinlock_t err_lock;
	struct kref kref;
};

static struct usb_driver ch34x_pis_driver;
static void skel_delete( struct kref *kref );


static DEFINE_MUTEX( io_mutex );

/*usb VID/PID Register Into System*/
static struct usb_device_id ch34x_usb_ids[] =
{
    { USB_DEVICE(CH34x_VENDOR_ID, CH34x_PRODUCT_ID) },                                 // CH34x
    { USB_DEVICE_INTERFACE_NUMBER(CH34x_VENDOR_ID, 0x55DB, MODE1_INTERFACE_NUM) },		// CH347 Mode1 SPI+IIC+UART
    { USB_DEVICE_INTERFACE_NUMBER(CH34x_VENDOR_ID, 0x55DD, MODE1_INTERFACE_NUM) },		// CH347 Mode3 JTAG+UART
//  { USB_DEVICE(0x1A86, 0x55DD) },	forse era cosi? o come sopra?
	{}
};

MODULE_DEVICE_TABLE(usb, ch34x_usb_ids);

static int ch34x_fops_release(struct inode *inode, struct file *file)
{
	struct ch34x_pis *dev;

	dev = (struct ch34x_pis *)file->private_data;
	if( dev == NULL )
		return -ENODEV;

	mutex_lock( &io_mutex );

	if( !--dev->open_count && dev->interface )
		usb_autopm_put_interface( dev->interface );
	mutex_unlock( &io_mutex );

	kref_put( &dev->kref, skel_delete );
	return 0;

}

//Control Endpoint Read
static int ch34x_func_read( __u8 request, __u16 value, __u16 index,
				struct ch34x_pis *dev, unsigned char *buf, __u16 len)
{
	int retval;
	/*Control Transform -->usb_control_msg */
	retval = usb_control_msg( dev->udev, usb_rcvctrlpipe( dev->udev, 0 ), 
		request, VENDOR_READ_TYPE, value, index, buf, len, 1000);

	dbg( "VENDOR_READ_TYPE: 0x%x : 0x%x : 0x%x %d - %d", request,
		value, index, retval, len );

	return retval;
}

//Control Endpoint Write
//In order to set chip register
static int ch34x_func_write( __u8 request, __u16 value, __u16 index,
			struct ch34x_pis *dev, unsigned char *buf,
			__u16 len )
{
	int retval;

	retval = usb_control_msg( dev->udev, 
			usb_sndctrlpipe(dev->udev, 0), request,
			VENDOR_WRITE_TYPE, value, index, buf, len, 1000);

	dbg( "VENDOR_READ_TYPE: 0x%x : 0x%x : 0x%x %d - %d", request,
		value, index, retval, len );

	return retval;
}


//Init Parallel Mode
//iMode-> 00/01 EPP
//iMode-> 02	MEM
static int CH34xInitParallel( unsigned char iMode, struct ch34x_pis *dev )
{
	int retval;
	__u8 RequestType = VENDOR_WRITE_TYPE;
	__u8 Request = CH34x_PARA_INIT;
	__u16 Value = ( iMode << 8 )|( iMode < 0x00000100 ? 0x02 : 0x00 );
	__u16 Index = 0;
	__u16 len = 0;
	retval = usb_control_msg( dev->udev, 
			usb_sndctrlpipe(dev->udev, 0), Request,
			RequestType, Value, Index, NULL, len, 1000);

	return retval;
}

//usb_fill_bulk_urb cpmplete callback
static void ch34x_write_bulk_callback( struct urb *urb )
{
	struct ch34x_pis *dev;

	dev = urb->context;

	if( urb->status )
	{
		if( !( urb->status == -ENOENT || urb->status == -ECONNRESET ||urb->status == -ESHUTDOWN))
			err("%s - nonzero write bulk status received: %d", __func__, urb->status );
		spin_lock( &dev->err_lock );
		dev->errors = urb->status;
		spin_unlock( &dev->err_lock );
	}
#if( LINUX_VERSION_CODE < KERNEL_VERSION( 2, 6, 35) )
	usb_buffer_free( urb->dev, urb->transfer_buffer_length,
			urb->transfer_buffer, urb->transfer_dma );
#else
	usb_free_coherent( urb->dev, urb->transfer_buffer_length,
			urb->transfer_buffer, urb->transfer_dma );
#endif
	up( &dev->limit_sem );
}

//EPP/MEM Read
ssize_t ch34x_fops_read(struct file *file, char __user *to_user,
	size_t count, loff_t *file_pos)
{
	struct ch34x_pis *dev;
	unsigned char mBuffer[4], *temp;
	int retval, i;
	// int j = 0;
int ttt; //daia
	unsigned long bytes_read, mNewlen;
	int Returnlen;
	unsigned long Bytes, totallen = 0;

	temp = kmalloc(2, GFP_KERNEL);
	if (!temp)
		return -ENOMEM;

	dev = (struct ch34x_pis *)file->private_data;
	if( count == 0 || count > MAX_BUFFER_LENGTH )
	{
		kfree(temp);
		return count;
	}
	// bytes_read = ( dev->VenIc >= 0x20 )?( CH34xA_EPP_IO_MAX - 
	// 	(CH34xA_EPP_IO_MAX & ( CH34x_PACKET_LENGTH - 1 ))): CH34x_EPP_IO_MAX;
	
	bytes_read = (count > CH347_PACKET_LENGTH) ? CH347_PACKET_LENGTH : count;

	// bytes_read = CH347_PACKET_LENGTH;

	mNewlen = count / bytes_read;
	temp[0] = mBuffer[0] = mBuffer[2] = Read_Mode;
	mBuffer[1] = ( unsigned char )bytes_read;
	mBuffer[3] = ( unsigned char )( count - mNewlen * bytes_read );
	dbg("count %d,->bytes_read %ld,->mNewlen %ld,->mBuffer[0] %d,->[1] %d,->[2]%d,->[3]%d",
		(int)count, bytes_read, mNewlen, mBuffer[0], mBuffer[1], mBuffer[2], mBuffer[3]);
		
	// if( mBuffer[3] )
	// 	mNewlen++;
	
	mutex_lock( &io_mutex );
	if( !dev->interface )
	{
		retval = -ENODEV;
		mutex_unlock( &io_mutex );
		goto exit;
	}
	mutex_unlock( &io_mutex );
	
	printk("mNewLen:%ld\n", mNewlen);
	for( i = 0; i < mNewlen; i++ )
	{
		// if( (i + 1) == mNewlen && mBuffer[3] )
		// {
		// 	temp[1] = mBuffer[3];
		// 	Bytes = mBuffer[3];
		// }
		// else
		// {
			temp[1] = mBuffer[1];
			Bytes = bytes_read;
		// }
		printk("Bytes:%ld\n", Bytes);

		dev->bulk_in_buffer = kmalloc( sizeof( unsigned char ) * Bytes, GFP_KERNEL );
		if( dev->bulk_in_buffer == NULL )
		{
			err("bulk_in_buffer malloc error");
			retval = -ENOMEM;
			goto exit;	
		}

		mutex_lock( &io_mutex );
		retval = usb_bulk_msg( dev->udev, usb_rcvbulkpipe( dev->udev,
				dev->bulk_in_endpointAddr ), 
				dev->bulk_in_buffer, 
				Bytes, &Returnlen, 1000 );
	
		if( retval )
		{
			retval = -EFAULT;
			mutex_unlock( &io_mutex );
			kfree( dev->bulk_in_buffer );
			err("usb_bulk_msg in error");
			goto exit;
		}	
		mutex_unlock( &io_mutex );
		ttt= copy_to_user( to_user + totallen, dev->bulk_in_buffer, Returnlen );
		totallen += Returnlen;
		
		printk("totallen:%ld, Returnlen:%d\n", totallen, Returnlen);
		kfree( dev->bulk_in_buffer );
	}
	kfree(temp);
	return totallen;
exit:
	kfree(temp);
	return retval;
}

//EPP/MEM Write
ssize_t ch34x_fops_write(struct file *file, const char __user *user_buffer,
	size_t count, loff_t *file_pos)
{
	struct ch34x_pis *dev;
	int retval = 0;
	char *WriteBuf = NULL;
	char *buf;
	int ttt; //daia
	struct urb *urb = NULL;
	unsigned int i;
	unsigned long mLength, mNewlen, mReturn = 0;	
	unsigned long write_size;

	dev = (struct ch34x_pis *)file->private_data;	
	if( count == 0 )
		goto exit;
	else if( count > MAX_BUFFER_LENGTH )
	{
		err("Data Overlimited,Must Under MAX_BUFFER_LENGTH");
		retval = -EFAULT;
		goto exit;
	}
	spin_lock_irq( &dev->err_lock );
	if(( retval = dev->errors ) < 0 )
	{
		dev->errors = 0;
		retval = ( retval == -EPIPE ) ? retval : -EIO;
	}
	spin_unlock_irq( &dev->err_lock );
	if( retval < 0 )
		goto exit;		
		
	mNewlen = count / CH34x_EPP_IO_MAX;
	mLength = count - mNewlen * CH34x_EPP_IO_MAX;
	// mNewlen *= CH34x_PACKET_LENGTH;
	mNewlen *= CH347_PACKET_LENGTH;
	buf = (char *)kmalloc( sizeof(unsigned char) * 4096, GFP_KERNEL );
	mutex_lock( &io_mutex );
	// for( i = 0; i < mNewlen; i += CH34x_PACKET_LENGTH )
	for( i = 0; i < mNewlen; i += CH347_PACKET_LENGTH )
	{
		ttt =copy_from_user( buf + i, user_buffer + mReturn, CH34x_EPP_IO_MAX );
		mReturn += CH34x_EPP_IO_MAX;
	}
	if( mLength )
	{
		ttt =copy_from_user( buf + i, user_buffer + mReturn, mLength );
		mNewlen += mLength;
	}
	mutex_unlock( &io_mutex );

	if( mNewlen > MAX_BUFFER_LENGTH )
	{
		
		urb = usb_alloc_urb( 0, GFP_KERNEL );
		if( !urb )
		{
			return -ENOMEM;
			goto error;
		}
		write_size = MAX_BUFFER_LENGTH;
#if( LINUX_VERSION_CODE < KERNEL_VERSION( 2, 6, 35) )
		WriteBuf = usb_buffer_alloc( dev->udev, write_size, 
			GFP_KERNEL, &urb->transfer_dma );
#else
		WriteBuf = usb_alloc_coherent( dev->udev, write_size,
			GFP_KERNEL, &urb->transfer_dma );
#endif

		if( !WriteBuf )
		{
			retval = -ENOMEM;
			goto error;
		}	
		
		memcpy(WriteBuf, buf, write_size);
		
		mutex_lock( &io_mutex );
		if( !dev->interface )
		{
			mutex_unlock( &io_mutex );
			retval = -ENOMEM;
			goto error;
		}		

		usb_fill_bulk_urb( urb, dev->udev, usb_sndbulkpipe( dev->udev,
			dev->bulk_out_endpointAddr ), WriteBuf, write_size,
				ch34x_write_bulk_callback, dev );
		urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
		
		usb_anchor_urb( urb, &dev->submitted );		
		
		retval = usb_submit_urb( urb, GFP_KERNEL );
		mutex_unlock( &io_mutex );	
		if( retval )
		{
			err("%s - failed submitting write urb,line:%d",
				__func__, __LINE__ );
			goto error_unanchor;
		}
		usb_free_urb( urb );
		mLength = mNewlen - MAX_BUFFER_LENGTH;	
	}
	else
		mLength = mNewlen;
	
	write_size = mLength;
	
	urb = usb_alloc_urb( 0, GFP_KERNEL );
	if( !urb )
	{
		return -ENOMEM;
		goto error;
	}
#if( LINUX_VERSION_CODE < KERNEL_VERSION( 2, 6, 35) )
	WriteBuf = usb_buffer_alloc( dev->udev, write_size, 
		GFP_KERNEL, &urb->transfer_dma );
#else
	WriteBuf = usb_alloc_coherent( dev->udev, write_size, 
		GFP_KERNEL, &urb->transfer_dma );
#endif
	if( !WriteBuf )
	{
		retval = -ENOMEM;
		goto error;
	}

	memcpy( WriteBuf, buf + (mNewlen - mLength), mLength );

	mutex_lock( &io_mutex );
	if( !dev->interface )
	{
		mutex_unlock( &io_mutex );
		retval = -ENOMEM;
		goto error;
	}
	/*initialize urb*/
	usb_fill_bulk_urb( urb, dev->udev, usb_sndbulkpipe( dev->udev,
			dev->bulk_out_endpointAddr ), WriteBuf, 
			write_size, ch34x_write_bulk_callback, dev );

	urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
	usb_anchor_urb( urb, &dev->submitted ); 
	retval = usb_submit_urb( urb, GFP_KERNEL );
	mutex_unlock( &io_mutex );
	if( retval )
	{
		err( "%s -failed to write submit urb,line:%d",
			__func__, __LINE__ );
		goto error_unanchor;
	}

	usb_free_urb( urb );	
	kfree( buf );
	return mNewlen;
error_unanchor:
	usb_unanchor_urb( urb );
error:
	if( urb )
	{
#if( LINUX_VERSION_CODE < KERNEL_VERSION( 2, 6, 35) )
		usb_buffer_free( dev->udev, write_size, WriteBuf,
				urb->transfer_dma );
#else
		usb_free_coherent( dev->udev, write_size, WriteBuf,
				urb->transfer_dma );
#endif
		usb_free_urb( urb );
	}
	kfree( buf );
exit:
	return retval;
	
}

//Write Data for I2C/Flash
static int ch34x_WriteData( unsigned long iLength, unsigned long iBuffer,
		struct ch34x_pis *dev)
{
	unsigned char *WriteBuf = NULL;
	unsigned long length;
	struct urb *urb;
	int retval = 0;
	
	length = iLength;

	if( length <= 0 )
	{
		goto exit;
	}
	if( down_interruptible( &dev->limit_sem ))
	{
		retval = -ERESTARTSYS;
		goto exit;
	}
	
	spin_lock_irq( &dev->err_lock );
	if((retval = dev->errors ) < 0)
	{
		dev->errors = 0;
		retval = ( retval == -EPIPE ) ? retval : -EIO;
	}
	spin_unlock_irq( &dev->err_lock );

	if( retval < 0 )	goto exit;
	urb = usb_alloc_urb( 0, GFP_KERNEL );
	if( !urb )
	{
		retval = -ENOMEM;
		goto error;
	}
#if( LINUX_VERSION_CODE < KERNEL_VERSION( 2, 6, 35) )
	WriteBuf = usb_buffer_alloc( dev->udev, length, 
		GFP_KERNEL, &urb->transfer_dma );
#else
	WriteBuf = usb_alloc_coherent( dev->udev, length, 
		GFP_KERNEL, &urb->transfer_dma );
#endif
	if( !WriteBuf )
	{
		retval = -ENOMEM;
		goto error;
	}

	if( copy_from_user( WriteBuf, (long __user *)iBuffer, length ))
	{
		retval = -EFAULT;
		goto error;
	}

	mutex_lock( &io_mutex );
	if( !dev->interface )
	{
		mutex_unlock( &io_mutex );
		retval = -ENODEV;
		goto error;
	}
	

	/* initialize the urb properly */
	usb_fill_bulk_urb( urb, dev->udev, usb_sndbulkpipe( dev->udev, 
			dev->bulk_out_endpointAddr ), WriteBuf, length,
			ch34x_write_bulk_callback, dev );

	urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
	usb_anchor_urb( urb, &dev->submitted );
	/* send the data out the bulk port */
	retval = usb_submit_urb( urb, GFP_KERNEL );	
	mutex_unlock( &io_mutex );
	if( retval )
	{
		err("%s - failed to submit writing urb,error %d,line %d",
			__func__, retval, __LINE__ );
		goto error_unanchor;
	}
	/*release our reference to this urb*/
	usb_free_urb( urb );
	return length;

error_unanchor:
	usb_unanchor_urb( urb );
error:
	if( urb )
	{
#if( LINUX_VERSION_CODE < KERNEL_VERSION( 2, 6, 35) )
		usb_buffer_free( dev->udev, length, WriteBuf,
				urb->transfer_dma );
#else
		usb_free_coherent( dev->udev, length, WriteBuf,
				urb->transfer_dma );
#endif
		usb_free_urb( urb );
	}
	up( &dev->limit_sem );
exit:
	return retval;
}


static int ch34x_data_write_read( unsigned long iLength, unsigned long iBuffer,
				unsigned long oBuffer, struct ch34x_pis *dev )
{
	unsigned long length;	// the length of iBuffer
	unsigned long totallen;
	int bytes_read;
	unsigned char *iBuf;	// Input data
	unsigned char *oBuf;	// Output data
	struct urb *iUrb;
	int i, mSave;		// the number of data in a block
	int readtimes;
	int retval = 0;

	dbg("iLength is %ld\n",iLength);
	if( iLength < 8 || iLength > MAX_BUFFER_LENGTH + 8 )
	{
		err(" The length input error");
		retval = -EFAULT;
		goto exit;
	}
	iBuf = kmalloc( sizeof(unsigned char) * iLength, GFP_KERNEL );
	retval = copy_from_user(iBuf, (char __user*)iBuffer, iLength);
	if( retval != 0 )
	{
		err("copy error");
		kfree( iBuf );
		goto exit;	
	}
 	iLength -= 8;
	length = iLength;
	mSave = iBuf[iLength];
	readtimes = iBuf[iLength + 4];
	kfree( iBuf );
	dbg("the number of a block %d,should read %d", mSave, readtimes);
	if( i * readtimes > MAX_BUFFER_LENGTH || mSave == 0 || readtimes == 0 )
	{
		return -EFAULT;
		goto exit;
	}
	
	if( down_interruptible( &dev->limit_sem ))
	{
		return -ERESTARTSYS;
		goto exit;
	}
	spin_lock_irq( &dev->err_lock );
	if((retval = dev->errors) < 0)
	{
		dev->errors = 0;
		retval = (retval == -EPIPE) ? retval : -EIO;
	}
	spin_unlock_irq( &dev->err_lock );
	if( retval < 0 )	
		goto exit;

	iUrb = usb_alloc_urb( 0, GFP_KERNEL );
	if( !iUrb )
	{
		retval = -ENOMEM;
		goto error;
	}
#if( LINUX_VERSION_CODE < KERNEL_VERSION( 2, 6, 35) )
	iBuf = usb_buffer_alloc( dev->udev, length, 
		GFP_KERNEL, &iUrb->transfer_dma );
#else
	iBuf = usb_alloc_coherent( dev->udev, length, 
		GFP_KERNEL, &iUrb->transfer_dma );
#endif
	if( !iBuf )
	{
		retval = -ENOMEM;
		goto error;
	}

	if( copy_from_user( iBuf, (char __user *)iBuffer, length ))
	{
		retval = -EFAULT;
		goto exit;	
	}
	mutex_lock( &io_mutex );
	if( !dev->interface )
	{
		mutex_unlock( &io_mutex );
		retval = -ENODEV;
		goto error;
	}	
	usb_fill_bulk_urb( iUrb, dev->udev, usb_sndbulkpipe( dev->udev,
			dev->bulk_out_endpointAddr ), iBuf, length,
			ch34x_write_bulk_callback, dev );
	iUrb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;	
	usb_anchor_urb( iUrb, &dev->submitted );
	retval = usb_submit_urb( iUrb, GFP_KERNEL );
	mutex_unlock( &io_mutex );
	if( retval )
	{
		err("%s-failed submitting write urb in write_read", __func__);
		goto error_unanchor;
	}
	usb_free_urb( iUrb );
// Read Urb Data
	totallen = mSave * readtimes;
	dbg("mSave : %d, readtimes : %d, totallen : %ld\n", mSave, readtimes, totallen);
	oBuf = kmalloc( sizeof( unsigned char ) * totallen, GFP_KERNEL );
	totallen = 0;
	for( i = 0; i < readtimes; i++ )
	{
		mutex_lock( &io_mutex );
		retval = usb_bulk_msg( dev->udev,
				usb_rcvbulkpipe( dev->udev, dev->bulk_in_endpointAddr),
				oBuf + i * CH34x_EPP_IO_MAX, CH34x_EPP_IO_MAX, &bytes_read, 10000);

		totallen += bytes_read;
		mutex_unlock( &io_mutex );
	}
	dbg("The actual length of Read is %ld", totallen);
	/*if the read is sucessful,copy the data to userspace*/
	if( copy_to_user((char __user *)oBuffer, oBuf, totallen))
	{
		retval = -ENOMEM;
		kfree( oBuf );
		goto exit;
	}
	
	kfree( oBuf );
	return totallen;
error_unanchor:
	if( iUrb )
		usb_unanchor_urb( iUrb );
error:
	if( iUrb )
	{
#if( LINUX_VERSION_CODE < KERNEL_VERSION( 2, 6, 35) )
		usb_buffer_free( dev->udev, length, iBuf,
				iUrb->transfer_dma );
#else
		usb_free_coherent( dev->udev, length, iBuf,
				iUrb->transfer_dma );
#endif
		usb_free_urb( iUrb );
	}
	up( &dev->limit_sem );
exit:
	return retval;

}

static void skel_delete( struct kref *kref )
{
	struct ch34x_pis *dev = container_of( kref, struct ch34x_pis, kref );
	usb_put_dev( dev->udev );

	kfree( dev );
}

int ch34x_fops_open(struct inode *inode, struct file *file)
{
	struct ch34x_pis *ch34x_p;
	struct usb_interface *interface;
	int retval = 0;
	unsigned int subminor;

#if( LINUX_VERSION_CODE < KERNEL_VERSION( 2, 6, 35) )
	subminor = iminor( inode );
#else
	subminor = iminor( file->f_path.dentry->d_inode );
#endif

	interface = usb_find_interface( &ch34x_pis_driver, subminor );
	if ( !interface )
	{
		err( "%s-error,cannot find device for minor :%d",
			__func__, subminor);
		retval = -ENODEV;
		goto exit;
	} 
	
	ch34x_p = usb_get_intfdata( interface );
	if( !ch34x_p )
	{
		err("Get interface data error");
		retval = -ENODEV;
		goto exit;
	}

	/* add the usage for device*/
	kref_get( &ch34x_p->kref );

	mutex_lock( &io_mutex );
	if( !ch34x_p->open_count++ )
	{
		retval = usb_autopm_get_interface( interface );
		if( retval )
		{
			ch34x_p->open_count--;
			mutex_unlock( &io_mutex );
			kref_put( &ch34x_p->kref, skel_delete );
			goto exit;
		} 
	}
	file->private_data = ch34x_p;
	mutex_unlock( &io_mutex );

exit:
	return retval;
}

#if( LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35) )
int ch34x_fops_ioctl( struct inode *inode, struct file *file,
	unsigned int ch34x_cmd, unsigned long ch34x_arg )
#else
long ch34x_fops_ioctl( struct file *file, unsigned int ch34x_cmd,
	unsigned long ch34x_arg )
#endif
{
	int retval = 0;
	char *buf;
	unsigned long bytes_read;
	unsigned long bytes_write;
	char *drv_version_tmp = DRV_VERSION;
	struct ch34x_pis *ch34x_pis_tmp;
	unsigned long arg1, arg2, arg3, arg4;

	buf = kmalloc(2, GFP_KERNEL);
	if (!buf) {
		err("No memory!");
		return -EFAULT;
	}

	ch34x_pis_tmp = ( struct ch34x_pis *)file->private_data;
	if( ch34x_pis_tmp == NULL )
	{
		return -ENODEV;
	}
	switch( ch34x_cmd )
	{
		case CH34x_GET_DRV_VERSION:
			{
				retval = copy_to_user( (char __user *)( ch34x_arg ),
					( char * )drv_version_tmp, strlen( DRV_VERSION ));
				dbg("CH34x_GET_DRV_VERSION Successed");
				break;
			}
		case CH34x_CHIP_VERSION:
			{
				retval = ch34x_func_read( VENDOR_VERSION, 
					0x0000, 0x0000, ch34x_pis_tmp, buf, 0x02 );			
				
				retval = copy_to_user( (char __user *)( ch34x_arg ),
					(char *)buf, 0x02 );	
				ch34x_pis_tmp->VenIc = *(buf + 1) << 8 | *buf;
				dbg("------> 2 Chip Version is sucessful 0x%02x%x", *(buf + 1), *buf);
				break;
			}
		case CH34x_FUNCTION_SETPARA_MODE:
			{
				retval = ch34x_func_write(CH34x_DEBUG_WRITE, 0x2525, 
					(unsigned short)( ch34x_arg << 8 | ch34x_arg ), 
					ch34x_pis_tmp, NULL, 0x00 );
				if( retval != 0 )
					err("CH34x_FUNCTION_SETPARA_MODE Error");
				dbg("------>SetParaMode - ch34x_arg 0x%lx", ch34x_arg);
				break;
			}
		case CH34x_FUNCTION_READ_MODE:
			{
				get_user( arg1, (long __user *)ch34x_arg);
				if( arg1 )
					Read_Mode = CH34x_PARA_CMD_R1;
				else
					Read_Mode = CH34x_PARA_CMD_R0;

				dbg( "---->Read_Mode : 0x%x", Read_Mode );
				break;
			}
		case CH34x_FUNCTION_WRITE_MODE:
			{
				get_user( arg1, (long __user *)ch34x_arg);
				if( arg1 )
					Write_Mode = CH34x_PARA_CMD_W1;
				else
					Write_Mode = CH34x_PARA_CMD_W0;

				dbg( "Write_Mode : 0x%x", Write_Mode );
				break;
			}
		case CH34x_I2C_READ_MODE:
			{
				break;	
			}
		case CH34x_I2C_WRITE_MODE:
			{
				break;
			}
		case CH34x_PIPE_DATA_DOWN:	
			{
				dbg("------> Use Pipe Data Down");
				get_user( arg1, (long __user *)ch34x_arg);
				get_user( arg2, ((long __user *)ch34x_arg + 1));
				get_user( bytes_write, (long __user *)arg1);
				dbg("------> length :%ld", bytes_write);
				retval = ch34x_WriteData( bytes_write,        
							arg2,
							ch34x_pis_tmp );

				break;
			}
		case CH34x_PIPE_WRITE_READ:
			{	
				dbg("------> Use Pipe Date Write/Read");

				get_user( arg1, (long __user *)ch34x_arg);
				get_user( arg2, ((long __user *)ch34x_arg + 1));
				get_user( arg3, ((long __user *)ch34x_arg + 2));
				get_user( arg4, ((long __user *)ch34x_arg + 3));

				dbg("Input number is %ld\n", arg1);	
				bytes_read = ch34x_data_write_read( arg1,
						arg2, arg3, ch34x_pis_tmp );
				if( bytes_read <= 0 )
				{
					err("Read Error");
					return -EFAULT;
				}
				dbg("Read bytes is %ld", bytes_read);	
				retval = put_user( bytes_read, (long __user *)arg4);
					
				break;
			}
		case CH34x_PIPE_DEVICE_CTRL:
			{
				get_user(arg1, (long __user *)ch34x_arg);
				retval = CH34xInitParallel((u8)arg1, ch34x_pis_tmp);
				if( retval < 0 )
				{
					err("Init Parallel Error");
					return -EFAULT;
				}
				break;
			}
		default:
			return -ENOTTY;
			break;
	}	

	kfree(buf);
	return 0;
}

static const struct file_operations ch34x_fops_driver = {
	.owner		= THIS_MODULE,
	.open		= ch34x_fops_open,
	.release	= ch34x_fops_release,
	.read		= ch34x_fops_read,
	.write		= ch34x_fops_write,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
	.ioctl			= ch34x_fops_ioctl,
#else
	.unlocked_ioctl = ch34x_fops_ioctl,
#endif
};

/*
 *usb class driver info in order to get a minor number from the usb core
 *and to have the device registered with the driver core
 */
static struct usb_class_driver ch34x_class = {
	.name =		"ch34x_pis%d",
	.fops =		&ch34x_fops_driver,
	.minor_base =	CH34x_MINOR_BASE,
};

static int ch34x_pis_probe( struct usb_interface *intf, const 
	struct usb_device_id *id )
{
	struct usb_host_interface *hinterface;
	struct usb_endpoint_descriptor *endpoint;
	struct ch34x_pis *ch34x_p;
	
	size_t buffer_size;
	int retval = -ENOMEM;
	int i;
	
	/* allocate memory for our device state and initialize it */
	ch34x_p = kzalloc( sizeof( *ch34x_p ), GFP_KERNEL );
	if( !ch34x_p )
	{
		err("Out of Memory");
		goto error;
	}
	
	/* init */
	kref_init( &ch34x_p->kref );
	sema_init( &ch34x_p->limit_sem, WRITES_IN_FLIGHT );
	spin_lock_init( &ch34x_p->err_lock );
	init_usb_anchor( &ch34x_p->submitted);	

	ch34x_p->udev = usb_get_dev( interface_to_usbdev( intf ));
	ch34x_p->interface = intf;
 
	hinterface = intf->cur_altsetting;
	
	if( hinterface->desc.bNumEndpoints < 1)
		return -ENODEV;
	/* Get Endpoint*/
	for( i = 0; i < hinterface->desc.bNumEndpoints; ++i )
	{
		endpoint = &hinterface->endpoint[i].desc;
		
		if(( endpoint->bEndpointAddress & USB_DIR_IN ) &&
			 ( endpoint->bmAttributes & 2 ) == 0x02 ) 
		{
			dbg("Found a bulk in endpoint");
			buffer_size = le16_to_cpu( endpoint->wMaxPacketSize );
			ch34x_p->bulk_in_size = buffer_size;
			ch34x_p->bulk_in_endpointAddr = endpoint->bEndpointAddress;
		}
		
		if((( endpoint->bEndpointAddress & USB_DIR_IN ) == 0x00 ) &&
		  ( endpoint->bmAttributes & 2 ) == 0x02 )
		{
			dbg("Found a bulk out endpoint");
			ch34x_p->bulk_out_endpointAddr = endpoint->bEndpointAddress;
		}
		
		if(( endpoint->bEndpointAddress & USB_DIR_IN ) &&
		  ( endpoint->bmAttributes & 3 ) == 0x03 )
		{
			dbg("Found a interrupt in endpoint");
			ch34x_p->interrupt_in_endpoint = endpoint;
		}
	}
	
	/* save our data point in this interface device */
	usb_set_intfdata( intf, ch34x_p );
	
	retval = usb_register_dev( intf, &ch34x_class );
	if( retval )
	{
		err( "usb_get_dev error,disable to use this device" );
		usb_set_intfdata( intf, NULL );
		goto error;
	}
	
	dbg( "Ch34x_pis device now attached to ch34x_pis-%d", intf->minor );
	
	return 0;

error:
	if( ch34x_p )
		kref_put( &ch34x_p->kref, skel_delete);

	return retval;
}

static int ch34x_pis_suspend(struct usb_interface *intf, pm_message_t message )
{
	struct ch34x_pis *dev = usb_get_intfdata( intf );
	int time;

	if( !dev )
		return 0;
	
	time = usb_wait_anchor_empty_timeout( &dev->submitted, 1000 );
	if( !time )
		usb_kill_anchored_urbs( &dev->submitted );
	
	return 0;
}

static int ch34x_pis_resume( struct usb_interface *intf )
{
	return 0;
}

static void ch34x_pis_disconnect( struct usb_interface *intf )
{
	struct ch34x_pis *dev;
	int minor = intf->minor;
	
	dev = usb_get_intfdata( intf );
	usb_set_intfdata( intf, NULL );

	/* give back our minor */
	usb_deregister_dev( intf, &ch34x_class );
		
	mutex_lock( &io_mutex );
	dev->interface = NULL;	
	mutex_unlock( &io_mutex );

	usb_kill_anchored_urbs( &dev->submitted );
	/*decrement our usage count*/
	kref_put( &dev->kref, skel_delete );

	dbg("CH34x_pis-%d now disconnected", minor );	
}

static int ch34x_pre_reset( struct usb_interface *intf )
{
	struct ch34x_pis *dev = usb_get_intfdata( intf );
	int time;
	
	mutex_lock( &io_mutex );
	time = usb_wait_anchor_empty_timeout( &dev->submitted, 1000 );
	if( !time )
		usb_kill_anchored_urbs( &dev->submitted );
	
	return 0;	
}

static int ch34x_post_reset( struct usb_interface *intf )
{
	struct ch34x_pis *dev = usb_get_intfdata( intf );
	
	dev->errors = -EPIPE;
	mutex_unlock( &io_mutex );

	return 0;
}

//usb driver Interface
static struct usb_driver ch34x_pis_driver = {
	.name  		= DRV_NAME,
	.probe		= ch34x_pis_probe,
	.disconnect	= ch34x_pis_disconnect,
	.suspend	= ch34x_pis_suspend,
	.resume		= ch34x_pis_resume,
	.pre_reset	= ch34x_pre_reset,
	.post_reset	= ch34x_post_reset,
	.id_table	       = ch34x_usb_ids,
	.supports_autosuspend = 1,
};

static int __init ch34x_pis_init(void)
{
	int retval;

	printk(KERN_INFO KBUILD_MODNAME ": " DRIVER_DESC "\n");
	printk(KERN_INFO KBUILD_MODNAME ": " VERSION_DESC "\n");
	retval = usb_register( &ch34x_pis_driver );
	if( retval )
		printk( KERN_INFO "CH34x Device Register Failed.\n" );
	return retval;
}

static void __exit ch34x_pis_exit(void)
{
	printk(KERN_INFO KBUILD_MODNAME ": " "ch34x driver exit.\n");
	usb_deregister(&ch34x_pis_driver);
}

module_init(ch34x_pis_init);
module_exit(ch34x_pis_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
