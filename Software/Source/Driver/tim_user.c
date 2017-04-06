/**
******************************************************************************
* @file    tim_user.c
* @author  Inmotion NaviPanel team
* @date    2016/09/14
* @brief   定时器相关驱动
* @attention Copyright (C) 2016 Inmotion Corporation
******************************************************************************
*/

#include "tim_user.h"
#include "math_lib.h"
#include "speed_loop.h"
#include "global_defines.h"
#include "motion_control.h"

#define TICK_FREQ   1000
#define BasicTimer  (TIM7)

#define FREQ(cnt,hz)  (++(cnt) >= BASIC_TIM_FREQ/(hz))

static u32 BasicTimOverflowCount = 0;
RunFlagType RunFlag;

#define FREQ_FLAG(t, freq) do{\
    if(tick - (t) >= (TICK_FREQ/(freq))) { \
        (t) += (TICK_FREQ/(freq)); \
        (RunFlag.##t) = 1; } \
}while(0)

u32 US_FREQ = 40;

void RunFlagHandler(void)
{
    static u32 led = 0;
    static u32 _100Hz = 0;
    static u32 second = 0;
    static u32 imu = 0;
    static u32 motion = 0;
    static u32 us = 0;
    static u32 contact=0;
    u32 tick = HAL_GetTick();
    
    if(tick - _100Hz >= TICK_FREQ/100)
    {
        _100Hz += TICK_FREQ/100;
        RunFlag.supervise = 1;
        RunFlag.boost = 1;
    }

    FREQ_FLAG(imu, IMU_UPDATE_FREQ);
    FREQ_FLAG(led, 2);
    FREQ_FLAG(contact, 500);
    FREQ_FLAG(second, 1);
    FREQ_FLAG(motion, MOTION_PREQ);
    FREQ_FLAG(us, US_FREQ);
}

u32 GetIntervalCnt(u32 *last_capture_cnt, u32 *last_cnt)
{
    u32 plusecnt;
    u32 capture_1, capture_2;
    u16 tim_cnt_1, tim_cnt_2;
    
    capture_1 = BasicTimOverflowCount;
    tim_cnt_1 = BasicTimer->CNT;
    capture_2 = BasicTimOverflowCount;
    tim_cnt_2 = BasicTimer->CNT;

    if(capture_1 != capture_2)
    {
        capture_1 = capture_2;
        tim_cnt_1 = tim_cnt_2;
    }

    plusecnt = MINUS_S32(capture_1, *last_capture_cnt) * (BASIC_TIM_PERIOD+1) + tim_cnt_1 - *last_cnt;
    
    *last_cnt = tim_cnt_1;
    *last_capture_cnt = capture_1;
    
    return plusecnt;
}

u32 GetIntervalTimeUs(u32 *last_capture_cnt, u32 *last_cnt)
{
    return (u64)(GetIntervalCnt(last_capture_cnt, last_cnt)) * 1000000 / BASIC_TIM_CNT_FREQ;
}

/**
* @brief  Basic TIM 中断处理函数
* @param  curve: 曲线参数
* @retval None
*/
void BasicTIM_IRQHandler(TIM_HandleTypeDef *htim)
{
	static u32 cnt2 = 0;
    
    s32 encoder_delta;
    
    BasicTimOverflowCount++;
    
    if(FREQ(cnt2, MOTION_PREQ))
    {
        cnt2 = 0;
        SpeedLoop();
    }
}
