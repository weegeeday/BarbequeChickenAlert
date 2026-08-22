# Barbeque Chicken Alert (Nintendo DS Port)

A native C++ port of *Barbeque Chicken Alert* for the Nintendo DS built using devkitARM and libnds.

---

## 🎮 Controls

### **Top Screen Navigation**
- **D-Pad Up / Down**: Scroll through available upgrades.
- **D-Pad Left / Right** or **L / R Triggers**: Switch Top Screen tabs (`1:UPG` $\leftrightarrow$ `2:BANK` $\leftrightarrow$ `3:REB` $\leftrightarrow$ `4:STAT`).
- **A Button**: Purchase selected upgrade / execute Rebirth.
- **X Button**: Deposit 10% into Bank (when in Bank tab).
- **Y Button**: Withdraw 10% from Bank (when in Bank tab).
- **SELECT Button**: **Save Game** (saves state across all screens).

### **Bottom Screen Touch**
- **Bouncing Chickens**: Tap 32x32 roasted chicken drumsticks with stylus/mouse to gather chickens!
- **Alert Badge**: Tap the randomized `BCAG.png` alert popup to claim popup rewards.

---

## 🔨 Building the NDS ROM

### Prerequisites
- [devkitPro / devkitARM](https://devkitpro.org/) with libnds installed.
- Python 3 with Pillow (`pip install Pillow`).

### Build Instructions
Run the automated build script in the root directory:

```cmd
build.bat
```

This compiles:
- `BarbequeChickenAlert.nds` (Nintendo DS ROM file)
