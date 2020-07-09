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

#ifndef __UWB_API_IMPL_H__
#define __UWB_API_IMPL_H__

/**
 * Configure the mac layer
 * @param inst     Pointer to struct uwb_dev.
 * @param config   Pointer to struct uwb_dev_config.
 * @return struct uwb_dev_status
 *
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_mac_config(struct uwb_dev * dev, struct uwb_dev_config * config)
{
    return (dev->uw_funcs->uf_mac_config(dev, config));
}
EXPORT_SYMBOL(uwb_mac_config);

/**
 * Configuration of the uwb transmitter
 * including the power and pulse generator delay. The input is a pointer to the data structure
 * of type struct uwb_dev_txrf_config that holds all the configurable items.
 *
 * @param inst      Pointer to struct uwb_dev
 * @param config    Pointer to struct uwb_dev_txrf_config.
 * @return void
 */
UWB_API_IMPL_PREFIX void
uwb_txrf_config(struct uwb_dev * dev, struct uwb_dev_txrf_config *config)
{
    return (dev->uw_funcs->uf_txrf_config(dev, config));
}
EXPORT_SYMBOL(uwb_txrf_config);

/**
 * Translate coarse and fine power levels to a registry value used in struct uwb_dev_txrf_config.
 *
 * @param inst      Pointer to struct uwb_dev
 * @param reg       Pointer to where to store the registry value
 * @param coarse    Coarse power control value in dBm (DA)
 * @param fine      Fine power value in dBm (Mixer)
 * @return true on success, false otherwise
 */
UWB_API_IMPL_PREFIX bool
uwb_txrf_power_value(struct uwb_dev * dev, uint8_t *reg, dpl_float32_t coarse, dpl_float32_t fine)
{
    return (dev->uw_funcs->uf_txrf_power_value(dev, reg, coarse, fine));
}
EXPORT_SYMBOL(uwb_txrf_power_value);

/**
 *  Configure the device for both DEEP_SLEEP and SLEEP modes, and on-wake mode
 *  i.e., before entering the sleep, the device can be programmed for TX or RX, then upon "waking up" the TX/RX settings
 *  will be preserved and the device can immediately perform the desired action TX/RX.
 *
 * NOTE: e.g. Tag operation - after deep sleep, the device needs to just load the TX buffer and send the frame.
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return void
 */
UWB_API_IMPL_PREFIX void
uwb_sleep_config(struct uwb_dev * dev)
{
    return (dev->uw_funcs->uf_sleep_config(dev));
}
EXPORT_SYMBOL(uwb_sleep_config);

/**
 * API to enter device into sleep mode.
 *
 * @param inst   Pointer to struct uwb_dev.
 * @return struct uwb_dev_status
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_enter_sleep(struct uwb_dev * dev)
{
    return (dev->uw_funcs->uf_enter_sleep(dev));
}
EXPORT_SYMBOL(uwb_enter_sleep);

/**
 * Set the auto TX to sleep bit. This means that after a frame
 * transmission the device will enter deep sleep mode. The uwb_sleep_config() function
 * needs to be called before this to configure the on-wake settings.
 *
 * NOTE: the IRQ line has to be low/inactive (i.e. no pending events)
 *
 * @param inst       Pointer to struct uwb_dev.
 * @param enable     1 to configure the device to enter deep sleep after TX,
 *                   0 to disables the configuration.
 * @return struct uwb_dev_status
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_enter_sleep_after_tx(struct uwb_dev * dev, uint8_t enable)
{
    return (dev->uw_funcs->uf_enter_sleep_after_tx(dev, enable));
}
EXPORT_SYMBOL(uwb_enter_sleep_after_tx);

/**
 *  Sets the auto RX to sleep bit. This means that after a frame
 *  received the device will enter deep sleep mode. The uwb_sleep_config() function
 *  needs to be called before this to configure the on-wake settings.
 *
 * NOTE: the IRQ line has to be low/inactive (i.e. no pending events).
 * @param inst       Pointer to struct uwb_dev.
 * @param enable     1 to configure the device to enter deep sleep after TX,
 *                   0 to disables the configuration
 * @return struct uwb_dev_status
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_enter_sleep_after_rx(struct uwb_dev * dev, uint8_t enable)
{
    return (dev->uw_funcs->uf_enter_sleep_after_rx(dev, enable));
}
EXPORT_SYMBOL(uwb_enter_sleep_after_rx);

/**
 * Wakeup device from sleep to init.
 *
 * @param inst  struct uwb_dev.
 * @return struct uwb_dev_status
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_wakeup(struct uwb_dev * dev)
{
    return (dev->uw_funcs->uf_wakeup(dev));
}
EXPORT_SYMBOL(uwb_wakeup);

/**
 * Enable/Disable the double receive buffer mode.
 *
 * @param inst    Pointer to struct uwb_dev.
 * @param enable  1 to enable, 0 to disable the double buffer mode.
 *
 * @return struct uwb_dev_status
 *
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_set_dblrxbuff(struct uwb_dev * dev, bool enable)
{
    return (dev->uw_funcs->uf_set_dblrxbuf(dev, enable));
}
EXPORT_SYMBOL(uwb_set_dblrxbuff);

/**
 * Set Receive Wait Timeout period.
 *
 * @param inst      pointer to struct uwb_dev.
 * @param timeout   Indicates how long the receiver remains on from the RX enable command.
 *                  The time parameter used here is in 1.0256 * us (512/499.2MHz) units.
 *                  If set to 0 the timeout is disabled.
 * @return struct uwb_dev_status
 *
 * @brief The Receive Frame Wait Timeout period is a 32-bit field. The units for this parameter are roughly 1μs,
 * (the exact unit is 512 counts of the fundamental 499.2 MHz UWB clock, or 1.026 μs). When employing the frame wait timeout,
 * RXFWTO should be set to a value greater than the expected RX frame duration and include an allowance for any uncertainly
 * attaching to the expected transmission start time of the awaited frame.
 * When using .rxauto_enable feature it is important to understand the role of rx_timeout, in this situation it is the timeout
 * that actually turns-off the receiver and returns the transeiver to the idle state.
 *
 * NOTE: On dw1000 the timeout is a 16bit field only.
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_set_rx_timeout(struct uwb_dev *dev, uint32_t to)
{
    return (dev->uw_funcs->uf_set_rx_timeout(dev, to));
}
EXPORT_SYMBOL(uwb_set_rx_timeout);

/**
 * Adjust RX Wait Timeout period.
 *
 * @param inst      pointer to struct uwb_dev.
 * @param timeout   Indicates how long the receiver remains on from the RX enable command. The time parameter used here is in 1.0256
 * us (512/499.2MHz) units If set to 0 the timeout is disabled.
 * @return struct uwb_dev_status
 * @brief The Receive Frame Wait Timeout period is a 32-bit field. The units for this parameter are roughly 1μs,
 * (the exact unit is 512 counts of the fundamental 499.2 MHz UWB clock, or 1.026 μs). When employing the frame wait timeout,
 * RXFWTO should be set to a value greater than the expected RX frame duration and include an allowance for any uncertainly
 * attaching to the expected transmission start time of the awaited frame.
 * When using .rxauto_enable feature it is important to understand the role of rx_timeout, in this situation it is the timeout
 * that actually turns-off the receiver and returns the transeiver to the idle state.
 *
 * OBSERVE: This only works if the receiver has been setup to use a timeout with uwb_set_rx_timeout first.
 *
 * NOTE: On dw1000 the timeout is a 16bit field only.
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_adj_rx_timeout(struct uwb_dev *dev, uint32_t to)
{
    return (dev->uw_funcs->uf_adj_rx_timeout(dev, to));
}
EXPORT_SYMBOL(uwb_adj_rx_timeout);

/**
 * Set Absolute rx start and end, in dtu
 *
 * @param dev      pointer to struct uwb_dev.
 * @param rx_start The future time at which the RX will start, in dwt timeunits (uwb usecs << 16).
 * @param rx_end   The future time at which the RX will end, in dwt timeunits.
 *
 * @return struct uwb_dev_status
 *
 * @brief Automatically adjusts the rx_timeout after each received frame to match dx_time_end
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_set_rx_window(struct uwb_dev *dev, uint64_t rx_start, uint64_t rx_end)
{
    return (dev->uw_funcs->uf_set_rx_window(dev, rx_start, rx_end));
}
EXPORT_SYMBOL(uwb_set_rx_window);

/**
 * Set Absolute rx end, in dtu. NOTE: Doesn't set the initial
 *
 * @param dev      pointer to struct uwb_dev.
 * @param rx_end   The future time at which the RX will end, in dwt timeunits.
 *
 * @return struct uwb_dev_status
 *
 * @brief Automatically adjusts the rx_timeout after each received frame to match dx_time_end.
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_set_abs_timeout(struct uwb_dev *dev, uint64_t rx_end)
{
    return (dev->uw_funcs->uf_set_abs_timeout(dev, rx_end));
}
EXPORT_SYMBOL(uwb_set_abs_timeout);

/**
 * To specify a time in future to either turn on the receiver to be ready to receive a frame,
 * or to turn on the transmitter and send a frame.
 * The low-order 9-bits of this register are ignored.
 * The delay is in UWB microseconds * 65535 = DTU.
 *
 * @param inst     Pointer to uwb_dev.
 * @param delay    Delayed Send or receive Time, in dwt timeunits (uwb usecs << 16).
 * @return uwb_dev_status
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_set_delay_start(struct uwb_dev *dev, uint64_t dx_time)
{
    return (dev->uw_funcs->uf_set_delay_start(dev, dx_time));
}
EXPORT_SYMBOL(uwb_set_delay_start);

/**
 * Start transmission.
 *
 * @param inst  pointer to struct uwb_dev.
 * @return struct uwb_dev_status
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_start_tx(struct uwb_dev * dev)
{
    return (dev->uw_funcs->uf_start_tx(dev));
}
EXPORT_SYMBOL(uwb_start_tx);

/**
 * Activate reception mode (rx).
 *
 * @param inst  pointer to struct uwb_dev.
 * @return struct uwb_dev_status
 *
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_start_rx(struct uwb_dev * dev)
{
    return (dev->uw_funcs->uf_start_rx(dev));
}
EXPORT_SYMBOL(uwb_start_rx);

/**
 * Gracefully turn off reception mode.
 *
 * @param inst  pointer to struct uwb_dev
 * @return struct uwb_dev_status
 *
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_stop_rx(struct uwb_dev *dev)
{
    return (dev->uw_funcs->uf_stop_rx(dev));
}
EXPORT_SYMBOL(uwb_stop_rx);

/**
 * Write the supplied TX data into the tranceiver's
 * TX buffer.The input parameters are the data length in bytes and a pointer
 * to those data bytes.
 *
 ***** NIKLAS TODO: This text should be fixed, no need to mention CRC bytes here?
 * Note: This is the length of TX message (including the 2 byte CRC) - max is 1023 standard PHR mode allows up to 127 bytes
 * if > 127 is programmed, DWT_PHRMODE_EXT needs to be set in the phrMode configuration.
 *
 * @param dev                 Pointer to uwb_dev.
 * @param tx_frame_bytes      Pointer to the user buffer containing the data to send.
 * @param tx_buffer_offset    This specifies an offset in the tranceiver's TX Buffer where writing of data starts.
 * @param tx_frame_length     This is the total frame length, including the two byte CRC.
 * @return struct uwb_dev_status
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_write_tx(struct uwb_dev* dev, uint8_t *tx_frame_bytes,
             uint16_t tx_buffer_offset, uint16_t tx_frame_length)
{
    return (dev->uw_funcs->uf_write_tx(dev, tx_frame_bytes, tx_buffer_offset, tx_frame_length));
}
EXPORT_SYMBOL(uwb_write_tx);

/**
 * Configure the TX frame control register before the transmission of a frame.
 *
 * @param dev               Pointer to struct uwb_dev.
 * @param tx_frame_length   This is the length of TX message (excluding the 2 byte CRC) - max is 1023
 *                          NOTE: standard PHR mode allows up to 127 bytes.
 * @param tx_buffer_offset  The offset in the tx buffer to start writing the data.
 * @return void
 */
UWB_API_IMPL_PREFIX void
uwb_write_tx_fctrl(struct uwb_dev* dev, uint16_t tx_frame_length, uint16_t tx_buffer_offset)
{
    return (dev->uw_funcs->uf_write_tx_fctrl_ext(dev, tx_frame_length, tx_buffer_offset, 0));
}
EXPORT_SYMBOL(uwb_write_tx_fctrl);

/**
 * Configure the TX frame control register before the transmission of a frame with extended options.
 *
 * @param dev               Pointer to struct uwb_dev.
 * @param tx_frame_length   This is the length of TX message (excluding the 2 byte CRC) - max is 1023
 *                          NOTE: standard PHR mode allows up to 127 bytes.
 * @param tx_buffer_offset  The offset in the tx buffer to send data from.
 * @param ext               Optional pointer to struct uwb_fctrl_ext with additional parameters
 * @return void
 */
UWB_API_IMPL_PREFIX void
uwb_write_tx_fctrl_ext(struct uwb_dev* dev, uint16_t tx_frame_length,
                       uint16_t tx_buffer_offset, struct uwb_fctrl_ext *ext)
{
    return (dev->uw_funcs->uf_write_tx_fctrl_ext(dev, tx_frame_length, tx_buffer_offset, ext));
}
EXPORT_SYMBOL(uwb_write_tx_fctrl_ext);

/**
 * Wait for a DMA transfer
 *
 * @param inst     Pointer to struct uwb_dev.
 * @param timeout  Time in os_ticks to wait, use DPL_TIMEOUT_NEVER to wait indefinitely
 * @return void
 */
UWB_API_IMPL_PREFIX int
uwb_hal_noblock_wait(struct uwb_dev * dev, dpl_time_t timeout)
{
    return (dev->uw_funcs->uf_hal_noblock_wait(dev, timeout));
}
EXPORT_SYMBOL(uwb_hal_noblock_wait);

/**
 * Wait for a tx to complete
 *
 * @param inst     Pointer to struct uwb_dev.
 * @param timeout  Time in ticks to wait, use DPL_TIMEOUT_NEVER (UINT32_MAX) to wait indefinitely
 * @return int     Returns 0 on success, error code otherwise
 */
UWB_API_IMPL_PREFIX int
uwb_tx_wait(struct uwb_dev * dev, uint32_t timeout)
{
    return (dev->uw_funcs->uf_tx_wait(dev, timeout));
}
EXPORT_SYMBOL(uwb_tx_wait);

/**
 * Enable wait for response feature.
 *
 * @param dev               Pointer to struct uwb_dev.
 * @param enable            Enables/disables the wait for response feature.
 * @return struct uwb_dev_status
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_set_wait4resp(struct uwb_dev *dev, bool enable)
{
    return (dev->uw_funcs->uf_set_wait4resp(dev, enable));
}
EXPORT_SYMBOL(uwb_set_wait4resp);

/**
 * Wait-for-Response turn-around Time. This 20-bit field is used to configure the turn-around time between TX complete
 * and RX enable when the wait for response function is being used. This function is enabled by the WAIT4RESP control in
 * Register file: 0x0D – System Control Register. The time specified by this W4R_TIM parameter is in units of approximately 1 μs,
 * or 128 system clock cycles. This configuration may be used to save power by delaying the turn-on of the receiver,
 * to align with the response time of the remote system, rather than turning on the receiver immediately after transmission completes.
 *
 * @param inst   Pointer to struct uwb_dev.
 * @param delay  The delay is in UWB microseconds.
 *
 * @return struct uwb_dev_status
 *
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_set_wait4resp_delay(struct uwb_dev * dev, uint32_t delay)
{
    return (dev->uw_funcs->uf_set_wait4resp_delay(dev, delay));
}
EXPORT_SYMBOL(uwb_set_wait4resp_delay);

/**
 * Set rxauto disable
 *
 * @param inst   Pointer to struct uwb_dev.
 * @param disable  Disable mac-layer auto rx-reenable feature. The default behavior is rxauto enable, this API overrides default behavior
 * on an individual transaction.
 *
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_set_rxauto_disable(struct uwb_dev * dev, bool disable)
{
    return (dev->uw_funcs->uf_set_rxauto_disable(dev, disable));
}
EXPORT_SYMBOL(uwb_set_rxauto_disable);

/**
 * Read the current system time of the uwb tranceiver
 *
 * @param dev     Pointer to struct uwb_dev.
 * @return time   64bit uwt usecs
 */
UWB_API_IMPL_PREFIX uint64_t
uwb_read_systime(struct uwb_dev* dev)
{
    return (dev->uw_funcs->uf_read_systime(dev));
}
EXPORT_SYMBOL(uwb_read_systime);

/**
 * Read lower 32bit of system time.
 *
 * @param dev      Pointer to struct uwb_dev.
 *
 * @return time
 */
UWB_API_IMPL_PREFIX uint32_t
uwb_read_systime_lo32(struct uwb_dev* dev)
{
    return (dev->uw_funcs->uf_read_systime_lo32(dev));
}
EXPORT_SYMBOL(uwb_read_systime_lo32);

/**
 * Read receive time. (As adjusted by the LDE)
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return time
 */
UWB_API_IMPL_PREFIX uint64_t
uwb_read_rxtime(struct uwb_dev* dev)
{
    return (dev->uw_funcs->uf_read_rxtime(dev));
}
EXPORT_SYMBOL(uwb_read_rxtime);

/**
 * Read lower 32bit of receive time.
 *
 * @param dev      Pointer to struct uwb_dev.
 *
 * @return time
 */
UWB_API_IMPL_PREFIX uint32_t
uwb_read_rxtime_lo32(struct uwb_dev* dev)
{
    return (dev->uw_funcs->uf_read_rxtime_lo32(dev));
}
EXPORT_SYMBOL(uwb_read_rxtime_lo32);

/**
 * Read STS receive time. (As adjusted by the LDE)
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return time if valid, 0xFFFFFFFFFFFFFFFFULL otherwise
 */
UWB_API_IMPL_PREFIX uint64_t
uwb_read_sts_rxtime(struct uwb_dev* dev)
{
    return (dev->uw_funcs->uf_read_sts_rxtime(dev));
}
EXPORT_SYMBOL(uwb_read_sts_rxtime);

/**
 * Read transmit time.
 *
 * @param dev      Pointer to struct uwb_dev.
 *
 * @return time
 */
UWB_API_IMPL_PREFIX uint64_t
uwb_read_txtime(struct uwb_dev* dev)
{
    return (dev->uw_funcs->uf_read_txtime(dev));
}
EXPORT_SYMBOL(uwb_read_txtime);

/**
 * Read lower 32bit of transmit time.
 *
 * @param dev      Pointer to struct uwb_dev.
 *
 * @return time
 */
UWB_API_IMPL_PREFIX uint32_t
uwb_read_txtime_lo32(struct uwb_dev* dev)
{
    return (dev->uw_funcs->uf_read_txtime_lo32(dev));
}
EXPORT_SYMBOL(uwb_read_txtime_lo32);

/**
 * Calculate the frame duration (airtime) in usecs (not uwb usecs).
 * @param attrib    Pointer to struct uwb_phy_attributes_t * struct. The phy attritubes are part of the IEEE802.15.4-2011 standard.
 * Note the morphology of the frame depends on the mode of operation, see the dw*000_hal.c for the default behaviour
 * @param nlen      The length of the frame to be transmitted/received excluding crc
 * @return uint16_t duration in usec (not uwb usecs)
 */
UWB_API_IMPL_PREFIX uint16_t
uwb_phy_frame_duration(struct uwb_dev* dev, uint16_t nlen)
{
    return (dev->uw_funcs->uf_phy_frame_duration(dev, nlen));
}
EXPORT_SYMBOL(uwb_phy_frame_duration);

/**
 * API to calculate the SHR (Preamble + SFD) duration. This is used to calculate the correct rx_timeout.
 * @param attrib    Pointer to struct uwb_phy_attributes *. The phy attritubes are part of the IEEE802.15.4-2011 standard.
 * Note the morphology of the frame depends on the mode of operation, see the dw*000_hal.c for the default behaviour
 * @param nlen      The length of the frame to be transmitted/received excluding crc
 * @return uint16_t duration in usec (not uwb usec)
 */
UWB_API_IMPL_PREFIX uint16_t
uwb_phy_SHR_duration(struct uwb_dev* dev)
{
    return (dev->uw_funcs->uf_phy_SHR_duration(dev));
}
EXPORT_SYMBOL(uwb_phy_SHR_duration);

/**
 * API to calculate the data duration. This is used to calculate the correct rx_timeout.
 * @param attrib    Pointer to struct uwb_phy_attributes *. The phy attritubes are part of the IEEE802.15.4-2011 standard.
 * Note the morphology of the frame depends on the mode of operation, see the dw*000_hal.c for the default behaviour
 * @param nlen      The length of the frame to be transmitted/received excluding crc
 * @return uint16_t duration in usec (not uwb usec)
 */
UWB_API_IMPL_PREFIX uint16_t
uwb_phy_data_duration(struct uwb_dev* dev, uint16_t nlen)
{
    return (dev->uw_funcs->uf_phy_data_duration(dev, nlen));
}
EXPORT_SYMBOL(uwb_phy_data_duration);

/**
 * Turn off the transceiver.
 *
 * @param inst  Pointer to struct uwb_dev.
 * @return void
 */
UWB_API_IMPL_PREFIX void
uwb_phy_forcetrxoff(struct uwb_dev* dev)
{
    return (dev->uw_funcs->uf_phy_forcetrxoff(dev));
}
EXPORT_SYMBOL(uwb_phy_forcetrxoff);

/**
 * Reset the UWB receiver.
 *
 * @param inst   Pointer to struct uwb_dev.
 * @return void
 */
UWB_API_IMPL_PREFIX void
uwb_phy_rx_reset(struct uwb_dev * dev)
{
    return (dev->uw_funcs->uf_phy_rx_reset(dev));
}
EXPORT_SYMBOL(uwb_phy_rx_reset);

/**
 * Repeated tx mode
 *
 * @param inst   Pointer to struct uwb_dev.
 * @param rate   Rate at which frames with be repeated in dtu, set to 0 to disable
 * @return void
 */
UWB_API_IMPL_PREFIX void
uwb_phy_repeated_frames(struct uwb_dev * dev, uint64_t rate)
{
    return (dev->uw_funcs->uf_phy_repeated_frames(dev, rate));
}
EXPORT_SYMBOL(uwb_phy_repeated_frames);

/**
 * Enable rx regardless of hpdwarning
 * TODO: also for tx?
 * @param inst    Pointer to struct uwb_dev.
 * @param enable  weather to continue with rx regardless of error
 *
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_set_on_error_continue(struct uwb_dev * dev, bool enable)
{
    return (dev->uw_funcs->uf_set_on_error_continue(dev, enable));
}
EXPORT_SYMBOL(uwb_set_on_error_continue);

/**
 * Update PAN ID in device
 *
 * @param inst    Pointer to struct uwb_dev.
 * @param uint16_t pan id
 *
 */
UWB_API_IMPL_PREFIX void
uwb_set_panid(struct uwb_dev * dev, uint16_t pan_id)
{
    dev->pan_id = pan_id;
    return (dev->uw_funcs->uf_set_panid(dev, pan_id));
}
EXPORT_SYMBOL(uwb_set_panid);

/**
 * Update UID in device
 *
 * @param inst    Pointer to struct uwb_dev.
 * @param uint16_t uid
 *
 */
UWB_API_IMPL_PREFIX void
uwb_set_uid(struct uwb_dev * dev, uint16_t uid)
{
    dev->uid = uid;
    return (dev->uw_funcs->uf_set_uid(dev, uid));
}
EXPORT_SYMBOL(uwb_set_uid);

/**
 * Update EUID in device
 *
 * @param inst    Pointer to struct uwb_dev.
 * @param uint64_t euid
 *
 */
UWB_API_IMPL_PREFIX void
uwb_set_euid(struct uwb_dev * dev, uint64_t euid)
{
    dev->euid = euid;
    return (dev->uw_funcs->uf_set_euid(dev, euid));
}
EXPORT_SYMBOL(uwb_set_euid);

/**
 * Calculate the clock offset ratio from the carrior integrator value
 *
 * @param inst           Pointer to struct uwb_dev.
 * @param val            Carrier integrator value / TTCO value
 * @param type           uwb_cr_types_t
 *
 * @return dpl_float64_t   the relative clock offset ratio
 */
UWB_API_IMPL_PREFIX dpl_float64_t
uwb_calc_clock_offset_ratio(struct uwb_dev * dev, int32_t integrator_val, uwb_cr_types_t type)
{
    return (dev->uw_funcs->uf_calc_clock_offset_ratio(dev, integrator_val, type));
}
EXPORT_SYMBOL(uwb_calc_clock_offset_ratio);

/**
 * Calculate rssi from last RX in dBm.
 * Note that config.rxdiag_enable needs to be set.
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return rssi (dBm) on success
 */
UWB_API_IMPL_PREFIX dpl_float32_t
uwb_get_rssi(struct uwb_dev * dev)
{
    return (dev->uw_funcs->uf_get_rssi(dev));
}
EXPORT_SYMBOL(uwb_get_rssi);

/**
 * Calculate First Path Power Level from last RX in dBm,
 * Note that config.rxdiag_enable needs to be set.
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return fppl (dBm) on success
 */
UWB_API_IMPL_PREFIX dpl_float32_t
uwb_get_fppl(struct uwb_dev * dev)
{
    return (dev->uw_funcs->uf_get_fppl(dev));
}
EXPORT_SYMBOL(uwb_get_fppl);

/**
 * Calculate rssi from an rxdiag structure
 *
 * @param inst  Pointer to struct uwb_dev.
 * @param diag  Pointer to struct uwb_dev_rxdiag.
 *
 * @return rssi (dBm) on success
 */
UWB_API_IMPL_PREFIX dpl_float32_t
uwb_calc_rssi(struct uwb_dev * dev, struct uwb_dev_rxdiag * diag)
{
    return (dev->uw_funcs->uf_calc_rssi(dev, diag));
}
EXPORT_SYMBOL(uwb_calc_rssi);

/**
 * Calculate specific rssi for a sequence from an rxdiag structure
 *
 * @param inst  Pointer to struct uwb_dev.
 * @param diag  Pointer to struct uwb_dev_rxdiag.
 * @param type  One of UWB_RXDIAG_IPATOV, UWB_RXDIAG_STS, UWB_RXDIAG_STS2
 *
 * @return rssi (dBm) on success
 */
UWB_API_IMPL_PREFIX dpl_float32_t
uwb_calc_seq_rssi(struct uwb_dev * dev, struct uwb_dev_rxdiag * diag, uint16_t type)
{
    return (dev->uw_funcs->uf_calc_seq_rssi(dev, diag, type));
}
EXPORT_SYMBOL(uwb_calc_seq_rssi);

/**
 * Calculate First Path Power Level (fppl) from an rxdiag structure
 *
 * @param inst  Pointer to struct uwb_dev.
 * @param diag  Pointer to struct uwb_dev_rxdiag.
 *
 * @return fppl (dBm) on success
 */
UWB_API_IMPL_PREFIX dpl_float32_t
uwb_calc_fppl(struct uwb_dev * dev, struct uwb_dev_rxdiag * diag)
{
    return (dev->uw_funcs->uf_calc_fppl(dev, diag));
}
EXPORT_SYMBOL(uwb_calc_fppl);

/**
 * Give a rough estimate of how likely the received packet is
 * line of sight (LOS). Taken from 4.7 of DW1000 manual 2.12
 *
 * @param rssi rssi as calculated by uwb_calc_rssi
 * @param fppl fppl as calculated by uwb_calc_fppl
 *
 * @return 1.0 for likely LOS, 0.0 for non-LOS, with a sliding scale in between.
 */
UWB_API_IMPL_PREFIX dpl_float32_t
uwb_estimate_los(struct uwb_dev * dev, dpl_float32_t rssi, dpl_float32_t fppl)
{
    return (dev->uw_funcs->uf_estimate_los(dev, rssi, fppl));
}
EXPORT_SYMBOL(uwb_estimate_los);

/**
 * Calculate pdoa from an rxdiag structure
 *
 * @param inst  Pointer to struct uwb_dev.
 * @param diag  Pointer to struct uwb_dev_rxdiag.
 *
 * @return pdoa (radians) on success
 */
UWB_API_IMPL_PREFIX dpl_float32_t
uwb_calc_pdoa(struct uwb_dev * dev, struct uwb_dev_rxdiag * diag)
{
    return (dev->uw_funcs->uf_calc_pdoa(dev, diag));
}
EXPORT_SYMBOL(uwb_calc_pdoa);

/**
 * Synchronise clock to external clock if present
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return struct uwb_dev_status
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_sync_to_ext_clock(struct uwb_dev * dev)
{
    if (dev->uw_dyn_funcs.uf_sync_to_ext_clock) {
        return (dev->uw_dyn_funcs.uf_sync_to_ext_clock(dev));
    } else {
        return dev->status;
    }
}
EXPORT_SYMBOL(uwb_sync_to_ext_clock);

/**
 * Enable frame filtering
 *
 * @param inst  Pointer to struct uwb_dev.
 * @param bitmask  Enables/disables the frame filtering options according to
 *      UWB_FF_NOTYPE_EN        no frame types allowed (Frame filter disabled)
 *      UWB_FF_BEACON_EN        beacon frames allowed
 *      UWB_FF_DATA_EN          data frames allowed
 *      UWB_FF_ACK_EN           ack frames allowed
 *      UWB_FF_MAC_EN           mac control frames allowed
 *      UWB_FF_RSVD_EN          reserved frame types allowed
 *      UWB_FF_MULTI_EN         multipurpose frames allowed
 *      UWB_FF_FRAG_EN          fragmented frame types allowed
 *      UWB_FF_EXTEND_EN        extended frame types allowed
 *      UWB_FF_COORD_EN         behave as coordinator (can receive frames with no destination address (PAN ID has to match))
 *      UWB_FF_IMPBRCAST_EN     allow MAC implicit broadcast
 * @return struct uwb_dev_status
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_mac_framefilter(struct uwb_dev * dev, uint16_t enable)
{
    return (dev->uw_funcs->uf_mac_framefilter(dev, enable));
}
EXPORT_SYMBOL(uwb_mac_framefilter);

/**
 * Enable the auto-ACK feature.
 * NOTE: needs to have frame filtering enabled as well.
 *
 * @param inst    Pointer to _dw1000_dev_instance_t.
 * @param enable  If non-zero the ACK is sent after this delay, max is 255.
 * @return struct uwb_dev_status
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_set_autoack(struct uwb_dev * dev, bool enable)
{
    return (dev->uw_funcs->uf_set_autoack(dev, enable));
}
EXPORT_SYMBOL(uwb_set_autoack);

/**
 * Set the auto-ACK delay. If the delay (parameter) is 0, the ACK will
 * be sent as-soon-as-possable
 * otherwise it will be sent with a programmed delay (in symbols), max is 255.
 * NOTE: needs to have frame filtering enabled as well.
 *
 * @param inst   Pointer to _dw1000_dev_instance_t.
 * @param delay  If non-zero the ACK is sent after this delay, max is 255.
 * @return struct uwb_dev_status
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_set_autoack_delay(struct uwb_dev * dev, uint8_t delay)
{
    return (dev->uw_funcs->uf_set_autoack_delay(dev, delay));
}
EXPORT_SYMBOL(uwb_set_autoack_delay);

/**
 * Enable and/or reset device event counters
 *
 * @param inst    Pointer to struct uwb_dev.
 * @param enable  Enables the device internal event counters
 * @param reset   If true, reset counters
 * @return struct uwb_dev_status
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_event_cnt_ctrl(struct uwb_dev * dev, bool enable, bool reset)
{
    return (dev->uw_funcs->uf_event_cnt_ctrl(dev, enable, reset));
}
EXPORT_SYMBOL(uwb_event_cnt_ctrl);

/**
 * Read device event counters
 *
 * @param inst    Pointer to struct uwb_dev.
 * @param res     Pointer to struct uwb_dev_evcnt
 * @return struct uwb_dev_status
 */
UWB_API_IMPL_PREFIX struct uwb_dev_status
uwb_event_cnt_read(struct uwb_dev * dev, struct uwb_dev_evcnt *res)
{
    return (dev->uw_funcs->uf_event_cnt_read(dev, res));
}
EXPORT_SYMBOL(uwb_event_cnt_read);

#ifdef __cplusplus
}
#endif

#endif /* __UWB_H__ */
