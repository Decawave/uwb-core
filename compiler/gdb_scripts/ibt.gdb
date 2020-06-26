# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.


define spibt_print_one
    set $start_t = $arg0
    set $p = $arg1
    set $p_last = $arg2
    set $diff = 0
    if $arg3 != 0
        set $diff = ($p.utime - $p_last.utime)
    end
    if $p.utime != 0
        printf " %10ld ", $inst.bt_ticks2usec * $p.utime
        printf " %10ld ", $inst.bt_ticks2usec * ($p.utime - $start_t)
        printf " %8ld ", $inst.bt_ticks2usec * $diff
        printf " %6ld ", $inst.bt_ticks2usec * ($p.utime_end - $p.utime)
        printf " %3s%s ", ($p.non_blocking)?"NB-":"", ($p.is_write)?"W":"R"

        set $j = 0
        while $j<4
            if $j < $p.cmd_len
                printf "%02X", $p.cmd[$j]
            else
                printf "  "
            end
            set $j++
        end

        printf " %4d ", $p.data_len
        set $j = 0
        while $j < $p.data_len && $j < sizeof($p.data)
            printf "%02X", $p.data[$j]
            set $j++
        end
        printf "\n"
    end
end

define ibt_print_one
    set $start_t = $arg0
    set $p = $arg1
    set $p_last = $arg2
    set $diff = 0
    if $arg3 != 0
      set $diff = ($p.utime - $p_last.utime)
    end
    if $p.utime != 0
        printf " %10ld ", $inst.bt_ticks2usec * $p.utime
        printf " %10ld ", $inst.bt_ticks2usec * ($p.utime - $start_t)
        printf " %8ld ", $inst.bt_ticks2usec * $diff
        printf " %6ld ", $inst.bt_ticks2usec * ($p.utime_end - $p.utime)
        printf " %02X %02X ", ($p.fctrl&0xff), ($p.fctrl>>8)
        if sizeof($p.sys_status_hi) == 1
            printf " %10X ", (((uint64_t)$p.sys_status_hi)<<32)|$p.sys_status_lo
        else
            printf " %16X ", (((uint64_t)$p.sys_status_hi)<<32)|$p.sys_status_lo
        end
    end
end

define ibt
    printf " %10s ", "abs"
    printf " %10s ", "us"
    printf " %8s ", "diff"
    printf " %6s ", "dur"
    printf " %5s", "fctrl"
    printf " %10s \n", "status"
    set $inst = $arg0
    set $i = 0
    set $i_max = sizeof($inst.sys_status_bt)/sizeof(*$inst.sys_status_bt)
    set $start_t = 0
    while $i < $i_max
        set $i_mod = ($inst.sys_status_bt_idx + $i +1) % $i_max
        set $p = $inst.sys_status_bt[$i_mod]
        if $start_t == 0
           set $start_t = $p.utime
        end
        ibt_print_one $start_t $p $p_last $i
        if $p.utime != 0
          printf "\n"
        end
        set $i++
        set $p_last = $p
    end
end

document ibt
usage: ibt_dump <pointer to instance>
Dumps the specified instance's interrupt backtrace.
Ex: ibt_dump hal_dw1000_instances[0]
end

define spibt
    printf " %10s ", "abs"
    printf " %10s ", "us"
    printf " %8s ", "diff"
    printf " %6s ", "dur"
    printf " %5s", "fctrl"
    printf " %10s \n", "status"
    set $inst = $arg0
    set $i = 0
    set $i_max = sizeof($inst.spi_bt)/sizeof(*$inst.spi_bt)
    set $start_t = 0
    while $i < $i_max
        set $i_mod = ($inst.spi_bt_idx + $i +1) % $i_max
        set $p = $inst.spi_bt[$i_mod]
        if $start_t == 0
           set $start_t = $p.utime
        end
        spibt_print_one $start_t $p $p_last $i
        set $i = $i+1
        set $p_last = $p
    end
end

document spibt
usage: spibt_dump <pointer to instance>
Dumps the specified instance's interrupt backtrace.
Ex: spibt_dump hal_dw1000_instances[0]
end

define uwb_bt
    printf " %10s ", "abs"
    printf " %10s ", "us"
    printf " %8s ", "diff"
    printf " %6s ", "dur"
    printf " %5s", "fctrl"
    printf " %10s \n", "status"
    set $inst = $arg0
    set $spi_i = 0
    set $irq_i = 0
    set $spi_i_max = sizeof($inst.spi_bt)/sizeof(*$inst.spi_bt)
    set $irq_i_max = sizeof($inst.sys_status_bt)/sizeof(*$inst.sys_status_bt)
    set $start_t = 0
    set $spi_p_last = 0
    set $irq_p_last = 0
    while $spi_i < $spi_i_max || $irq_i < $irq_i_max
        set $spi_i_mod = ($inst.spi_bt_idx + $spi_i +1) % $spi_i_max
        set $irq_i_mod = ($inst.sys_status_bt_idx + $irq_i +1) % $irq_i_max
        set $spi_p = $inst.spi_bt[$spi_i_mod]
        set $irq_p = $inst.sys_status_bt[$irq_i_mod]

        if $spi_p.utime < $irq_p.utime && $spi_i < $spi_i_max || $irq_i > $irq_i_max
            if $start_t == 0
              set $start_t = $spi_p.utime
            end
            spibt_print_one $start_t $spi_p $spi_p_last $spi_i
            set $spi_i++
            set $spi_p_last = $spi_p
        else
            if $start_t == 0
              set $start_t = $irq_p.utime
            end
            printf "\e[7m"
            ibt_print_one $start_t $irq_p $irq_p_last $irq_i
            printf "\e[27m\n"
            set $irq_i++
            set $irq_p_last = $irq_p
        end
    end
end

document uwb_bt
usage: uwb_bt <pointer to instance>
Dumps the specified instance's interrupt backtrace.
Ex1: uwb_bt hal_dw1000_instances[0]
Ex2: uwb_bt hal_dw3000_instances[0]
end
