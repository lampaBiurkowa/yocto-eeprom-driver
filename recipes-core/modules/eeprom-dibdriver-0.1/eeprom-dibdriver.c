#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lampaBiurkowa");
MODULE_DESCRIPTION("A simple driver for 24lc512 EEPROM");

static struct i2c_client *eeprom_client;

static int my_eeprom_probe(struct i2c_client *client);
static void my_eeprom_remove(struct i2c_client *client);

static struct of_device_id my_driver_ids[] = {
	{
		.compatible = "at24,myOwn24c256",
	}, { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, my_driver_ids);

static struct i2c_device_id dibeeprom[] = {
	{"dibeeprom", 0},
	{ },
};
MODULE_DEVICE_TABLE(i2c, dibeeprom);

static struct i2c_driver my_driver = {
	.probe = my_eeprom_probe,
	.remove = my_eeprom_remove,
	.id_table = dibeeprom,
	.driver = {
		.name = "dibeeprom",
		.of_match_table = my_driver_ids,
	},
};

static struct proc_dir_entry *proc_file;
static uint16_t saved_address;

static ssize_t my_write(struct file *File, const char *user_buffer, size_t count, loff_t *offs) {
    char *buf;
    char i2c_buf[3];
    uint16_t addr;
    uint8_t value;
    int ret;

    buf = kmalloc(count + 1, GFP_KERNEL);
    if (!buf) {
        return -ENOMEM;
    }

    if (copy_from_user(buf, user_buffer, count)) {
        kfree(buf);
        return -EFAULT;
    }
    buf[count] = '\0';
    ret = sscanf(buf, "%hu %hhu", &addr, &value);
    kfree(buf);
    if (ret == 1) {
        saved_address = addr;
        printk(KERN_ERR "Saved address %u for future read\n", saved_address);
        return count;
    }
    else if (ret != 2) {
        return -EINVAL;
    }

    if (addr >= 512 || value > 0xFF) {
        return -EINVAL;
    }

    i2c_buf[0] = addr >> 8;
    i2c_buf[1] = addr & 0xFF;
    i2c_buf[2] = value;

    printk("Writing to address %u of EEPROM.\n", addr);

    ret = i2c_master_send(eeprom_client, i2c_buf, 3);
    if (ret < 0) {
        printk(KERN_ERR "Failed to write to EEPROM\n");
        return ret;
    }
    printk("wrote 0x%x from EEPROM.\n", i2c_buf[2]);

    return count;
}

static ssize_t my_read(struct file *File, char *user_buffer, size_t count, loff_t *offs) {
    char *buf;
    char addr_buf[2];
    char data_buf[1];
    struct i2c_msg msg;
    uint16_t addr;
    int ret;

    if (*offs > 0) {
        return 0;
    }

    addr = saved_address;

    if (addr >= 512) {
        return -EINVAL;
    }

    addr_buf[0] = addr >> 8;
    addr_buf[1] = addr & 0xFF;

    msg.addr = eeprom_client->addr;
    msg.flags = 0;
    msg.len = 2;
    msg.buf = addr_buf;
    ret = i2c_transfer(eeprom_client->adapter, &msg, 1);

    printk("Read address of %u.\n", addr);

    ret = i2c_master_recv(eeprom_client, data_buf, 1);
    if (ret < 0) {
        printk(KERN_ERR "Failed to read from EEPROM\n");
        return ret;
    }

    if (copy_to_user(user_buffer, data_buf, 1)) {
        return -EFAULT;
    }
    printk("Read 0x%x from EEPROM.\n", data_buf[0]);

    *offs = 1;

    return 1;
}

static const struct proc_ops fops = {
	.proc_write = my_write,
	.proc_read = my_read,
};

static int my_eeprom_probe(struct i2c_client *client) {
	printk("Now I am in the Probe function!\n");

	if(client->addr != 0x50) {
		printk("Wrong I2C address!\n");
		return -1;
	}

	eeprom_client = client;
		
	proc_file = proc_create("dibeeprom", 0666, NULL, &fops); //set correct permissions
	if(proc_file == NULL) {
		printk("Error creating /proc/dibeeprom\n");
		return -ENOMEM;
	}

	return 0;
}

static void my_eeprom_remove(struct i2c_client *client) {
	printk("Now I am in the Remove function!\n");
	proc_remove(proc_file);
}

module_i2c_driver(my_driver);
