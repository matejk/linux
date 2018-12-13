/**
 * The JESD204 framework
 *
 * Copyright (c) 2018 Analog Devices Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#ifndef _JESD204_PRIV_H_
#define _JESD204_PRIV_H_

#include <linux/jesd204/jesd204.h>

extern struct bus_type jesd204_bus_type;

struct jesd204_dev;

/**
 * struct jesd204_link_in - JESD204 link input link
 * @list		list entry for a device to keep a list of links
 * @clk			clock to match with the out-clock of a JESD204 device
 * @dev			ref to JESD204 device that provides this input link
 */
struct jesd204_link_in {
	struct list_head		list;
	struct clk			*clk;
	struct jesd204_dev		*jdev;
};

/**
 * struct jesd204_dev_list - list of JESD204 device refs
 * @list		list entry for a link to keep a list of device refs
 * @jdev		reference to JESD204 dev
 */
struct jesd204_dev_list {
	struct list_head		list;
	struct jesd204_dev		*jdev;
};

/**
 * struct jesd204_link_out - JESD204 link output link
 * @list		list entry for a device to keep a list of links
 * @clk			clock this device outputs (to other devices)
 * @jdev		reference to (parent) device that provides this output
 * @dev_list		list of refs to JESD204 devs that have this dev as input
 */
struct jesd204_link_out {
	struct list_head		list;
	struct clk			*clk;
	struct jesd204_dev		*jdev;
	struct list_head		jdev_list;
};

/**
 * struct jesd204_dev - JESD204 device
 * @list		list entry for the framework to keep a list of devices
 * @name		name of the device
 * @parent		parent device that registers itself as a JESD204 device
 * @ops			JESD204 operations specified via function pointers
 * @ref			ref count for this JESD204 device
 * @inputs		list of JESD204 devices that are inputs of this device
 * @outputs		list of JESD204 devices that take input from this device
 */
struct jesd204_dev {
	struct list_head		list;

	const char			*name;
	struct device			*parent;
	struct jesd204_dev_ops		*ops;
	struct kref			ref;

	struct list_head		inputs;
	struct list_head		outputs;
};

int jesd204_dev_init_links(struct jesd204_dev *jdev,
			   struct jesd204_dev_data *init,
			   struct list_head *jdev_list);

#endif /* _JESD204_PRIV_H_ */
