/**
 * The JESD204 framework
 *
 * Copyright (c) 2018 Analog Devices Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#define pr_fmt(fmt) "jesd204: " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/debugfs.h>
#include <linux/of.h>
#include <linux/slab.h>

#include "jesd204-priv.h"

static DEFINE_MUTEX(jesd204_device_list_lock);
static LIST_HEAD(jesd204_device_list);

static dev_t jesd204_devt;

#define JESD204_DEV_MAX 256
struct bus_type jesd204_bus_type = {
	.name = "jesd204",
};
EXPORT_SYMBOL(jesd204_bus_type);

static struct dentry *jesd204_debugfs_dentry;

static struct device_node *of_jesd204_dev_get_node(struct device *dev)
{
	if (!dev->of_node)
		return NULL;
	return of_get_child_by_name(dev->of_node, "jesd204");
}

static int devm_jesd204_dev_match(struct device *dev, void *res, void *data)
{
	struct jesd204_dev **r = res;

	if (!r || !*r) {
		WARN_ON(!r || !*r);
		return 0;
	}

	return *r == data;
}

struct jesd204_dev *jesd204_dev_register(struct device *dev,
					 struct jesd204_dev_data *init)
{
	struct device_node *jnode;
	struct jesd204_dev *jdev;
	int ret;

	if (!dev)
		return ERR_PTR(-EINVAL);

	jnode = of_jesd204_dev_get_node(dev);
	if (!jnode)
		return NULL;

	if (!init || !init->name)
		return ERR_PTR(-EINVAL);

	jdev = kzalloc(sizeof(*jdev), GFP_KERNEL);
	if (!jdev)
		return ERR_PTR(-ENOMEM);

	jdev->name = kstrdup_const(init->name, GFP_KERNEL);
	if (!jdev->name) {
		ret = -ENOMEM;
		goto fail;
	}

	jdev->ops = init->ops;
	jdev->parent = get_device(dev);
	kref_init(&jdev->ref);

	mutex_lock(&jesd204_device_list_lock);
	list_add(&jdev->list, &jesd204_device_list);
	mutex_unlock(&jesd204_device_list_lock);

	return jdev;

fail:
	kfree_const(jdev->name);
	kfree(jdev);

	return ERR_PTR(ret);
}
EXPORT_SYMBOL(jesd204_dev_register);

/* Free memory allocated. */
static void __jesd204_dev_release(struct kref *ref)
{
	struct jesd204_dev *jdev = container_of(ref, struct jesd204_dev, ref);

	if (jdev->parent)
		put_device(jdev->parent);

	kfree_const(jdev->name);
	kfree(jdev);
}

/**
 * jesd204_dev_unregister() - unregister a device from the JESD204 subsystem
 * @jdev:		Device structure representing the device.
 **/
void jesd204_dev_unregister(struct jesd204_dev *jdev)
{
	if (!jdev || IS_ERR(jdev))
		return;

	kref_put(&jdev->ref, __jesd204_dev_release);
}
EXPORT_SYMBOL(jesd204_dev_unregister);

static void devm_jesd204_dev_unreg(struct device *dev, void *res)
{
	jesd204_dev_unregister(*(struct jesd204_dev **)res);
}

struct jesd204_dev *devm_jesd204_dev_register(struct device *dev,
					      struct jesd204_dev_data *init)
{
	struct jesd204_dev **jdevp, *jdev;

	if (!of_jesd204_dev_get_node(dev))
		return NULL;

	jdevp = devres_alloc(devm_jesd204_dev_unreg, sizeof(*jdevp),
			     GFP_KERNEL);
	if (!jdevp)
		return ERR_PTR(-ENOMEM);

	jdev = jesd204_dev_register(dev, init);
	if (!IS_ERR(jdev)) {
		*jdevp = jdev;
		devres_add(dev, jdevp);
	} else {
		devres_free(jdevp);
	}

	return jdev;
}
EXPORT_SYMBOL_GPL(devm_jesd204_dev_register);

/**
 * devm_jesd204_dev_unregister - Resource-managed jesd204_dev_unregister()
 * @dev:	Device this jesd204_dev belongs to
 * @jdev:	the jesd204_dev associated with the device
 *
 * Unregister jesd204_dev registered with devm_jesd204_dev_register().
 */
void devm_jesd204_dev_unregister(struct device *dev, struct jesd204_dev *jdev)
{
	int rc;

	rc = devres_release(dev, devm_jesd204_dev_unreg,
			    devm_jesd204_dev_match, jdev);
	WARN_ON(rc);
}
EXPORT_SYMBOL_GPL(devm_jesd204_dev_unregister);

static int __init jesd204_init(void)
{
	int ret;

	mutex_init(&jesd204_device_list_lock);

	/* Register sysfs bus */
	ret  = bus_register(&jesd204_bus_type);
	if (ret < 0) {
		pr_err("could not register bus type\n");
		goto error_nothing;
	}

	ret = alloc_chrdev_region(&jesd204_devt, 0, JESD204_DEV_MAX, "jesd204");
	if (ret < 0) {
		pr_err("failed to allocate char dev region\n");
		goto error_unregister_bus_type;
	}

	jesd204_debugfs_dentry = debugfs_create_dir("jesd204", NULL);

	return 0;

error_unregister_bus_type:
	bus_unregister(&jesd204_bus_type);
error_nothing:
	return ret;
}

static void __exit jesd204_exit(void)
{
	if (jesd204_devt)
		unregister_chrdev_region(jesd204_devt, JESD204_DEV_MAX);
	bus_unregister(&jesd204_bus_type);
	debugfs_remove_recursive(jesd204_debugfs_dentry);
	mutex_destroy(&jesd204_device_list_lock);
}

subsys_initcall(jesd204_init);
module_exit(jesd204_exit);

MODULE_AUTHOR("Alexandru Ardelean <alexandru.ardelean@analog.com>");
MODULE_DESCRIPTION("JESD204 core");
MODULE_LICENSE("GPL");
