#!/usr/bin/env python3
"""
🔋 Nova Battery & Power Management System
============================================

Batarya verimliliği ve güç yönetimi için akıllı sistem

Features:
- Batarya durumu izleme
- Dinamik güç profilleri
- Thermal management
- Adaptive frequency scaling

Author: Nova Team
Date: February 7, 2026
"""

import time
import platform
import subprocess
from typing import Dict, Optional
from dataclasses import dataclass
from enum import Enum


class PowerProfile(Enum):
    """Güç profilleri"""
    ECO = "eco"  # Maksimum batarya tasarrufu
    BALANCED = "balanced"  # Dengeli
    PERFORMANCE = "performance"  # Maksimum performans
    ADAPTIVE = "adaptive"  # Akıllı adaptif


@dataclass
class BatteryStatus:
    """Batarya durumu"""
    percentage: float
    is_charging: bool
    time_remaining: Optional[int]  # dakika
    power_consumption: float  # watt
    temperature: Optional[float]  # celsius


@dataclass
class PowerBudget:
    """Güç bütçesi"""
    max_watts: float
    current_usage: float
    available: float
    cpu_allocation: float
    gpu_allocation: float


class BatteryPowerManager:
    """
    Batarya ve Güç Yönetim Sistemi
    
    Akıllı güç yönetimi ile:
    - Batarya ömrünü uzatır
    - Thermal throttling'i engeller
    - Performans/verimlilik dengesini sağlar
    """
    
    def __init__(self, profile: PowerProfile = PowerProfile.ADAPTIVE):
        self.profile = profile
        self.system = platform.system()
        self.battery_history = []
        self.power_events = []
        
        # Güç profil ayarları
        self.profile_settings = {
            PowerProfile.ECO: {
                'max_cpu_freq': 0.5,  # %50 max CPU
                'max_gpu_freq': 0.3,  # %30 max GPU
                'target_temp': 50,     # °C
                'max_power': 15        # watt
            },
            PowerProfile.BALANCED: {
                'max_cpu_freq': 0.75,
                'max_gpu_freq': 0.6,
                'target_temp': 70,
                'max_power': 30
            },
            PowerProfile.PERFORMANCE: {
                'max_cpu_freq': 1.0,
                'max_gpu_freq': 1.0,
                'target_temp': 90,
                'max_power': 100
            },
            PowerProfile.ADAPTIVE: {
                'max_cpu_freq': 1.0,
                'max_gpu_freq': 1.0,
                'target_temp': 80,
                'max_power': 50
            }
        }
    
    def get_battery_status(self) -> Optional[BatteryStatus]:
        """Batarya durumunu al"""
        try:
            if self.system == "Darwin":  # macOS
                return self._get_macos_battery()
            elif self.system == "Linux":
                return self._get_linux_battery()
            elif self.system == "Windows":
                return self._get_windows_battery()
        except Exception as e:
            print(f"⚠️  Batarya durumu alınamadı: {e}")
            return None
        
        return None
    
    def _get_macos_battery(self) -> Optional[BatteryStatus]:
        """macOS batarya durumu"""
        try:
            cmd = "pmset -g batt"
            output = subprocess.check_output(cmd, shell=True).decode()
            
            # Parse percentage
            percentage = 100.0
            is_charging = "charging" in output.lower()
            
            if "%" in output:
                for line in output.split('\n'):
                    if '%' in line:
                        parts = line.split()
                        for part in parts:
                            if '%' in part:
                                percentage = float(part.replace('%', '').replace(';', ''))
            
            return BatteryStatus(
                percentage=percentage,
                is_charging=is_charging,
                time_remaining=None,
                power_consumption=0.0,
                temperature=None
            )
        except Exception as e:
            return None
    
    def _get_linux_battery(self) -> Optional[BatteryStatus]:
        """Linux batarya durumu"""
        try:
            # /sys/class/power_supply/ altını kontrol et
            import os
            battery_path = "/sys/class/power_supply/BAT0/"
            
            if not os.path.exists(battery_path):
                return None
            
            with open(f"{battery_path}capacity", 'r') as f:
                percentage = float(f.read().strip())
            
            with open(f"{battery_path}status", 'r') as f:
                status = f.read().strip()
                is_charging = status == "Charging"
            
            return BatteryStatus(
                percentage=percentage,
                is_charging=is_charging,
                time_remaining=None,
                power_consumption=0.0,
                temperature=None
            )
        except Exception:
            return None
    
    def _get_windows_battery(self) -> Optional[BatteryStatus]:
        """Windows batarya durumu"""
        try:
            import psutil
            battery = psutil.sensors_battery()
            
            if battery:
                return BatteryStatus(
                    percentage=battery.percent,
                    is_charging=battery.power_plugged,
                    time_remaining=battery.secsleft // 60 if battery.secsleft != -1 else None,
                    power_consumption=0.0,
                    temperature=None
                )
        except Exception:
            return None
        
        return None
    
    def calculate_power_budget(self, battery_status: Optional[BatteryStatus] = None) -> PowerBudget:
        """
        Güç bütçesini hesapla
        
        Faktörler:
        - Batarya seviyesi
        - Şarj durumu
        - Profil ayarları
        - Thermal durumu
        """
        if battery_status is None:
            battery_status = self.get_battery_status()
        
        settings = self.profile_settings[self.profile]
        
        # Batarya seviyesine göre ayarlama
        if battery_status and not battery_status.is_charging:
            if battery_status.percentage < 20:
                # Düşük batarya: Agresif tasarruf
                max_watts = settings['max_power'] * 0.3
            elif battery_status.percentage < 50:
                # Orta batarya: Dengeli
                max_watts = settings['max_power'] * 0.6
            else:
                # Yüksek batarya: Normal
                max_watts = settings['max_power']
        else:
            # Şarjda: Tam performans
            max_watts = settings['max_power']
        
        # CPU/GPU dağılımı
        cpu_allocation = max_watts * 0.6  # %60 CPU
        gpu_allocation = max_watts * 0.4  # %40 GPU
        
        return PowerBudget(
            max_watts=max_watts,
            current_usage=0.0,
            available=max_watts,
            cpu_allocation=cpu_allocation,
            gpu_allocation=gpu_allocation
        )
    
    def should_throttle(self, current_power: float, budget: PowerBudget) -> bool:
        """Throttling gerekli mi?"""
        return current_power > budget.max_watts
    
    def get_recommended_profile(self, battery_status: Optional[BatteryStatus] = None) -> PowerProfile:
        """Önerilen profili al"""
        if battery_status is None:
            battery_status = self.get_battery_status()
        
        if battery_status is None:
            return PowerProfile.BALANCED
        
        # Batarya durumuna göre öneri
        if battery_status.is_charging:
            return PowerProfile.PERFORMANCE
        elif battery_status.percentage < 20:
            return PowerProfile.ECO
        elif battery_status.percentage < 50:
            return PowerProfile.BALANCED
        else:
            return PowerProfile.ADAPTIVE
    
    def optimize_for_battery_life(self) -> Dict[str, any]:
        """
        Batarya ömrü için optimize et
        
        Returns:
            Optimizasyon önerileri
        """
        battery = self.get_battery_status()
        budget = self.calculate_power_budget(battery)
        recommended = self.get_recommended_profile(battery)
        
        return {
            'battery_status': battery,
            'power_budget': budget,
            'recommended_profile': recommended.value,
            'current_profile': self.profile.value,
            'should_switch': recommended != self.profile
        }
    
    def print_status(self):
        """Durumu yazdır"""
        battery = self.get_battery_status()
        budget = self.calculate_power_budget(battery)
        
        print("\n" + "=" * 60)
        print("🔋 BATTERY & POWER MANAGEMENT")
        print("=" * 60)
        
        if battery:
            status_icon = "🔌" if battery.is_charging else "🔋"
            print(f"{status_icon} Battery: {battery.percentage:.1f}%")
            print(f"⚡ Charging: {'Yes' if battery.is_charging else 'No'}")
            if battery.time_remaining:
                print(f"⏱️  Time Remaining: {battery.time_remaining} minutes")
        else:
            print("⚠️  Battery status unavailable (desktop system?)")
        
        print(f"\n💡 Current Profile: {self.profile.value.upper()}")
        print(f"📊 Power Budget: {budget.max_watts:.1f}W")
        print(f"   • CPU: {budget.cpu_allocation:.1f}W")
        print(f"   • GPU: {budget.gpu_allocation:.1f}W")
        print("=" * 60)


def main():
    """Test fonksiyonu"""
    print("🔋 Nova Battery & Power Manager")
    
    manager = BatteryPowerManager()
    manager.print_status()
    
    # Optimizasyon önerileri
    optimization = manager.optimize_for_battery_life()
    
    if optimization['should_switch']:
        print(f"\n💡 Öneri: {optimization['recommended_profile'].upper()} profiline geçiş yapabilirsiniz")


if __name__ == "__main__":
    main()
