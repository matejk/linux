/**
 * The JESD204 framework
 *
 * Copyright (c) 2018 Analog Devices Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#ifndef _JESD204_H_
#define _JESD204_H_

struct jesd204_dev;

struct jesd204_dev_ops {

};

struct jesd204_dev_data {
	const char				*name;
	struct jesd204_dev_ops			*ops;
	struct clk				**output_clocks;
	uint32_t				output_clocks_num;
};

struct jesd204_dev *jesd204_dev_register(struct device *dev,
					 struct jesd204_dev_data *init);
struct jesd204_dev *devm_jesd204_dev_register(struct device *dev,
					      struct jesd204_dev_data *init);

void jesd204_dev_unregister(struct jesd204_dev *jdev);
void devm_jesd204_unregister(struct device *dev, struct jesd204_dev *jdev);

#endif
