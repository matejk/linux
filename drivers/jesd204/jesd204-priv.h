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
};

#endif /* _JESD204_PRIV_H_ */
