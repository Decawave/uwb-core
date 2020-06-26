#ifndef _UWBCORE_PRIV_H_
#define _UWBCORE_PRIV_H_

#ifdef __cplusplus
extern "C" {
#endif

int uwbcore_hal_bsp_dw1000_init(int spi_num, uint32_t devid, struct dpl_sem *sem);
int uwbcore_hal_bsp_dw3000_init(int spi_num, uint32_t devid, struct dpl_sem *sem);

#ifdef __cplusplus
}
#endif

#endif
