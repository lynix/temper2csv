/* Copyright 2014 Alexander Koch <lynix47@gmail.com>
 *
 * This file is part of 'temper2csv'.
 *
 * 'temper2csv' is distributed under the MIT License, see LICENSE file.
 */

#include <libusb-1.0/libusb.h>

#include <fcntl.h>
#include <sys/types.h>
#include <string.h>

#define USB_VID   0x0c45
#define USB_PID   0x7401
#define USB_EPA   0x82
#define USB_IFN1  0x00
#define USB_IFN2  0x01
#define USB_CNF   0x01
#define USB_LEN   8
#define USB_TMO   5000

static const unsigned char data_query[] = { 0x01, 0x80, 0x33, 0x01, 0x00, 0x00,
		0x00, 0x00 };
static const unsigned char data_init[] = { 0x01, 0x01 };

libusb_device_handle *temper = NULL;

libusb_device_handle *temper_open(char **err)
{
	libusb_init(NULL);

	libusb_device **dev_list;
	ssize_t num_devs;
	if ((num_devs = libusb_get_device_list(NULL, &dev_list)) < 0) {
		*err = "failed to fetch USB device list";
		return NULL;
	}

	libusb_device *dev = NULL;
	char found = 0;
	for (ssize_t i=0; i<num_devs; i++) {
		dev = dev_list[i];
		struct libusb_device_descriptor desc;
		if (libusb_get_device_descriptor(dev, &desc) < 0) {
			*err = "failed to read USB device descriptor";
			break;
		}
		if (desc.idVendor == USB_VID && desc.idProduct == USB_PID) {
			libusb_ref_device(dev);
			found = 1;
			break;
		}
	}
	libusb_free_device_list(dev_list, 1);

	if (!found) {
		*err = "no supported device found (or insufficient access rights)";
		libusb_exit(NULL);
		return NULL;
	}

	libusb_device_handle *handle;
	if (libusb_open(dev, &handle) != 0) {
		*err = "failed to open USB device";
		libusb_unref_device(dev);
		libusb_exit(NULL);
		return NULL;
	}

	if (libusb_kernel_driver_active(handle, USB_IFN1)) {
		int r = libusb_detach_kernel_driver(handle, USB_IFN1);
		if (r != 0 && r != LIBUSB_ERROR_NOT_FOUND) {
			*err = "failed to detach kernel driver for interface 1";
			libusb_close(handle);
			libusb_unref_device(dev);
			libusb_exit(NULL);
			return NULL;
		}
	}
	if (libusb_kernel_driver_active(handle, USB_IFN2)) {
		int r = libusb_detach_kernel_driver(handle, USB_IFN2);
		if (r != 0 && r != LIBUSB_ERROR_NOT_FOUND) {
			*err = "failed to detach USB kernel driver for interface 2";
			libusb_close(handle);
			libusb_unref_device(dev);
			libusb_exit(NULL);
			return NULL;
		}
	}

	if (libusb_set_configuration(handle, USB_CNF) != 0) {
		*err = "failed to set USB device configuration";
		libusb_close(handle);
		libusb_unref_device(dev);
		libusb_exit(NULL);
		return NULL;
	}

	return handle;
}


int temper_control_transfer(uint16_t value, uint16_t index, unsigned char *data,
		uint16_t length, char **err)
{
	if (libusb_claim_interface(temper, USB_IFN1) != 0) {
		*err = "failed to claim interface";
		libusb_close(temper);
		libusb_exit(NULL);
		return -1;
	}

	int ret;
	if ((ret = libusb_control_transfer(temper, 0x21, 0x09, value, index, data, length,
			USB_TMO)) != length) {
		*err = " USB control transfer failed";
		libusb_release_interface(temper, USB_IFN1);
		libusb_close(temper);
		libusb_exit(NULL);
		return -1;
	}
	libusb_release_interface(temper, USB_IFN1);

	return 0;
}


int temper_interrupt_transfer(unsigned char ep, unsigned char *data, int length,
		char **err)
{
	if (libusb_claim_interface(temper, USB_IFN1) != 0) {
		*err = "failed to claim interface";
		libusb_close(temper);
		libusb_exit(NULL);
		return -1;
	}

	int trans;
	if (libusb_interrupt_transfer(temper, ep, data, length, &trans, USB_TMO)
			!= 0) {
		*err = "USB interrupt transfer failed";
		libusb_release_interface(temper, USB_IFN1);
		libusb_close(temper);
		libusb_exit(NULL);
		return -1;
	}

	libusb_release_interface(temper, USB_IFN1);

	return 0;
}


double temper_read(char **err)
{
	if (temper == NULL) {
		temper = temper_open(err);
		if (temper == NULL)
			return -1;

		// initial control query
		if (temper_control_transfer(0x0201, 0x00, (unsigned char *)data_init, 2,
				err) != 0)
			return -1;
	}

	// query
	if (temper_control_transfer(0x0200, 0x01, (unsigned char *)data_query,
			USB_LEN, err) != 0)
		return -1;

	//response
	unsigned char data[USB_LEN];
	bzero(data, USB_LEN);
	if (temper_interrupt_transfer(USB_EPA, data, USB_LEN, err) != 0)
		return -1;

	double temp = (data[3] & 0xFF) + (data[2] << 8);
	temp *= 125.0 / 32000.0;

	return temp;
}
