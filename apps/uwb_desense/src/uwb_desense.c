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

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/kernel.h>

#include <dpl/dpl.h>
#include <dpl/dpl_types.h>
#include <dpl/dpl_eventq.h>
#include <uwb/uwb.h>
#include <desense/desense.h>

MODULE_AUTHOR("UWBCore Maintainer <uwbcore@gmail.com>");
MODULE_DESCRIPTION("UWB Desense RF Tests");
MODULE_LICENSE("Dual MPL/GPL");
MODULE_ALIAS("");

#define slog(fmt, ...)                                                \
    pr_info("uwb_desense: %s(): %d: "fmt, __func__, __LINE__, ##__VA_ARGS__)


static int __init uwb_desense_entry(void)
{
    uwb_desense_pkg_init();
    return 0;
}

static void __exit uwb_desense_exit(void)
{
    uwb_desense_pkg_down(0);
}

module_init(uwb_desense_entry);
module_exit(uwb_desense_exit);
