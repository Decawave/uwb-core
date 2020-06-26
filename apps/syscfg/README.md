# Toplevel syscfg

## Overview
This project is used to build the default syscfg and sysinit for the cmake build enviorement

```no-highlight
newt target create syscfg
newt target set syscfg app=apps/syscfg
newt target set syscfg bsp=@decawave-uwb-core/hw/bsp/hikey960_dwmX000
newt build syscfg
```
