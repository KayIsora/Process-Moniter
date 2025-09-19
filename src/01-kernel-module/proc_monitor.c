#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>
#include "proc_monitor.h"
#include <linux/time_types.h>
#include <linux/types.h>

#define BUF_SIZE 128

static int monitor_enabled = 0;
static struct proc_dir_entry *monitor_entry, *stats_entry, *io_entry;
static char stats_buf[BUF_SIZE];

// /proc/sysmonitor - read/write
static ssize_t monitor_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    char kbuf[2];
    if (count == 0)
        return 0;
    if (count > 1) count = 1;
    if (copy_from_user(kbuf, buf, count))
        return -EFAULT;
    kbuf[count] = 0;
    if (kbuf[0] == '1')
        monitor_enabled = 1;
    else if (kbuf[0] == '0')
        monitor_enabled = 0;
    else
        return -EINVAL;
    return count;
}

static ssize_t monitor_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    char msg[32];
    int len = snprintf(msg, sizeof(msg), "Monitor: %d\n", monitor_enabled);
    return simple_read_from_buffer(buf, count, ppos, msg, len);
}

// 1. Thay đổi này cho kernel 6.8.0 trở lên
static const struct proc_ops monitor_proc_ops = {
    .proc_read = monitor_read,
    .proc_write = monitor_write,
};

// /proc/sysstats - read-only
static ssize_t stats_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    int len = snprintf(stats_buf, BUF_SIZE, "Uptime: %lu sec\nLoad: 0.1\n", (unsigned long)jiffies/HZ);
    return simple_read_from_buffer(buf, count, ppos, stats_buf, len);
}

static const struct proc_ops stats_proc_ops = {
    .proc_read = stats_read,
};

// /proc/sysio - read-only
static int io_read(struct seq_file *seq, void *v)
{
    seq_printf(seq, "IO Count: 42\nIO Errors: 0\n");
    return 0;
}

static int io_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, io_read, NULL);
}

static const struct proc_ops io_proc_ops = {
    .proc_open = io_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static int __init proc_monitor_init(void)
{
    monitor_entry = proc_create(PROC_MONITOR_NAME, 0666, NULL, &monitor_proc_ops);
    stats_entry   = proc_create(PROC_STATS_NAME,   0444, NULL, &stats_proc_ops);
    io_entry      = proc_create(PROC_IO_NAME,      0444, NULL, &io_proc_ops);
    pr_info("proc_monitor kernel module loaded\n");
    return 0;
}

static void __exit proc_monitor_exit(void)
{
    proc_remove(monitor_entry);
    proc_remove(stats_entry);
    proc_remove(io_entry);
    pr_info("proc_monitor kernel module unloaded\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tên nhóm/tác giả");
MODULE_DESCRIPTION("SysMonitor Kernel Module Example");
module_init(proc_monitor_init);
module_exit(proc_monitor_exit);
