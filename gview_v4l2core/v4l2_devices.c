/*******************************************************************************#
#           guvcview              http://guvcview.sourceforge.net               #
#                                                                               #
#           Paulo Assis <pj.assis@gmail.com>                                    #
#                                                                               #
# This program is free software; you can redistribute it and/or modify          #
# it under the terms of the GNU General Public License as published by          #
# the Free Software Foundation; either version 2 of the License, or             #
# (at your option) any later version.                                           #
#                                                                               #
# This program is distributed in the hope that it will be useful,               #
# but WITHOUT ANY WARRANTY; without even the implied warranty of                #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 #
# GNU General Public License for more details.                                  #
#                                                                               #
# You should have received a copy of the GNU General Public License             #
# along with this program; if not, write to the Free Software                   #
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA     #
#                                                                               #
********************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <linux/videodev2.h>
#include <libv4l2.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "gviewv4l2core.h"
#include "v4l2_devices.h"
#include "../config.h"

extern int verbosity;

/* device list structure */
static v4l2_device_list_t my_device_list;

/*
 * get the device list
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: pointer to statically allocated device list
 */
v4l2_device_list_t* get_device_list()
{
	return &my_device_list;
}

/*
 * get the number of available v4l2 devices
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: number of available v4l2 devices
 */
int v4l2core_get_num_devices()
{
	return my_device_list.num_devices;
}

/*
 * get the device sys data for index
 * args:
 *   index - device index
 * 
 * asserts:
 *   none
 * 
 * returns: pointer to device sys data
 */
v4l2_dev_sys_data_t* v4l2core_get_device_sys_data(int index)
{
	if(index >= v4l2core_get_num_devices())
	{
		fprintf(stderr, "V4L2_CORE: invalid device index %i using %i\n",
			index, v4l2core_get_num_devices() - 1);
		index = v4l2core_get_num_devices() - 1;
	}

	if(index < 0)
	{
		fprintf(stderr, "V4L2_CORE: invalid device index %i using 0\n", index);
		index = 0;
	}

	return &(my_device_list.list_devices[index]);
}

/*
 * free v4l2 devices list
 * args:
 *   none
 *
 * asserts:
 *   vd->list_devices is not null
 *
 * returns: void
 */
static void free_device_list()
{
	/*assertions*/
	assert(my_device_list.list_devices != NULL);

	int i=0;
	for(i=0;i<(my_device_list.num_devices);i++)
	{
		free(my_device_list.list_devices[i].device);
		free(my_device_list.list_devices[i].name);
		free(my_device_list.list_devices[i].driver);
		free(my_device_list.list_devices[i].location);
	}
	free(my_device_list.list_devices);
	my_device_list.list_devices = NULL;
}
 
/*
 * enumerate available v4l2 devices
 * and creates list in vd->list_devices
 * args:
 *   none
 *
 * asserts:
 *   my_device_list.udev is not null
 *   my_device_list.list_devices is null
 *
 * returns: error code
 */
int enum_v4l2_devices()
{
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices;
    struct udev_list_entry *dev_list_entry;

    int num_dev = 0;
    struct v4l2_capability v4l2_cap;

    my_device_list.list_devices = calloc(1, sizeof(v4l2_dev_sys_data_t));
    if(my_device_list.list_devices == NULL)
	{
		fprintf(stderr, "V4L2_CORE: FATAL memory allocation failure (enum_v4l2_devices): %s\n", strerror(errno));
		exit(-1);
	}

    /* Create a list of the devices in the 'v4l2' subsystem. */
    enumerate = udev_enumerate_new(my_device_list.udev);
    udev_enumerate_add_match_subsystem(enumerate, "video4linux");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);
    /*
     * For each item enumerated, print out its information.
     * udev_list_entry_foreach is a macro which expands to
     * a loop. The loop will be executed for each member in
     * devices, setting dev_list_entry to a list entry
     * which contains the device's path in /sys.
     */
    udev_list_entry_foreach(dev_list_entry, devices)
    {
        const char *path;

        /*
         * Get the filename of the /sys entry for the device
         * and create a udev_device object (dev) representing it
         */
        path = udev_list_entry_get_name(dev_list_entry);
        struct udev_device *dev = udev_device_new_from_syspath(my_device_list.udev, path);

        /* usb_device_get_devnode() returns the path to the device node
            itself in /dev. */
        const char *v4l2_device = udev_device_get_devnode(dev);
        if (verbosity > 0)
            printf("V4L2_CORE: Device Node Path: %s\n", v4l2_device);

		int fd = 0;
        /* open the device and query the capabilities */
        if ((fd = v4l2_open(v4l2_device, O_RDWR | O_NONBLOCK, 0)) < 0)
        {
            fprintf(stderr, "V4L2_CORE: ERROR opening V4L2 interface for %s\n", v4l2_device);
            v4l2_close(fd);
            continue; /*next dir entry*/
        }

        if (xioctl(fd, VIDIOC_QUERYCAP, &v4l2_cap) < 0)
        {
            fprintf(stderr, "V4L2_CORE: VIDIOC_QUERYCAP error: %s\n", strerror(errno));
            fprintf(stderr, "V4L2_CORE: couldn't query device %s\n", v4l2_device);
            v4l2_close(fd);
            continue; /*next dir entry*/
        }
        v4l2_close(fd);

        num_dev++;
        /* Update the device list*/
        my_device_list.list_devices = realloc(my_device_list.list_devices, num_dev * sizeof(v4l2_dev_sys_data_t));
        if(my_device_list.list_devices == NULL)
		{
			fprintf(stderr, "V4L2_CORE: FATAL memory allocation failure (enum_v4l2_devices): %s\n", strerror(errno));
			exit(-1);
		}
        my_device_list.list_devices[num_dev-1].device = strdup(v4l2_device);
        my_device_list.list_devices[num_dev-1].name = strdup((char *) v4l2_cap.card);
        my_device_list.list_devices[num_dev-1].driver = strdup((char *) v4l2_cap.driver);
        my_device_list.list_devices[num_dev-1].location = strdup((char *) v4l2_cap.bus_info);
        my_device_list.list_devices[num_dev-1].valid = 1;
        my_device_list.list_devices[num_dev-1].current = 0;
				
        /* The device pointed to by dev contains information about
            the v4l2 device. In order to get information about the
            USB device, get the parent device with the
            subsystem/devtype pair of "usb"/"usb_device". This will
            be several levels up the tree, but the function will find
            it.*/
        dev = udev_device_get_parent_with_subsystem_devtype(
                dev,
                "usb",
                "usb_device");
        if (!dev)
        {
            fprintf(stderr, "V4L2_CORE: Unable to find parent usb device.");
            continue;
        }

        /* From here, we can call get_sysattr_value() for each file
            in the device's /sys entry. The strings passed into these
            functions (idProduct, idVendor, serial, etc.) correspond
            directly to the files in the directory which represents
            the USB device. Note that USB strings are Unicode, UCS2
            encoded, but the strings returned from
            udev_device_get_sysattr_value() are UTF-8 encoded. */
        if (verbosity > 0)
        {
            printf("  VID/PID: %s %s\n",
                udev_device_get_sysattr_value(dev,"idVendor"),
                udev_device_get_sysattr_value(dev, "idProduct"));
            printf("  %s\n  %s\n",
                udev_device_get_sysattr_value(dev,"manufacturer"),
                udev_device_get_sysattr_value(dev,"product"));
            printf("  serial: %s\n",
                udev_device_get_sysattr_value(dev, "serial"));
            printf("  busnum: %s\n",
                udev_device_get_sysattr_value(dev, "busnum"));
            printf("  devnum: %s\n",
                udev_device_get_sysattr_value(dev, "devnum"));
        }

        my_device_list.list_devices[num_dev-1].vendor = strtoull(udev_device_get_sysattr_value(dev,"idVendor"), NULL, 16);
        my_device_list.list_devices[num_dev-1].product = strtoull(udev_device_get_sysattr_value(dev, "idProduct"), NULL, 16);
        my_device_list.list_devices[num_dev-1].busnum = strtoull(udev_device_get_sysattr_value(dev, "busnum"), NULL, 10);
		my_device_list.list_devices[num_dev-1].devnum = strtoull(udev_device_get_sysattr_value(dev, "devnum"), NULL, 10);

        udev_device_unref(dev);
    }
    /* Free the enumerator object */
    udev_enumerate_unref(enumerate);

    my_device_list.num_devices = num_dev;

    return(E_OK);
}

/*
 * Initiate the device list (with udev monitoring)
 * args:
 *   none
 * 
 * asserts:
 *   none
 * 
 * returns: none
 */ 
void v4l2core_init_device_list()
{
	/* Create a udev object */
	my_device_list.udev = udev_new();
	/*start udev device monitoring*/
	/* Set up a monitor to monitor v4l2 devices */
	if(my_device_list.udev)
	{
		my_device_list.udev_mon = udev_monitor_new_from_netlink(my_device_list.udev, "udev");
		udev_monitor_filter_add_match_subsystem_devtype(my_device_list.udev_mon, "video4linux", NULL);
		udev_monitor_enable_receiving(my_device_list.udev_mon);
		/* Get the file descriptor (fd) for the monitor */
		my_device_list.udev_fd = udev_monitor_get_fd(my_device_list.udev_mon);

		enum_v4l2_devices();
	}
} 

/*
 * get the device index in device list
 * args:
 *   videodevice - string with videodevice node (e.g: /dev/video0)
 * 
 * asserts:
 *   none
 * 
 * returns:
 *   videodevice index in device list [0 - num_devices[ or -1 on error
 */ 
int v4l2core_get_device_index(const char *videodevice)
{
	if(my_device_list.num_devices > 0 && my_device_list.list_devices != NULL)
	{
		int dev_index = 0;
		for(dev_index = 0; dev_index < my_device_list.num_devices; ++dev_index)
		{
			if(strcmp(videodevice, my_device_list.list_devices[dev_index].device)==0)
			{
				my_device_list.list_devices[dev_index].current = 1;
				return(dev_index);
			}
		}
	}
	
	fprintf(stderr,"V4L2CORE: couldn't determine device (%s) list index\n", videodevice);
	return -1;
}

/*
 * check for new devices
 * args:
 *   vd - pointer to device data (can be null)
 *
 * asserts:
 *   my_device_list.udev is not null
 *   my_device_list.udev_fd is valid (> 0)
 *   my_device_list.udev_mon is not null
 *
 * returns: true(1) if device list was updated, false(0) otherwise
 */
int check_device_list_events(v4l2_dev_t *vd)
{
	/*assertions*/
	assert(my_device_list.udev != NULL);
	assert(my_device_list.udev_fd > 0);
	assert(my_device_list.udev_mon != NULL);

    fd_set fds;
    struct timeval tv;
    int ret = 0;

    FD_ZERO(&fds);
    FD_SET(my_device_list.udev_fd, &fds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    ret = select(my_device_list.udev_fd+1, &fds, NULL, NULL, &tv);

    /* Check if our file descriptor has received data. */
    if (ret > 0 && FD_ISSET(my_device_list.udev_fd, &fds))
    {
        /*
         * Make the call to receive the device.
         *   select() ensured that this will not block.
         */
        struct udev_device *dev = udev_monitor_receive_device(my_device_list.udev_mon);
        if (dev)
        {
            if (verbosity > 0)
            {
                printf("V4L2_CORE: Got Device event\n");
                printf("          Node: %s\n", udev_device_get_devnode(dev));
                printf("     Subsystem: %s\n", udev_device_get_subsystem(dev));
                printf("       Devtype: %s\n", udev_device_get_devtype(dev));
                printf("        Action: %s\n", udev_device_get_action(dev));
            }

            /*update device list*/
            if(my_device_list.list_devices != NULL)
				free_device_list();
            enum_v4l2_devices();

            /*update the current device index*/
            if(vd)
            {
				vd->this_device = v4l2core_get_device_index(vd->videodevice);
				if(vd->this_device < 0)
					vd->this_device = 0;
	
				if(my_device_list.list_devices)
					my_device_list.list_devices[vd->this_device].current = 1;
			}
			
            udev_device_unref(dev);

            return(1);
        }
        else
            fprintf(stderr, "V4L2_CORE: No Device from receive_device(). An error occured.\n");
    }

    return(0);
}

/*
 * close v4l2 devices list
 * args:
 *   none
 *
 * asserts:
 *   vd->list_devices is not null
 *
 * returns: void
 */
void v4l2core_close_v4l2_device_list()
{
	free_device_list();
	
	if (my_device_list.udev)
		udev_unref(my_device_list.udev);
}
