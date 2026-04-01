# CLAUDE.md - Project Context for Claude Code

## Project Overview

**DIAMANT v2.1 REV F** - Smart lighting and solar power management system by D-SLS SRL (Milan).

- **MCU**: ESP32-C6-MINI-1-N4 (RISC-V 160MHz, WiFi6, BLE5, Thread/Matter)
- **Power**: Dual LT8708 bidirectional converters + Dual LT8391 LED drivers (400W total)
- **Battery**: BQ76952 BMS supporting LiFePO4/NMC/NCA (3-16S)
- **Lighting**: Warm 2700K + Cool 5000K LED channels, DALI-2 DT8 protocol
- **Connectivity**: WiFi6, BLE5, 802.15.4 Thread, USB-C native CDC-ACM
- **BOM**: ~$121

## Repository Structure

- `DIAMANT_v2.1_REV_F_SPECIFICATION.txt` - Main hardware specification (GPIO routing, power topology, BOM, PCB layout)
- `.github/agents/` - GitHub Copilot custom agents

## Key Technical Details

### GPIO Allocation (23 GPIOs)
- ADC: IO0 (DIM_SENSE), IO1 (DIM_AC), IO2 (V_BATT), IO23 (DIP_CFG)
- SPI: IO4/5/6 (CLK/MISO/MOSI), IO22 (CS_BMS), IO15 (CS_OLED)
- I2C: IO20 (SDA), IO21 (SCL) - 8 devices at 400kHz
- UART1: IO9 (DALI_TX), IO11 (DALI_RX) - MAX22513 DALI-2
- USB: IO12/IO13 (D-/D+)
- PWM: IO18 (LED_W), IO19 (LED_C), IO10 (WS2812B)
- GPIO: IO3 (BMS_ALERT), IO7 (FAN_EN), IO8 (RELAY), IO14 (PIR_IN), IO17 (AUX_SW)

### Power Architecture
- PV input: up to 80V, 15A MC4 connectors
- LT8708 #1: MPPT PV-to-Battery, 97% efficiency
- LT8708 #2: Bidirectional BUS/V2G, 500W
- V_BUS rail: 24-60V

## Guidelines for Claude Code

- This is a hardware documentation repository, not a software project
- When reviewing or modifying the specification, preserve the ASCII diagram formatting
- Validate GPIO pin assignments against ESP32-C6 datasheet constraints
- Ensure no GPIO conflicts when suggesting changes
- BOM cost changes should be noted when modifying component selections
