# Top Level UWB-Core module
KBUILD_CFLAGS   += -fms-extensions -std=gnu11 -Wno-declaration-after-statement -DSPLIT_LOADER=0

src := $(if $(filter /%,$(src)),$(src),$(srctree)/$(src))

subdir-ccflags-y += -Wno-error
subdir-ccflags-y += -Wno-missing-braces -Wno-microsoft-anon-tag
subdir-ccflags-y += -Wno-incompatible-pointer-types-discards-qualifiers

ccflags-y += -I$(src)/lib/rng_math/include
ccflags-y += -I$(src)/lib/uwb_softfloat/include
ccflags-y += -I$(src)/hw/drivers/uwb/include
ccflags-$(CONFIG_UWB_DW1000) += -I$(src)/hw/drivers/uwb/uwb_dw1000/include
ccflags-$(CONFIG_UWB_DW3000) += -I$(src)/hw/drivers/uwb/uwb_dw3000-c0/include
ccflags-y += -I$(src)/porting/dpl_lib/include
ccflags-y += -I$(src)/porting/dpl/kernel/include
ccflags-y += -I$(src)/porting/dpl/kernel/uwb_hal/include
ccflags-y += -I$(src)/bin/targets/syscfg/generated/include
ccflags-y += -I$(src)/sys/uwbcfg/include
ccflags-y += -I$(src)/lib/dsp/include
ccflags-y += -I$(src)/lib/euclid/include
ccflags-y += -I$(src)/lib/cir/include
ccflags-y += -I$(src)/lib/cir/cir_dw1000/include
ccflags-y += -I$(src)/lib/cir/cir_dw3000-c0/include
ccflags-y += -I$(src)/lib/uwb_rng/include
ccflags-y += -I$(src)/lib/uwb_ccp/include
ccflags-y += -I$(src)/lib/uwb_wcs/include
ccflags-y += -I$(src)/lib/json/include
ccflags-y += -I$(src)/lib/tdma/include
ccflags-y += -I$(src)/lib/uwb_transport/include

# uwbcore.ko kernel module generated from uwbcore.c + all other files
# uwbcore depends on uwb-hal
obj-$(CONFIG_UWB_CORE) := uwbcore.o
uwbcore-y	:= porting/dpl/kernel/src/uwbcore.o
uwbcore-$(CONFIG_UWB_DW1000) += porting/dpl/kernel/src/uwbcore_dw1000_init.o
uwbcore-$(CONFIG_UWB_DW3000) += porting/dpl/kernel/src/uwbcore_dw3000_init.o
uwbcore-y	+= porting/dpl/kernel/src/config.o
uwbcore-y   += bin/targets/syscfg/generated/src/syscfg-sysinit-app.o
uwbcore-y   += bin/targets/syscfg/generated/src/syscfg-sysdown-app.o
uwbcore-y	+= porting/dpl/kernel/src/hal_spi.o
uwbcore-y	+= porting/dpl/kernel/src/hal_gpio.o
uwbcore-y	+= porting/dpl/kernel/src/hal_timer.o
uwbcore-y	+= porting/dpl/kernel/src/dpl_sem.o
uwbcore-y	+= porting/dpl/kernel/src/dpl_mutex.o
uwbcore-y	+= porting/dpl/kernel/src/dpl_task.o
uwbcore-y	+= porting/dpl/kernel/src/dpl_eventq.o
uwbcore-y	+= porting/dpl/kernel/src/dpl_callout.o
uwbcore-y	+= porting/dpl/kernel/src/dpl_time.o
uwbcore-y	+= porting/dpl/kernel/src/dpl_cputime.o
uwbcore-y	+= porting/dpl/kernel/src/stats.o
uwbcore-y	+= porting/dpl/kernel/src/stats_sysfs.o
uwbcore-y	+= porting/dpl_lib/src/dpl_mbuf.o
uwbcore-y	+= porting/dpl_lib/src/dpl_mempool.o
uwbcore-y	+= porting/dpl_lib/src/streamer.o
uwbcore-y	+= hw/drivers/uwb/src/uwb.o
uwbcore-y	+= lib/cir/src/cir.o
uwbcore-y	+= lib/cir/src/cir_json.o
uwbcore-y	+= lib/cir/src/cir_chrdev.o
uwbcore-$(CONFIG_UWB_DW1000)	+= hw/drivers/uwb/uwb_dw1000/src/dw1000_dev.o
uwbcore-$(CONFIG_UWB_DW1000)	+= hw/drivers/uwb/uwb_dw1000/src/dw1000_hal.o
uwbcore-$(CONFIG_UWB_DW1000)	+= hw/drivers/uwb/uwb_dw1000/src/dw1000_phy.o
uwbcore-$(CONFIG_UWB_DW1000)	+= hw/drivers/uwb/uwb_dw1000/src/dw1000_mac.o
uwbcore-$(CONFIG_UWB_DW1000)	+= hw/drivers/uwb/uwb_dw1000/src/dw1000_otp.o
uwbcore-$(CONFIG_UWB_DW1000)	+= hw/drivers/uwb/uwb_dw1000/src/dw1000_gpio.o
uwbcore-$(CONFIG_UWB_DW1000)	+= hw/drivers/uwb/uwb_dw1000/src/dw1000_cli.o
uwbcore-$(CONFIG_UWB_DW1000)	+= hw/drivers/uwb/uwb_dw1000/src/dw1000_sysfs.o
uwbcore-$(CONFIG_UWB_DW1000)	+= hw/drivers/uwb/uwb_dw1000/src/dw1000_debugfs.o
uwbcore-$(CONFIG_UWB_DW1000)	+= hw/drivers/uwb/uwb_dw1000/src/dw1000_pkg.o
uwbcore-$(CONFIG_UWB_DW1000)	+= lib/cir/cir_dw1000/src/cir_dw1000.o
uwbcore-$(CONFIG_UWB_DW3000)	+= hw/drivers/uwb/uwb_dw3000-c0/src/dw3000_dev.o
uwbcore-$(CONFIG_UWB_DW3000)	+= hw/drivers/uwb/uwb_dw3000-c0/src/dw3000_hal.o
uwbcore-$(CONFIG_UWB_DW3000)	+= hw/drivers/uwb/uwb_dw3000-c0/src/dw3000_phy.o
uwbcore-$(CONFIG_UWB_DW3000)	+= hw/drivers/uwb/uwb_dw3000-c0/src/dw3000_mac.o
uwbcore-$(CONFIG_UWB_DW3000)	+= hw/drivers/uwb/uwb_dw3000-c0/src/dw3000_otp.o
uwbcore-$(CONFIG_UWB_DW3000)	+= hw/drivers/uwb/uwb_dw3000-c0/src/dw3000_gpio.o
uwbcore-$(CONFIG_UWB_DW3000)	+= hw/drivers/uwb/uwb_dw3000-c0/src/dw3000_cli.o
uwbcore-$(CONFIG_UWB_DW3000)	+= hw/drivers/uwb/uwb_dw3000-c0/src/dw3000_sysfs.o
uwbcore-$(CONFIG_UWB_DW3000)	+= hw/drivers/uwb/uwb_dw3000-c0/src/dw3000_debugfs.o
uwbcore-$(CONFIG_UWB_DW3000)	+= hw/drivers/uwb/uwb_dw3000-c0/src/dw3000_pkg.o
uwbcore-$(CONFIG_UWB_DW3000)	+= lib/cir/cir_dw3000-c0/src/cir_dw3000.o
# Config
uwbcore-y	+= sys/uwbcfg/src/uwbcfg.o
uwbcore-y	+= sys/uwbcfg/src/uwbcfg_sysfs.o
uwbcore-$(CONFIG_UWB_DW1000) += sys/uwbcfg/src/uwbcfg_dw1000.o
uwbcore-$(CONFIG_UWB_DW3000) += sys/uwbcfg/src/uwbcfg_dw3000.o
# Rng
uwbcore-y	+= lib/uwb_rng/src/uwb_rng.o
uwbcore-y	+= lib/uwb_rng/src/rng_json.o
uwbcore-y	+= lib/uwb_rng/src/rng_encode.o
uwbcore-y	+= lib/uwb_rng/src/slots.o
uwbcore-y	+= lib/uwb_rng/src/rng_chrdev.o
uwbcore-y	+= lib/uwb_rng/src/rng_sysfs.o
uwbcore-y	+= lib/twr_ss/src/twr_ss.o
uwbcore-y	+= lib/twr_ss_ack/src/twr_ss_ack.o
uwbcore-y	+= lib/twr_ss_ext/src/twr_ss_ext.o
uwbcore-y	+= lib/twr_ds/src/twr_ds.o
uwbcore-y	+= lib/twr_ds_ext/src/twr_ds_ext.o
uwbcore-y       += lib/rng_math/src/rng_math.o
# CCP
uwbcore-y	+= lib/uwb_ccp/src/uwb_ccp.o
uwbcore-y	+= lib/uwb_ccp/src/ccp_chrdev.o
uwbcore-y	+= lib/uwb_ccp/src/ccp_json.o
uwbcore-y	+= lib/uwb_ccp/src/ccp_sysfs.o
# WCS
uwbcore-y	+= lib/uwb_wcs/src/uwb_wcs.o
uwbcore-y	+= lib/uwb_wcs/src/wcs_json.o
uwbcore-y	+= lib/uwb_wcs/src/wcs_chrdev.o
uwbcore-y	+= lib/uwb_wcs/src/wcs_timescale.o
# JSON
uwbcore-y	+= lib/json/src/json_util.o
uwbcore-y	+= lib/json/src/json_encode.o
uwbcore-y	+= lib/json/src/json_decode.o

# TDMA
uwbcore-y	+= lib/tdma/src/tdma.o

# UWB Transport
uwbcore-y	+= lib/uwb_transport/src/uwb_transport.o

# Softfloat - OBJS_PRIMITIVES
uwbcore-y += lib/uwb_softfloat/src/s_compare96M.o
uwbcore-y += lib/uwb_softfloat/src/s_compare128M.o
uwbcore-y += lib/uwb_softfloat/src/s_shortShiftLeft64To96M.o
uwbcore-y += lib/uwb_softfloat/src/s_shortShiftLeftM.o
uwbcore-y += lib/uwb_softfloat/src/s_shiftLeftM.o
uwbcore-y += lib/uwb_softfloat/src/s_shortShiftRightM.o
uwbcore-y += lib/uwb_softfloat/src/s_shortShiftRightJam64.o
uwbcore-y += lib/uwb_softfloat/src/s_shortShiftRightJamM.o
uwbcore-y += lib/uwb_softfloat/src/s_shiftRightJam32.o
uwbcore-y += lib/uwb_softfloat/src/s_shiftRightJam64.o
uwbcore-y += lib/uwb_softfloat/src/s_shiftRightJamM.o
uwbcore-y += lib/uwb_softfloat/src/s_shiftRightM.o
uwbcore-y += lib/uwb_softfloat/src/s_countLeadingZeros8.o
uwbcore-y += lib/uwb_softfloat/src/s_countLeadingZeros32.o
uwbcore-y += lib/uwb_softfloat/src/s_countLeadingZeros64.o
uwbcore-y += lib/uwb_softfloat/src/s_addM.o
uwbcore-y += lib/uwb_softfloat/src/s_addCarryM.o
uwbcore-y += lib/uwb_softfloat/src/s_addComplCarryM.o
uwbcore-y += lib/uwb_softfloat/src/s_negXM.o
uwbcore-y += lib/uwb_softfloat/src/s_sub1XM.o
uwbcore-y += lib/uwb_softfloat/src/s_subM.o
uwbcore-y += lib/uwb_softfloat/src/s_mul64To128M.o
uwbcore-y += lib/uwb_softfloat/src/s_mul128MTo256M.o
uwbcore-y += lib/uwb_softfloat/src/s_approxRecip_1Ks.o
uwbcore-y += lib/uwb_softfloat/src/s_approxRecip32_1.o
uwbcore-y += lib/uwb_softfloat/src/s_approxRecipSqrt_1Ks.o
uwbcore-y += lib/uwb_softfloat/src/s_approxRecipSqrt32_1.o
uwbcore-y += lib/uwb_softfloat/src/s_remStepMBy32.o

# Softfloat - OBJS_SPECIALIZE
uwbcore-y += lib/uwb_softfloat/src/softfloat_raiseFlags.o
uwbcore-y += lib/uwb_softfloat/src/s_f32UIToCommonNaN.o
uwbcore-y += lib/uwb_softfloat/src/s_commonNaNToF32UI.o
uwbcore-y += lib/uwb_softfloat/src/s_propagateNaNF32UI.o
uwbcore-y += lib/uwb_softfloat/src/s_f64UIToCommonNaN.o
uwbcore-y += lib/uwb_softfloat/src/s_commonNaNToF64UI.o
uwbcore-y += lib/uwb_softfloat/src/s_propagateNaNF64UI.o
uwbcore-y += lib/uwb_softfloat/src/f128M_isSignalingNaN.o
uwbcore-y += lib/uwb_softfloat/src/s_f128MToCommonNaN.o
uwbcore-y += lib/uwb_softfloat/src/s_commonNaNToF128M.o
uwbcore-y += lib/uwb_softfloat/src/s_propagateNaNF128M.o

# Softfloat - OBJS_OTHERS
uwbcore-y += lib/uwb_softfloat/src/s_roundToUI32.o
uwbcore-y += lib/uwb_softfloat/src/s_roundMToUI64.o
uwbcore-y += lib/uwb_softfloat/src/s_roundToI32.o
uwbcore-y += lib/uwb_softfloat/src/s_roundMToI64.o
uwbcore-y += lib/uwb_softfloat/src/s_normSubnormalF32Sig.o
uwbcore-y += lib/uwb_softfloat/src/s_roundPackToF32.o
uwbcore-y += lib/uwb_softfloat/src/s_normRoundPackToF32.o
uwbcore-y += lib/uwb_softfloat/src/s_addMagsF32.o
uwbcore-y += lib/uwb_softfloat/src/s_subMagsF32.o
uwbcore-y += lib/uwb_softfloat/src/s_mulAddF32.o
uwbcore-y += lib/uwb_softfloat/src/s_normSubnormalF64Sig.o
uwbcore-y += lib/uwb_softfloat/src/s_roundPackToF64.o
uwbcore-y += lib/uwb_softfloat/src/s_normRoundPackToF64.o
uwbcore-y += lib/uwb_softfloat/src/s_addMagsF64.o
uwbcore-y += lib/uwb_softfloat/src/s_subMagsF64.o
uwbcore-y += lib/uwb_softfloat/src/s_mulAddF64.o
uwbcore-y += lib/uwb_softfloat/src/s_isNaNF128M.o
uwbcore-y += lib/uwb_softfloat/src/s_tryPropagateNaNF128M.o
uwbcore-y += lib/uwb_softfloat/src/s_invalidF128M.o
uwbcore-y += lib/uwb_softfloat/src/s_shiftNormSigF128M.o
uwbcore-y += lib/uwb_softfloat/src/s_roundPackMToF128M.o
uwbcore-y += lib/uwb_softfloat/src/s_normRoundPackMToF128M.o
uwbcore-y += lib/uwb_softfloat/src/s_addF128M.o
uwbcore-y += lib/uwb_softfloat/src/s_mulAddF128M.o
uwbcore-y += lib/uwb_softfloat/src/softfloat_state.o
uwbcore-y += lib/uwb_softfloat/src/ui32_to_f32.o
uwbcore-y += lib/uwb_softfloat/src/ui32_to_f64.o
uwbcore-y += lib/uwb_softfloat/src/ui32_to_f128M.o
uwbcore-y += lib/uwb_softfloat/src/ui64_to_f32.o
uwbcore-y += lib/uwb_softfloat/src/ui64_to_f64.o
uwbcore-y += lib/uwb_softfloat/src/ui64_to_f128M.o
uwbcore-y += lib/uwb_softfloat/src/i32_to_f32.o
uwbcore-y += lib/uwb_softfloat/src/i32_to_f64.o
uwbcore-y += lib/uwb_softfloat/src/i32_to_f128M.o
uwbcore-y += lib/uwb_softfloat/src/i64_to_f32.o
uwbcore-y += lib/uwb_softfloat/src/i64_to_f64.o
uwbcore-y += lib/uwb_softfloat/src/i64_to_f128M.o
uwbcore-y += lib/uwb_softfloat/src/f32_to_ui32.o
uwbcore-y += lib/uwb_softfloat/src/f32_to_ui64.o
uwbcore-y += lib/uwb_softfloat/src/f32_to_i32.o
uwbcore-y += lib/uwb_softfloat/src/f32_to_i64.o
uwbcore-y += lib/uwb_softfloat/src/f32_to_ui32_r_minMag.o
uwbcore-y += lib/uwb_softfloat/src/f32_to_ui64_r_minMag.o
uwbcore-y += lib/uwb_softfloat/src/f32_to_i32_r_minMag.o
uwbcore-y += lib/uwb_softfloat/src/f32_to_i64_r_minMag.o
uwbcore-y += lib/uwb_softfloat/src/f32_to_f64.o
uwbcore-y += lib/uwb_softfloat/src/f32_to_f128M.o
uwbcore-y += lib/uwb_softfloat/src/f32_roundToInt.o
uwbcore-y += lib/uwb_softfloat/src/f32_add.o
uwbcore-y += lib/uwb_softfloat/src/f32_sub.o
uwbcore-y += lib/uwb_softfloat/src/f32_mul.o
uwbcore-y += lib/uwb_softfloat/src/f32_mulAdd.o
uwbcore-y += lib/uwb_softfloat/src/f32_div.o
uwbcore-y += lib/uwb_softfloat/src/f32_rem.o
uwbcore-y += lib/uwb_softfloat/src/f32_sqrt.o
uwbcore-y += lib/uwb_softfloat/src/f32_eq.o
uwbcore-y += lib/uwb_softfloat/src/f32_le.o
uwbcore-y += lib/uwb_softfloat/src/f32_lt.o
uwbcore-y += lib/uwb_softfloat/src/f32_eq_signaling.o
uwbcore-y += lib/uwb_softfloat/src/f32_le_quiet.o
uwbcore-y += lib/uwb_softfloat/src/f32_lt_quiet.o
uwbcore-y += lib/uwb_softfloat/src/f32_isSignalingNaN.o
uwbcore-y += lib/uwb_softfloat/src/f64_to_ui32.o
uwbcore-y += lib/uwb_softfloat/src/f64_to_ui64.o
uwbcore-y += lib/uwb_softfloat/src/f64_to_i32.o
uwbcore-y += lib/uwb_softfloat/src/f64_to_i64.o
uwbcore-y += lib/uwb_softfloat/src/f64_to_ui32_r_minMag.o
uwbcore-y += lib/uwb_softfloat/src/f64_to_ui64_r_minMag.o
uwbcore-y += lib/uwb_softfloat/src/f64_to_i32_r_minMag.o
uwbcore-y += lib/uwb_softfloat/src/f64_to_i64_r_minMag.o
uwbcore-y += lib/uwb_softfloat/src/f64_to_f32.o
uwbcore-y += lib/uwb_softfloat/src/f64_to_f128M.o
uwbcore-y += lib/uwb_softfloat/src/f64_roundToInt.o
uwbcore-y += lib/uwb_softfloat/src/f64_add.o
uwbcore-y += lib/uwb_softfloat/src/f64_sub.o
uwbcore-y += lib/uwb_softfloat/src/f64_mul.o
uwbcore-y += lib/uwb_softfloat/src/f64_mulAdd.o
uwbcore-y += lib/uwb_softfloat/src/f64_div.o
uwbcore-y += lib/uwb_softfloat/src/f64_rem.o
uwbcore-y += lib/uwb_softfloat/src/f64_sqrt.o
uwbcore-y += lib/uwb_softfloat/src/f64_eq.o
uwbcore-y += lib/uwb_softfloat/src/f64_le.o
uwbcore-y += lib/uwb_softfloat/src/f64_lt.o
uwbcore-y += lib/uwb_softfloat/src/f64_eq_signaling.o
uwbcore-y += lib/uwb_softfloat/src/f64_le_quiet.o
uwbcore-y += lib/uwb_softfloat/src/f64_lt_quiet.o
uwbcore-y += lib/uwb_softfloat/src/f64_isSignalingNaN.o
uwbcore-y += lib/uwb_softfloat/src/f128M_to_ui32.o
uwbcore-y += lib/uwb_softfloat/src/f128M_to_ui64.o
uwbcore-y += lib/uwb_softfloat/src/f128M_to_i32.o
uwbcore-y += lib/uwb_softfloat/src/f128M_to_i64.o
uwbcore-y += lib/uwb_softfloat/src/f128M_to_ui32_r_minMag.o
uwbcore-y += lib/uwb_softfloat/src/f128M_to_ui64_r_minMag.o
uwbcore-y += lib/uwb_softfloat/src/f128M_to_i32_r_minMag.o
uwbcore-y += lib/uwb_softfloat/src/f128M_to_i64_r_minMag.o
uwbcore-y += lib/uwb_softfloat/src/f128M_to_f32.o
uwbcore-y += lib/uwb_softfloat/src/f128M_to_f64.o
uwbcore-y += lib/uwb_softfloat/src/f128M_roundToInt.o
uwbcore-y += lib/uwb_softfloat/src/f128M_add.o
uwbcore-y += lib/uwb_softfloat/src/f128M_sub.o
uwbcore-y += lib/uwb_softfloat/src/f128M_mul.o
uwbcore-y += lib/uwb_softfloat/src/f128M_mulAdd.o
uwbcore-y += lib/uwb_softfloat/src/f128M_div.o
uwbcore-y += lib/uwb_softfloat/src/f128M_rem.o
uwbcore-y += lib/uwb_softfloat/src/f128M_sqrt.o
uwbcore-y += lib/uwb_softfloat/src/f128M_eq.o
uwbcore-y += lib/uwb_softfloat/src/f128M_le.o
uwbcore-y += lib/uwb_softfloat/src/f128M_lt.o
uwbcore-y += lib/uwb_softfloat/src/f128M_eq_signaling.o
uwbcore-y += lib/uwb_softfloat/src/f128M_le_quiet.o
uwbcore-y += lib/uwb_softfloat/src/f128M_lt_quiet.o
uwbcore-y += lib/uwb_softfloat/src/math/s_log10.o
uwbcore-y += lib/uwb_softfloat/src/math/s_atan.o
uwbcore-y += lib/uwb_softfloat/src/math/s_atan2.o
uwbcore-y += lib/uwb_softfloat/src/math/s_asin.o
uwbcore-y += lib/uwb_softfloat/src/math/s_nan.o
uwbcore-y += lib/uwb_softfloat/src/math/s_fmod.o
uwbcore-y += lib/uwb_softfloat/src/math/s_strtod.o


# Separate module for the UWB-hal Layer
obj-$(CONFIG_UWB_HAL)	+= porting/dpl/kernel/uwb_hal/

# Separate module Listener
obj-$(CONFIG_UWB_LISTENER)	+= apps/uwb_listener/

# Separate module Wireshark Listener
obj-$(CONFIG_UWB_WIRE_LISTENER)         += apps/uwb_wire_listener/

# Separate module Transport Test
obj-$(CONFIG_UWB_TP_TEST)	+= apps/uwb_tp_test/

# Separate module Tdoa Tag test
obj-$(CONFIG_UWB_TDOA_SYNC_TAG)	+= apps/tdoa_sync_tag/

