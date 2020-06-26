/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <syscfg/syscfg.h>

#ifdef __KERNEL__
#include <dpl/dpl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <desense/desense.h>

#define slog(fmt, ...) \
    pr_info("uwb_desense: %s(): %d: "fmt, __func__, __LINE__, ##__VA_ARGS__)

struct debug_cmd {
    char *fn;
    struct uwb_desense_instance *desense;
};

static struct dentry *dir = 0;
static struct seq_file *seq_file = 0;

static int
buffer_printf(const char *fmt, ...)
{
    va_list args;
    if (!seq_file) {
        slog("No seq_file");
        return -1;
    }

	va_start(args, fmt);
	seq_vprintf(seq_file, fmt, args);
	va_end(args);
    return 0;
}

static int cmd_dump(struct seq_file *s, void *data)
{
    struct debug_cmd *cmd = (struct debug_cmd*) s->private;
    seq_file = s;

    slog("fn:%s", cmd->fn);

    if (!strcmp(cmd->fn, "data")) {
        desense_dump_data(cmd->desense, buffer_printf);
    }

    if (!strcmp(cmd->fn, "stats")) {
        desense_dump_stats(cmd->desense, buffer_printf);
    }

    seq_file = 0;
	return 0;
}

static int cmd_open(struct inode *inode, struct file *file)
{
    slog("");
	return single_open(file, cmd_dump, inode->i_private);
}

static const struct file_operations clk_dump_fops = {
	.open		= cmd_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static char* cmd_names[] = {
    "data",
    "stats",
    0
};

static char* dir_names[] = {
    "inst0",
    "inst1",
    "inst2",
    0
};

static struct debug_cmd cmd_s[ARRAY_SIZE(dir_names)*ARRAY_SIZE(cmd_names)];
static int cmd_s_idx = 0;

void desense_debugfs_init(struct uwb_desense_instance *desense)
{
    int i, j;
    struct dentry *subdir = 0;
    if (!dir) {
        dir = debugfs_create_dir("uwb_desense", NULL);
        if (!dir) {
            slog("Failed to create debugfs entry\n");
            return;
        }
    }

    if (desense->dev_inst->idx >= ARRAY_SIZE(dir_names)) {
        slog("device index exceeds directory name array\n");
        return;
    }

    i = desense->dev_inst->idx;
    subdir = debugfs_create_dir(dir_names[i], dir);
    if (!subdir) {
        slog("Failed to create debugfs subdir entry\n");
        return;
    }

    for (j=0;cmd_names[j];j++) {
        cmd_s[cmd_s_idx] = (struct debug_cmd){cmd_names[j], desense};
        if (!debugfs_create_file(cmd_names[j], S_IRUGO, subdir, &cmd_s[cmd_s_idx], &clk_dump_fops)) {
            slog("Failed to create cmd\n");
            return;
        }

        if (++cmd_s_idx > ARRAY_SIZE(cmd_s)) {
            pr_err("too few elements in debug_cmd vector\n");
            return;
        }
    }
}

void desense_debugfs_deinit(void)
{
    if (dir) {
        debugfs_remove_recursive(dir);
        dir = 0;
    }
}
#endif
