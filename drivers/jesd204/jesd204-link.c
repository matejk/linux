/**
 * JESD204 link creation via clocks
 *
 * Copyright (c) 2018 Analog Devices Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/of_platform.h>
#include <linux/i2c.h>
#include <linux/iio/iio.h>
#include <linux/spi/spi.h>

#include "jesd204-priv.h"

static int jesd204_dev_create_link(struct jesd204_dev *jdev,
				   struct jesd204_link_in *in,
				   struct jesd204_link_out *out)
{
	struct device *dev = jdev->parent;
	struct jesd204_dev_list *e;

	e = devm_kzalloc(dev, sizeof(*e), GFP_KERNEL);
	if (!e)
		return -ENOMEM;

	/* Link this output dev to the input */
	in->jdev = out->jdev;

	/* Add this device to the output of the device */
	e->jdev = jdev;
	list_add(&e->list, &out->jdev_list);

	return 0;
}

static struct jesd204_link_out *
jesd204_dev_find_output_link(struct list_head *jdev_list, struct clk *clk)
{
	struct jesd204_link_out *link;
	struct jesd204_dev *jdev;

	if (list_empty(jdev_list))
		return NULL;

	list_for_each_entry(jdev, jdev_list, list) {
		if (list_empty(&jdev->outputs))
			continue;
		list_for_each_entry(link, &jdev->outputs, list) {
			if (clk_is_match(link->clk, clk))
				return link;
		}
	}

	return NULL;
}

static int jesd204_dev_update_input_links(struct list_head *jdev_list,
					  struct jesd204_link_out *out)
{
	struct device *dev = out->jdev->parent;
	struct jesd204_link_in *in;
	struct jesd204_dev *jdev;
	int ret;

	if (list_empty(jdev_list))
		return 0;

	list_for_each_entry(jdev, jdev_list, list) {
		if (list_empty(&jdev->inputs))
			continue;
		list_for_each_entry(in, &jdev->inputs, list) {
			if (!in->jdev && clk_is_match(in->clk, out->clk)) {
				ret = jesd204_dev_create_link(out->jdev, in, out);
				if (ret < 0)
					return ret;
			}
		}
	}

	return 0;
}

static int jesd204_dev_init_output_links(struct jesd204_dev *jdev,
					 struct jesd204_dev_data *init,
					 struct list_head *jdev_list)
{
	struct device *dev = jdev->parent;
	struct jesd204_link_out *out;
	unsigned int i;
	int ret;

	/* Nothing to do if this device has no clock outputs */
	if (!init->output_clocks || !init->output_clocks_num)
		return 0;

	for (i = 0; i < init->output_clocks_num; i++) {
		struct clk *c = init->output_clocks[i];

		if (!c) {
			dev_err(dev, "null clock reference (%u)\n", i);
			return -EINVAL;
		}

		out = devm_kzalloc(dev, sizeof(*out), GFP_KERNEL);
		if (!out)
			return -ENOMEM;

		out->clk = c;
		out->jdev = jdev;
		INIT_LIST_HEAD(&out->jdev_list);

		/* Go over all input links and update any with this output */
		ret = jesd204_dev_update_input_links(jdev_list, out);
		if (ret < 0)
			return ret;

		list_add(&out->list, &jdev->outputs);
	}

	return 0;
}

static int jesd204_dev_init_input_links(struct jesd204_dev *jdev,
					struct list_head *jdev_list)
{
	struct device *dev = jdev->parent;
	struct jesd204_link_out *out;
	struct jesd204_link_in *in;
	struct device_node *np;
	int i, ret, count;

	np = dev->of_node;
	count = of_count_phandle_with_args(np, "clocks", "#clock-cells");
	if (count == -ENOENT)
		return 0;
	if (count < 1)
		return count;

	for (i = 0; i < count; i++) {
		struct clk *c = of_clk_get(np, i);

		if (IS_ERR(c))
			return PTR_ERR(c);

		in = devm_kzalloc(dev, sizeof(*in), GFP_KERNEL);
		if (!in)
			return -ENOMEM;

		in->clk = c;

		out = jesd204_dev_find_output_link(jdev_list, c);
		if (out) {
			ret = jesd204_dev_create_link(jdev, in, out);
			if (ret < 0)
				return ret;
		}

		list_add(&in->list, &jdev->inputs);
	}

	return 0;
}

int jesd204_dev_init_links(struct jesd204_dev *jdev,
			   struct jesd204_dev_data *init,
			   struct list_head *jdev_list)
{
	int ret;

	INIT_LIST_HEAD(&jdev->inputs);
	INIT_LIST_HEAD(&jdev->outputs);
	
	ret = jesd204_dev_init_output_links(jdev, init, jdev_list);

	if (ret < 0)
		return ret;

	ret = jesd204_dev_init_input_links(jdev, jdev_list);

	if (ret < 0)
		return ret;

	return 0;
}
