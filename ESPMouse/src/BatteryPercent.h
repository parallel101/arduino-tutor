#pragma once

#include <Arduino.h>

inline int batteryVoltageToPercentage(int voltage_mV)
{
  // 锂电池电压与电量对应关系（基于典型放电曲线）
  // 使用分段线性逼近法
  
  // 电压超出范围时的保护
  if (voltage_mV >= 4200) return 100;
  if (voltage_mV <= 3200) return 0;
  
  // 分段计算电量百分比
  if (voltage_mV >= 4100) {
    // 100% ~ 90%: 4.2V ~ 4.1V
    return map(voltage_mV, 4100, 4200, 90, 100);
  } else if (voltage_mV >= 4000) {
    // 90% ~ 70%: 4.1V ~ 4.0V
    return map(voltage_mV, 4000, 4100, 70, 90);
  } else if (voltage_mV >= 3900) {
    // 70% ~ 40%: 4.0V ~ 3.9V
    return map(voltage_mV, 3900, 4000, 40, 70);
  } else if (voltage_mV >= 3800) {
    // 40% ~ 20%: 3.9V ~ 3.8V
    return map(voltage_mV, 3800, 3900, 20, 40);
  } else if (voltage_mV >= 3700) {
    // 20% ~ 10%: 3.8V ~ 3.7V
    return map(voltage_mV, 3700, 3800, 10, 20);
  } else if (voltage_mV >= 3600) {
    // 10% ~ 5%: 3.7V ~ 3.6V
    return map(voltage_mV, 3600, 3700, 5, 10);
  } else {
    // 5% ~ 0%: 3.6V ~ 3.2V
    return map(voltage_mV, 3200, 3600, 0, 5);
  }
}
