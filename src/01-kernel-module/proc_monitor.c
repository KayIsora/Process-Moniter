#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "proc_monitor.h"
#include <linux/time_types.h>
#include <linux/types.h>

#define BUF_SIZE 128

int monitor_enabled = 0;
static struct proc_dir_entry *monitor_entry, *stats_entry, *io_entry;
static char stats_buf[BUF_SIZE];

// /proc/sysmonitor - read/write
ssize_t monitor_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
    char kbuf[2];
    if (count == 0)
        return 0;

    if (count > 1) count = 1;  // chỉ lấy 1 ký tự đầu
    if (copy_from_user(kbuf, buf, count))
        return -EFAULT;

    kbuf[count] = '\0';

    // Loại bỏ ký tự newline nếu có
    if (kbuf[0] == '\n')
        return count;

    if (kbuf[0] == '1')
        monitor_enabled = 1;
    else if (kbuf[0] == '0')
        monitor_enabled = 0;
    else
        return -EINVAL;  // Giá trị không hợp lệ trả lỗi 

    return count;
}


ssize_t monitor_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    char msg[32];
    int len = snprintf(msg, sizeof(msg), "Monitor: %d\n", monitor_enabled);
    return simple_read_from_buffer(buf, count, ppos, msg, len);
}

static const struct file_operations monitor_fops = {
    .owner = THIS_MODULE,
    .read = monitor_read,
    .write = monitor_write,
};

// /proc/sysstats - read-only
ssize_t stats_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    int len;
    len = snprintf(stats_buf, BUF_SIZE, "Uptime: %lu sec\nLoad: 0.1\n", (unsigned long)jiffies/HZ);
    return simple_read_from_buffer(buf, count, ppos, stats_buf, len);
}

static const struct file_operations stats_fops = {
    .owner = THIS_MODULE,
    .read = stats_read,
};

// /proc/sysio - read-only
ssize_t io_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    char iobuf[BUF_SIZE];
    int len = snprintf(iobuf, sizeof(iobuf), "IO Count: 42\nIO Errors: 0\n");
    return simple_read_from_buffer(buf, count, ppos, iobuf, len);
}

static const struct file_operations io_fops = {
    .owner = THIS_MODULE,
    .read = io_read,
};

static int __init proc_monitor_init(void) {
    monitor_entry = proc_create(PROC_MONITOR_NAME, 0666, NULL, &monitor_fops);
    stats_entry   = proc_create(PROC_STATS_NAME,   0444, NULL, &stats_fops);
    io_entry      = proc_create(PROC_IO_NAME,      0444, NULL, &io_fops);
    pr_info("proc_monitor kernel module loaded\n");
    return 0;
}

static void __exit proc_monitor_exit(void) {
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
