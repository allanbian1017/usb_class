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
#include <libusb.h>

const char *print_speed(int speed)
{
        switch (speed) {
        case LIBUSB_SPEED_LOW:
                return "Low-Speed";
        case LIBUSB_SPEED_FULL:
                return "Full-Speed";
        case LIBUSB_SPEED_HIGH:
                return "High-Speed";
        case LIBUSB_SPEED_SUPER:
                return "Super-Speed";
        default:
                break;
        }

        return "Unknown-Speed";
}


const char *print_class(unsigned char class)
{
	switch (class) {
	case 0x01:
		return "Audio";
	case 0x02:
		return "CDC";
	case 0x03:
		return "HID";
	case 0x05:
		return "PID";
	case 0x06:
		return "Image";
	case 0x07:
		return "Printer";
	case 0x08:
		return "MSC";
	case 0x09:
		return "Hub";
	case 0x0a:
		return "CDC-Data";
	case 0x0b:
		return "CCID";
	case 0x0d:
		return "Security";
	case 0x0e:
		return "UVC";
	case 0x0f:
		return "PHDC";
	case 0x10:
		return "A/V";
	case 0x11:
		return "Billboard";
	case 0xdc:
		return "Diagnostic";
	case 0xe0:
		return "WSC";
	case 0xef:
		return "MISC";
	case 0xfe:
		return "APP-Specific";
	case 0xff:
		return "Vendor";
	default:
		break;
	}

	return "Unspecific";
}

const char *print_power(char type)
{
	if (type & 0x80) return "Bus Powered";
	if (type & 0x40) return "Self Powered";
	
	return "Unknown";
}

const char *print_epdir(char addr)
{
	return (addr & 0x80) ? "in" : "out";
}

const char *print_eptype(char type)
{
	switch (type & 0x03) {
	case 0:
		return "ctrl";
	case 1:
		return "iso";
	case 2:
		return "bulk";
	case 3:
		return "int";
	default:
		break;
	}

	return "unknown";
}

void print_endpoint(const struct libusb_endpoint_descriptor *endpoint)
{
	printf("      bEndpointAddress: %02xh(%s)\n", endpoint->bEndpointAddress,
					print_epdir(endpoint->bEndpointAddress));
	printf("      bmAttributes:     %02xh(%s)\n", endpoint->bmAttributes, 
					print_eptype(endpoint->bmAttributes));
	printf("      wMaxPacketSize:   %d\n", endpoint->wMaxPacketSize);
	printf("      bInterval:        %d\n", endpoint->bInterval);
	printf("\n");
}

void print_altsetting(const struct libusb_interface_descriptor *interface)
{
	int i;

	printf("    bInterfaceNumber:   %d\n", interface->bInterfaceNumber);
	printf("    bAlternateSetting:  %02xh\n", interface->bAlternateSetting);
	printf("    bNumEndpoints:      %d\n", interface->bNumEndpoints);
	printf("    bInterfaceClass:    %02xh(%s)\n", interface->bInterfaceClass, 
					print_class(interface->bInterfaceClass));
	printf("    bInterfaceSubClass: %02xh\n", interface->bInterfaceSubClass);
	printf("    bInterfaceProtocol: %02xh\n", interface->bInterfaceProtocol);
	printf("    iInterface:         %d\n", interface->iInterface);

	for (i = 0; i < interface->bNumEndpoints; i++)
		print_endpoint(&interface->endpoint[i]);
}

void print_interface(const struct libusb_interface *interface)
{
	int i;

	for (i = 0; i < interface->num_altsetting; i++)
		print_altsetting(&interface->altsetting[i]);
}

void print_configuration(struct libusb_config_descriptor *config)
{
	int i;

	printf("  wTotalLength:         %d\n", config->wTotalLength);
	printf("  bNumInterfaces:       %d\n", config->bNumInterfaces);
	printf("  bConfigurationValue:  %02xh\n", config->bConfigurationValue);
	printf("  iConfiguration:       %02xh\n", config->iConfiguration);
	printf("  bmAttributes:         %02xh(%s)\n", config->bmAttributes,
					print_power(config->bmAttributes));
	printf("  bMaxPower:            %03d(%d mA)\n", config->MaxPower, 
					config->MaxPower*2);

	for (i = 0; i < config->bNumInterfaces; i++)
		print_interface(&config->interface[i]);
}

void print_dev(libusb_device *dev)
{
	struct libusb_device_descriptor dev_desc;
	struct libusb_config_descriptor *config_desc;
	int ret;
	int i;

	ret = libusb_get_device_descriptor(dev, &dev_desc);
	if (ret < 0) {
		fprintf(stderr, "failed to get device descriptor\n");
		return ;
	}

	printf("%s Device idVendor=%04x, idProduct=%04x:\n", 
			print_speed(libusb_get_device_speed(dev)),
			dev_desc.idVendor, dev_desc.idProduct);
	printf("bcdUSB:                 %04xh\n", dev_desc.bcdUSB);
	printf("bDeviceClass:           %02xh\n", dev_desc.bDeviceClass);
	printf("bDeviceSubClass:        %02xh\n", dev_desc.bDeviceSubClass);
	printf("bDeviceProtocol:        %02xh\n", dev_desc.bDeviceProtocol);
	printf("bMaxPacketSize:         %d\n", dev_desc.bMaxPacketSize0);
	printf("bcdDevice:              %04xh\n", dev_desc.bcdDevice);
	printf("bNumConfigurations:     %d\n", dev_desc.bNumConfigurations);

	for (i = 0; i < dev_desc.bNumConfigurations; i++) {
		ret = libusb_get_config_descriptor(dev, i, &config_desc);
		if (ret < 0) {
			fprintf(stderr, "failed to get config descriptor\n");
			return ;
		}

		print_configuration(config_desc);

		libusb_free_config_descriptor(config_desc);
	}

	printf("\n\n");
}

void print_devs(libusb_device **devs)
{
	libusb_device *dev;
	int i = 0;

	while ((dev = devs[i++]) != NULL) {
		print_dev(dev);
	}
}

int main(int argc, char *argv[])
{
	libusb_device **devs;
	int ret;

	ret = libusb_init(NULL);
	if (ret < 0)
		return 1;

	ret = libusb_get_device_list(NULL, &devs);
	if (ret < 0)
		goto exit;

	print_devs(devs);

	getchar();

	libusb_free_device_list(devs, 1);

exit:
	libusb_exit(NULL);
	return 0;
}
