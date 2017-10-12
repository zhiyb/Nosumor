# Nosumor remake
<sup>(A new project name needed)</sup>

Design a small USB keyboard for playing [osu!](https://osu.ppy.sh/): [Blog posts](https://zhiyb.wordpress.com/category/cc/embedded/nosumor/)

In addition to mandatory keyboard functionalities, USB audio and Micro SD support <sup>work in progress</sup> are also added.

The PCB layout was made compatible with [the original Nosumor v3 casing](http://noodlefighter.com/工作日志/log_nono/), which sells on [the official osu! store](https://osu.ppy.sh/store/product/20).

However, as a hobby project for learning USB framework and achieving high performance, the parts used in this design are probably unnecessarily expensive.

## Features

- USB 2.0 high-speed connection with 8kHz polling rate
- Audio output through headphone jack using USB audio class 2
- Application for dynamic reconfiguration and firmware upgrade

### Planned features

- Audio input (MIC) through headphone jack
- Micro SD card reader

## PCB features

- 50×50 (mm) board area
- 2 mechanical keyboard switches (Cherry MX RGB compatible) and 3 tactile buttons
- Keycap and bottom-side RGB lighting
- STM32F722RE Cortex-M7 high performance MCU @216MHz
- USB 2.0 high-speed PHY USB3370
- 3.5mm audio jack with TLV320AIC3110 audio codec
- On-board FT2232D connected to UART & JTAG for debugging
- Extension UART port with 3.3V power

Gerber files for PCB manufacturer Seeed Studio provided.

## Estimated costs

Configuration | Cost
---|---
Basic keyboard | £20
With audio and Micro SD slot | £30
Also with debugging USB port | £38
