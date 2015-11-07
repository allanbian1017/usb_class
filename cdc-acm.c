/***************************************************************************
 *   Copyright (C) 2015 by Tse-Lun Bien                                    *
 *   allanbian@gmail.com                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libusb.h>

#define CDC_ACM_DEV_VID	      		0x067b
#define CDC_ACM_DEV_PID	      		0x2303
#define CDC_ACM_DEV_IFACE	  	0x0

#define CDC_ACM_DEV_NOTIFYIN  		0x81
#define CDC_ACM_DEV_EPOUT	  	0x02
#define CDC_ACM_DEV_EPIN      		0x83
#define CDC_ACM_DEV_NOTIFYIN_LEN	10
#define CDC_ACM_DEV_EPIN_LEN		64

#define GET_LINE_REQUEST_TYPE           0xa1
#define GET_LINE_REQUEST                0x21

#define SET_LINE_REQUEST_TYPE           0x21
#define SET_LINE_REQUEST                0x20

#define SERIAL_STATE_REQUEST_TYPE	0xa1
#define SERIAL_STATE_REQUEST		0x20

#define UART_DCD                        0x01
#define UART_DSR                        0x02
#define UART_BREAK_ERROR                0x04
#define UART_RING                       0x08
#define UART_FRAME_ERROR                0x10
#define UART_PARITY_ERROR               0x20
#define UART_OVERRUN_ERROR              0x40
#define UART_CTS                        0x80

#define UART_STOP_BIT_1			0
#define UART_STOP_BIT_1_5		1
#define UART_STOP_BIT_2			2

#define UART_PARITY_NONE		0
#define UART_PARITY_ODD			1
#define UART_PARITY_EVEN		2
#define UART_PARITY_MARK		3
#define UART_PARITY_SPACE		4

struct line_code {
	unsigned char baudrate[4];
	unsigned char stop;
	unsigned char parity;
	unsigned char data;
};

int acm_write(libusb_device_handle *handle, unsigned char *data, int len)
{
	int act_len;
	int ret;

	ret = libusb_bulk_transfer(handle, CDC_ACM_DEV_EPOUT, data, len, &act_len, 0);
	if (ret != LIBUSB_SUCCESS)
		return -1;

	return act_len;
}

int acm_read(libusb_device_handle *handle, unsigned char *data, int len)
{
	unsigned char buf[CDC_ACM_DEV_EPIN_LEN];
	int act_len;
	int ret;

	ret = libusb_bulk_transfer(handle, CDC_ACM_DEV_EPIN, buf, CDC_ACM_DEV_EPIN_LEN, &act_len, 0);
	if (ret != LIBUSB_SUCCESS)
		return -1;

	if (len < act_len) {
		memcpy(data, buf, len);
	} else {
		memcpy(data, buf, act_len);
	}

	return act_len;
}

int acm_get_line_code(libusb_device_handle *handle, struct line_code *setting)
{
	int ret;

	ret = libusb_control_transfer(handle, GET_LINE_REQUEST_TYPE, 
			GET_LINE_REQUEST, 0, 0, (void *)setting, sizeof(struct line_code), 0);
	if (ret < 0)
		return -1;

	return 0;
}

int acm_set_line_code(libusb_device_handle *handle, struct line_code *setting)
{
	int ret;

	ret = libusb_control_transfer(handle, SET_LINE_REQUEST_TYPE, 
			SET_LINE_REQUEST, 0, 0, (void *)setting, sizeof(struct line_code), 0);
	if (ret < 0)
		return -1;

	return 0;
}

int acm_notify_serial_state(unsigned char *data, int len)
{
	if (data[8] & UART_DCD)
		printf("DCD\n");

	if (data[8] & UART_DSR)
		printf("DSR\n");

	if (data[8] & UART_BREAK_ERROR)
		printf("BREAK ERR\n");

	if (data[8] & UART_RING)
		printf("RING\n");

	if (data[8] & UART_FRAME_ERROR)
		printf("FRAME ERR\n");

	if (data[8] & UART_PARITY_ERROR)
		printf("PARITY ERR\n");

	if (data[8] & UART_OVERRUN_ERROR)
		printf("OVERRUN ERR\n");

	if (data[8] & UART_CTS)
		printf("CTS\n");

	return 0;
}

void acm_notify(struct libusb_transfer *xfer)
{
	int *completed = xfer->user_data;
	int act_len = xfer->actual_length;
	unsigned char *buf = xfer->buffer;
	int ret;

	*completed = 1;

	if (act_len >= 5) {
		printf("Notify!\n");
		if (buf[0] == SERIAL_STATE_REQUEST_TYPE &&
				buf[1] == SERIAL_STATE_REQUEST) {
			ret = acm_notify_serial_state(buf, act_len);
		}
	}

	ret = libusb_submit_transfer(xfer);
	if (ret != LIBUSB_SUCCESS)
		printf("Submit error!\n");
}

int main(int argc, char *argv[])
{
	libusb_device_handle *handle = NULL;
	struct libusb_transfer *xfer = NULL;
	struct line_code setting;
	struct timeval tv;
	unsigned char buf[32];
	int completed = 0;
	int br;
	int ret;

	ret = libusb_init(NULL);
	if (ret != LIBUSB_SUCCESS)
		return 1;

	handle = libusb_open_device_with_vid_pid(NULL, CDC_ACM_DEV_VID, CDC_ACM_DEV_PID);
	if (handle == NULL)
		goto deinit;

	ret = libusb_kernel_driver_active(handle, CDC_ACM_DEV_IFACE);
	if (ret == 1) {
		ret = libusb_detach_kernel_driver(handle, CDC_ACM_DEV_IFACE);
		if (ret != LIBUSB_SUCCESS)
			goto close;
	}

	ret = libusb_claim_interface(handle, CDC_ACM_DEV_IFACE);
	if (ret != LIBUSB_SUCCESS)
		goto attach;

	br = 115200;

	setting.baudrate[0] = br & 0xff;
	setting.baudrate[1] = (br >> 8) & 0xff;
	setting.baudrate[2] = (br >> 16) & 0xff;
	setting.baudrate[3] = (br >> 24) & 0xff;

	setting.stop = UART_STOP_BIT_1;
	setting.parity = UART_PARITY_NONE;
	setting.data = 8;

	ret = acm_set_line_code(handle, &setting);
	if (ret != 0)
		goto release;

	ret = acm_get_line_code(handle, &setting);
	if (ret != 0)
		goto release;

	br = setting.baudrate[3];
	br = (br << 8) | (setting.baudrate[2] & 0xff);
	br = (br << 8) | (setting.baudrate[1] & 0xff);
	br = (br << 8) | (setting.baudrate[0] & 0xff);

	printf("Baudrate: %d\n", br);
	printf("Stopbits: %d\n", setting.stop);
	printf("Parity:   %d\n", setting.parity);
	printf("Databits: %d\n", setting.data);

	buf[0] = 'h';
	buf[1] = 'e';
	buf[2] = 'l';
	buf[3] = 'l';
	buf[4] = 'o';

	ret = acm_write(handle, buf, 5);
	if (ret != 5)
		goto release;

	xfer = libusb_alloc_transfer(0);
	if (xfer == NULL)
		goto release;

	libusb_fill_interrupt_transfer(xfer, handle, CDC_ACM_DEV_NOTIFYIN, 
			buf, CDC_ACM_DEV_NOTIFYIN_LEN, acm_notify, &completed, 0);

	ret = libusb_submit_transfer(xfer);
	if (ret != LIBUSB_SUCCESS)
		goto dealloc;


	tv.tv_sec = 0;
	tv.tv_usec = 0;
	while (1) {
		ret = acm_read(handle, buf, 1);
		if (ret < 0)
			goto dealloc;

		if (ret > 0) 
			printf("len=%d  %x\n", ret, buf[0]);

		ret = libusb_handle_events_timeout(NULL, &tv);
		if (ret != LIBUSB_SUCCESS)
			goto dealloc;
	}

dealloc:
	libusb_free_transfer(xfer);
release:
	ret = libusb_release_interface(handle, CDC_ACM_DEV_IFACE);
attach:
	ret = libusb_attach_kernel_driver(handle, CDC_ACM_DEV_IFACE);
close:
	libusb_close(handle);
deinit:
	libusb_exit(NULL);
	return 0;
}
