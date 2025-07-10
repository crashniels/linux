#ifndef _BTIF_AUDIO_TS_H_
#define _BTIF_AUDIO_TS_H_

#include <linux/types.h>

extern u64 mtk_timer_get_cnt(u8 timer);

/* Report audio timestamp and timer value corresponding to it */
void btif_update_audio_ts(unsigned int audio_ts, u64 ts);

/* Driver registration/deregistration */
int btif_ts_init(void);
void btif_ts_exit(void);

#endif /* _BTIF_AUDIO_TS_H_ */

