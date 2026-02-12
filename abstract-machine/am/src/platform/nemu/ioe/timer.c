#include <am.h>
#include <nemu.h>
#if RTC_ADDR != 0xa0000048
#error "RTC_ADDR is wrong!"
#endif

void __am_timer_init()
{
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime)
{
  // 读取 RTC 设备（64位寄存器，分两次读取）
  uint32_t low = inl(RTC_ADDR);      // 读低 32 位
  uint32_t high = inl(RTC_ADDR + 4); // 读高 32 位

  // 拼接成 64 位微秒数
  uptime->us = ((uint64_t)high << 32) | (uint64_t)low;
}
void __am_timer_rtc(AM_TIMER_RTC_T *rtc)
{
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour = 0;
  rtc->day = 0;
  rtc->month = 0;
  rtc->year = 1900;
}
