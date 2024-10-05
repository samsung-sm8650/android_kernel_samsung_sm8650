/* drivers/input/sec_input/stm/stm_dump.c
 *
 * Copyright (C) 2011 Samsung Electronics Co., Ltd.
 *
 * Core file for Samsung TSC driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "stm_dev.h"
#include "stm_reg.h"

#if IS_ENABLED(CONFIG_TOUCHSCREEN_DUMP_MODE)
void stm_ts_check_rawdata(struct work_struct *work)
{
	struct stm_ts_data *ts = container_of(work, struct stm_ts_data, check_rawdata.work);

	if (ts->tsp_dump_lock == 1) {
		input_err(true, ts->dev, "%s: ignored ## already checking..\n", __func__);
		return;
	}
	if (atomic_read(&ts->plat_data->power_state) == SEC_INPUT_STATE_POWER_OFF) {
		input_err(true, ts->dev, "%s: ignored ## IC is power off\n", __func__);
		return;
	}

	stm_ts_run_rawdata_all(ts);
}

void stm_ts_dump_tsp_log(struct device *dev)
{
	struct stm_ts_data *ts = dev_get_drvdata(dev);

	pr_info("%s: %s %s: start\n", STM_TS_I2C_NAME, SECLOG, __func__);

	if (!ts) {
		pr_err("%s: %s %s: ignored ## tsp probe fail!!\n", STM_TS_I2C_NAME, SECLOG, __func__);
		return;
	}

	schedule_delayed_work(&ts->check_rawdata, msecs_to_jiffies(100));
}

void stm_ts_sponge_dump_flush(struct stm_ts_data *ts, int dump_area)
{
	int i, ret;
	unsigned char *sec_spg_dat;
	input_info(true, ts->dev, "%s: ++\n", __func__);

	sec_spg_dat = vmalloc(SEC_TS_MAX_SPONGE_DUMP_BUFFER);
	if (!sec_spg_dat) {
		input_err(true, ts->dev, "%s : Failed!!\n", __func__);
		return;
	}
	memset(sec_spg_dat, 0, SEC_TS_MAX_SPONGE_DUMP_BUFFER);

	input_info(true, ts->dev, "%s: dump area=%d\n", __func__, dump_area);

	/* check dump area */
	if (dump_area == 0) {
		sec_spg_dat[0] = SEC_TS_CMD_SPONGE_LP_DUMP_EVENT;
		sec_spg_dat[1] = 0;
	} else {
		sec_spg_dat[0] = (u8)ts->sponge_dump_border & 0xff;
		sec_spg_dat[1] = (u8)(ts->sponge_dump_border >> 8);
	}

	/* dump all events at once */

	if (ts->sponge_dump_event * ts->sponge_dump_format > SEC_TS_MAX_SPONGE_DUMP_BUFFER) {
		input_err(true, ts->dev, "%s: wrong sponge dump read size (%d)\n",
					__func__, ts->sponge_dump_event * ts->sponge_dump_format);
		vfree(sec_spg_dat);
		return;
	}

	ret = ts->stm_ts_read_sponge(ts, sec_spg_dat, ts->sponge_dump_event * ts->sponge_dump_format);
	if (ret < 0) {
		input_err(true, ts->dev, "%s: Failed to read sponge\n", __func__);
		vfree(sec_spg_dat);
		return;
	}

	for (i = 0 ; i < ts->sponge_dump_event ; i++) {
		int e_offset = i * ts->sponge_dump_format;
		char buff[30] = {0, };
		u16 edata[5];
		edata[0] = (sec_spg_dat[1 + e_offset] & 0xFF) << 8 | (sec_spg_dat[0 + e_offset] & 0xFF);
		edata[1] = (sec_spg_dat[3 + e_offset] & 0xFF) << 8 | (sec_spg_dat[2 + e_offset] & 0xFF);
		edata[2] = (sec_spg_dat[5 + e_offset] & 0xFF) << 8 | (sec_spg_dat[4 + e_offset] & 0xFF);
		edata[3] = (sec_spg_dat[7 + e_offset] & 0xFF) << 8 | (sec_spg_dat[6 + e_offset] & 0xFF);
		edata[4] = (sec_spg_dat[9 + e_offset] & 0xFF) << 8 | (sec_spg_dat[8 + e_offset] & 0xFF);

		if (edata[0] || edata[1] || edata[2] || edata[3] || edata[4]) {
			snprintf(buff, sizeof(buff), "%03d: %04x%04x%04x%04x%04x\n",
					i + (ts->sponge_dump_event * dump_area),
					edata[0], edata[1], edata[2], edata[3], edata[4]);
#if IS_ENABLED(CONFIG_SEC_DEBUG_TSP_LOG)
			sec_tsp_sponge_log(buff);
#endif
		}
	}

	vfree(sec_spg_dat);
	input_info(true, ts->dev, "%s: --\n", __func__);
	return;
}
EXPORT_SYMBOL(stm_ts_sponge_dump_flush);
#endif

MODULE_LICENSE("GPL");

