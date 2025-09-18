#include "btif_audio_ts.h"

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>

struct BtifAudioTs {
	unsigned long long local_time;
	unsigned int audio_time;
};

/* Device driver-related */
#define DEVICE_NAME "btts"
#define CLASS_NAME  "bttscl"
#define STPBT_GET_TS _IOR('w', 10, struct BtifAudioTs)
static int g_major_num;
static struct class *g_dev_class;
static struct device *g_device;

/* Timestamp for sample 0 */
static DEFINE_SPINLOCK(g_ts0_lock);
static struct BtifAudioTs g_ts0;

/* Timestamp ring buffer */
#define TS_BUFFER_SIZE 128
#define TS_READ_MIN (TS_BUFFER_SIZE / 4)
static DEFINE_SPINLOCK(g_ts_buffer_lock);
static struct BtifAudioTs g_ts_buffer[TS_BUFFER_SIZE];
static int g_rptr, g_wptr, g_buf_size;

/**
 Add timestamp and GPT value to the storage.
 If ringbuffer is full we do not wait for more space available,
 we overwrite the oldest value.
*/
void btif_update_audio_ts(unsigned int audio_ts, u64 ts)
{
	if (audio_ts == 0) {
		spin_lock(&g_ts0_lock);
		g_ts0.audio_time = audio_ts;
		g_ts0.local_time = ts;
		spin_unlock(&g_ts0_lock);
	}

	spin_lock(&g_ts_buffer_lock);
	/* clean the buffer if stream is restarted */
	if (audio_ts == 0) {
		g_rptr = g_wptr = g_buf_size = 0;
	}
	/* put data into the buffer */
	g_ts_buffer[g_wptr].audio_time = audio_ts;
	g_ts_buffer[g_wptr].local_time = ts;
	/* update pointers and size */
	if (++g_wptr >= TS_BUFFER_SIZE) {
		g_wptr = 0;
	}
	if (g_buf_size < TS_BUFFER_SIZE) {
		++g_buf_size;
	} else {
		g_rptr = g_wptr;
	}
	spin_unlock(&g_ts_buffer_lock);
}

/**** Driver ****/

static int dev_open(struct inode *inodep, struct file *filep)
{
	pr_debug("BTTS: flags %x\n", filep->f_flags);
	if ((filep->f_flags & O_NONBLOCK) == 0) {
		pr_err("BTTS: blocking operations are not supported");
		return -EINVAL;
	}
	return 0;
}

/**
 Read API.
 Reads at least TS_READ_MIN BtifAudioTs structs from timestamp ringbuffer.
 If no data is available, the call does not block and returns -EAGAIN,
 user space should retry at a later point.
*/
static ssize_t dev_read(struct file *filep,
				char *buffer, size_t len, loff_t *offset)
{
	size_t entries_to_read = len / sizeof(struct BtifAudioTs);
	size_t tail_size = 0, size = 0;
	ssize_t retval = -EACCES;

	spin_lock(&g_ts_buffer_lock);

	if (entries_to_read > g_buf_size) {
		if (g_buf_size < TS_READ_MIN) {
			retval = -EAGAIN;
			goto fail;
		}
		entries_to_read = g_buf_size;
	}

	if (g_rptr + entries_to_read > TS_BUFFER_SIZE) {
		tail_size = (TS_BUFFER_SIZE - g_rptr) *
				sizeof(struct BtifAudioTs);
		if (copy_to_user(buffer,
				g_ts_buffer + g_rptr, tail_size)) {
			goto fail;
		}
		g_buf_size -= (TS_BUFFER_SIZE - g_rptr);
		entries_to_read -= (TS_BUFFER_SIZE - g_rptr);
		g_rptr = 0;
		*offset += tail_size;
		buffer += tail_size;
		retval = tail_size;
	}

	size = entries_to_read * sizeof(struct BtifAudioTs);
	if (copy_to_user(buffer, g_ts_buffer + g_rptr, size)) {
		goto fail;
	}
	g_buf_size -= entries_to_read;
	g_rptr += entries_to_read;
	*offset += size;
	retval += size;

fail:
	spin_unlock(&g_ts_buffer_lock);

	return retval;
}

/**
 ioctl API.
 Handles STPBT_GET_TS command. Returns a BtifAudioTs struct
 which corresponds to the beginning of a stream.
 If called before the first stream playback, will return {0,0}.
*/
static long dev_unlocked_ioctl(struct file *filp,
				unsigned int cmd, unsigned long arg)
{
	long retval = 0;

	switch (cmd) {
	case STPBT_GET_TS: {
		struct BtifAudioTs ts;

		spin_lock(&g_ts0_lock);
		ts = g_ts0;
		spin_unlock(&g_ts0_lock);

		if (copy_to_user((struct BtifAudioTs __user *)arg,
				&ts, sizeof(ts))) {
			retval = -EACCES;
		}
		break;
	}
	default:
		retval = -EFAULT;
		pr_err("BTTS: unknown cmd (%x)\n", cmd);
		break;
	}

	return retval;
}

static long dev_compat_ioctl(struct file *filp,
				unsigned int cmd, unsigned long arg)
{
	return dev_unlocked_ioctl(filp, cmd, arg);
}

static int dev_release(struct inode *inodep, struct file *filep)
{
	pr_debug("BTTS: release\n");
	return 0;
}

static const struct file_operations fops = {
	.open = dev_open,
	.read = dev_read,
	.unlocked_ioctl = dev_unlocked_ioctl,
	.compat_ioctl = dev_compat_ioctl,
	.release = dev_release,
};

int btif_ts_init(void)
{
	pr_debug("BTTS: initializing\n");

	/* Try to dynamically allocate a major number for the device */
	g_major_num = register_chrdev(0, DEVICE_NAME, &fops);
	if (g_major_num < 0) {
		pr_alert("BTTS: failed to register a major number\n");
		return g_major_num;
	}

	/* Register the device class */
	g_dev_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(g_dev_class)) {
		unregister_chrdev(g_major_num, DEVICE_NAME);
		pr_alert("BTTS: failed to register device class\n");
		return PTR_ERR(g_dev_class);
	}

	/* Register the device driver */
	g_device = device_create(g_dev_class,
				NULL, MKDEV(g_major_num, 0), NULL, DEVICE_NAME);
	if (IS_ERR(g_device)) {
		class_destroy(g_dev_class);
		unregister_chrdev(g_major_num, DEVICE_NAME);
		pr_alert("BTTS: failed to create the device\n");
		return PTR_ERR(g_device);
	}
	pr_debug("BTTS: initialized\n");
	return 0;
}

void btif_ts_exit(void)
{
	device_destroy(g_dev_class, MKDEV(g_major_num, 0));
	class_unregister(g_dev_class);
	class_destroy(g_dev_class);
	unregister_chrdev(g_major_num, DEVICE_NAME);
	pr_debug("BTTS: exit\n");
}

