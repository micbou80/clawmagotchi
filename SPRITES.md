# 🎨 ClawMagotchi Sprites & Icons

Reference documentation for the 16×16 1-bit icon system used in ClawMagotchi.

---

## Overview

ClawMagotchi uses a minimal sprite system optimized for embedded hardware:

- **Size**: 16×16 pixels per icon
- **Color depth**: 1-bit (monochrome — pixels are either on or off)
- **Storage**: PROGMEM (Flash memory, doesn't consume RAM)
- **Per icon**: 32 bytes
- **Total system**: 16 icons × 32 bytes = **512 bytes Flash, 0 bytes RAM**

The icons are drawn with a user-specified color at runtime, making each icon reusable in different contexts (blue for email, orange for warnings, etc.).

---

## The 16 Icons

### ✉️ ICON_ENVELOPE — Email

```
                
                
 ▓▓▓▓▓▓▓▓▓▓▓▓▓ 
 ▓▓▓▓▓▓▓▓▓▓▓▓▓ 
 ▓▓▓      ▓▓▓  
 ▓▓▓▓    ▓▓▓▓  
 ▓▓ ▓▓  ▓▓ ▓▓  
 ▓▓  ▓▓▓▓  ▓▓  
 ▓▓   ▓▓   ▓▓  
 ▓▓    ▓    ▓▓  
 ▓▓         ▓▓  
 ▓▓         ▓▓  
 ▓▓         ▓▓  
 ▓▓▓▓▓▓▓▓▓▓▓▓▓ 
                
                
```
Rectangle with V-shaped flap. Used in: Email scenario.

---

### 💬 ICON_CHAT — Chat Bubble

```
                
    ▓▓▓▓▓▓▓▓   
   ▓▓▓▓▓▓▓▓▓▓  
  ▓▓▓▓▓▓▓▓▓▓▓▓ 
  ▓▓        ▓▓  
  ▓▓        ▓▓  
  ▓▓        ▓▓  
  ▓▓        ▓▓  
  ▓▓▓▓▓▓▓▓▓▓▓▓ 
   ▓▓▓▓▓▓▓▓▓▓  
    ▓▓▓▓▓▓▓▓   
     ▓▓         
     ▓▓▓        
      ▓▓▓       
       ▓        
                
```
Rounded rectangle with pointed tail (lower-left). Used in: Teams Message scenario.

---

### 🕒 ICON_CLOCK — Clock

```
                
      ▓▓▓▓▓     
   ▓▓▓▓▓▓▓▓▓   
  ▓▓▓      ▓▓▓  
  ▓▓        ▓▓  
 ▓▓   ▓▓   ▓▓  
 ▓▓   ▓▓   ▓▓  
 ▓▓   ▓▓   ▓▓  
 ▓▓   ▓▓▓▓▓▓▓  
 ▓▓   ▓▓▓▓▓▓▓  
 ▓▓         ▓▓  
  ▓▓        ▓▓  
  ▓▓▓      ▓▓▓  
   ▓▓▓▓▓▓▓▓▓   
      ▓▓▓▓▓     
                
```
Circle with hour and minute hands. Used in: Meeting Soon, Long Session.

---

### ⚠️ ICON_WARNING — Warning Triangle

```
                
       ▓▓       
       ▓▓       
      ▓▓▓▓      
      ▓▓▓▓      
     ▓▓  ▓▓     
     ▓▓  ▓▓     
    ▓▓    ▓▓    
    ▓▓ ▓▓ ▓▓    
   ▓▓  ▓▓  ▓▓   
   ▓▓  ▓▓  ▓▓   
  ▓▓        ▓▓  
  ▓▓   ▓▓   ▓▓  
 ▓▓          ▓▓ 
 ▓▓▓▓▓▓▓▓▓▓▓▓▓ 
                
```
Triangle with exclamation mark inside. Used in: Late, Context Switch, Stuck.

---

### ✅ ICON_CHECK — Checkmark

```
                
                
             ▓▓ 
            ▓▓▓ 
           ▓▓▓  
          ▓▓▓   
         ▓▓▓    
        ▓▓▓     
 ▓▓    ▓▓▓      
 ▓▓▓  ▓▓▓       
  ▓▓▓▓▓▓        
   ▓▓▓▓         
    ▓▓           
                
                
                
```
Bold checkmark. Used in: Done scenario.

---

### ❓ ICON_QUESTION — Question Mark

```
                
      ▓▓▓▓▓▓    
    ▓▓▓▓▓▓▓▓▓   
   ▓▓▓    ▓▓▓   
   ▓▓      ▓▓   
         ▓▓▓    
        ▓▓▓     
       ▓▓▓      
      ▓▓▓       
      ▓▓        
      ▓▓        
                
      ▓▓        
      ▓▓        
                
                
```
Classic question mark. Used in: Task Nudge, Needs Input.

---

### 💧 ICON_DROPLET — Water Drop

```
                
       ▓▓       
       ▓▓       
      ▓▓▓▓      
      ▓▓▓▓      
     ▓▓▓▓▓▓     
     ▓▓▓▓▓▓     
    ▓▓▓▓▓▓▓▓    
   ▓▓▓▓▓▓▓▓▓▓   
   ▓▓▓▓▓▓▓▓▓▓   
   ▓▓▓▓▓▓▓▓▓▓   
   ▓▓▓▓▓▓▓▓▓▓   
    ▓▓▓▓▓▓▓▓    
     ▓▓▓▓▓▓     
      ▓▓▓▓      
                
```
Teardrop/water droplet shape. Used in: Hydrate scenario.

---

### 🏃 ICON_RUNNER — Running Figure

```
                
      ▓▓▓       
      ▓▓▓       
      ▓▓▓       
       ▓        
     ▓▓▓▓▓▓     
    ▓▓▓▓▓▓▓▓    
   ▓▓  ▓        
       ▓        
       ▓        
      ▓▓▓       
     ▓▓ ▓▓      
    ▓▓   ▓▓     
   ▓▓     ▓▓    
   ▓       ▓    
                
```
Stick figure in running pose. Used in: Move scenario.

---

### 🛡️ ICON_SHIELD — Shield

```
                
   ▓▓▓▓▓▓▓▓▓▓   
  ▓▓▓▓▓▓▓▓▓▓▓▓  
 ▓▓▓▓▓▓▓▓▓▓▓▓▓▓ 
 ▓▓▓▓▓▓▓▓▓▓▓▓▓▓ 
 ▓▓▓▓▓▓▓▓▓▓▓▓▓▓ 
 ▓▓▓▓▓▓▓▓▓▓▓▓▓▓ 
 ▓▓▓▓▓▓▓▓▓▓▓▓▓▓ 
 ▓▓▓▓▓▓▓▓▓▓▓▓▓▓ 
  ▓▓▓▓▓▓▓▓▓▓▓▓  
  ▓▓▓▓▓▓▓▓▓▓▓▓  
   ▓▓▓▓▓▓▓▓▓▓   
    ▓▓▓▓▓▓▓▓    
     ▓▓▓▓▓▓     
      ▓▓▓▓      
       ▓▓       
```
Classic shield silhouette. Used in: Focus scenario.

---

### 🌅 ICON_SUNRISE — Sunrise

```
       ▓▓       
       ▓▓       
    ▓    ▓      
     ▓    ▓     
       ▓▓       
  ▓    ▓▓   ▓   
   ▓    ▓    ▓  
                
     ▓▓▓▓▓▓     
   ▓▓▓▓▓▓▓▓▓▓   
  ▓▓▓▓▓▓▓▓▓▓▓▓  
  ▓▓▓▓▓▓▓▓▓▓▓▓  
 ▓▓▓▓▓▓▓▓▓▓▓▓▓▓ 
 ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
 ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
                
```
Half sun rising above horizon with rays. Used in: Morning scenario.

---

### 🌙 ICON_MOON — Crescent Moon

```
                
      ▓▓▓▓▓     
    ▓▓▓▓▓▓      
   ▓▓▓▓▓        
  ▓▓▓▓▓▓        
  ▓▓▓▓▓         
 ▓▓▓▓▓▓         
 ▓▓▓▓▓▓         
 ▓▓▓▓▓▓         
 ▓▓▓▓▓▓         
  ▓▓▓▓▓         
  ▓▓▓▓▓▓        
   ▓▓▓▓▓        
    ▓▓▓▓▓▓      
      ▓▓▓▓▓     
                
```
Crescent moon shape. Used in: Shutdown scenario.

---

### ⚙️ ICON_GEAR — Gear

```
      ▓▓▓▓      
      ▓▓▓▓      
    ▓▓▓▓▓▓▓▓    
    ▓▓▓▓▓▓▓▓    
  ▓▓▓▓▓▓▓▓▓▓▓▓  
  ▓▓▓▓▓▓▓▓▓▓▓▓  
 ▓▓▓▓▓    ▓▓▓▓▓▓
 ▓▓▓▓      ▓▓▓▓▓
 ▓▓▓▓      ▓▓▓▓▓
 ▓▓▓▓▓    ▓▓▓▓▓▓
  ▓▓▓▓▓▓▓▓▓▓▓▓  
  ▓▓▓▓▓▓▓▓▓▓▓▓  
    ▓▓▓▓▓▓▓▓    
    ▓▓▓▓▓▓▓▓    
      ▓▓▓▓      
      ▓▓▓▓      
```
Gear/cog with central hole and teeth. Used in: Working scenario.

---

### ⚖️ ICON_SCALES — Balance Scales

```
       ▓▓       
       ▓▓       
 ▓▓▓▓▓▓▓▓▓▓▓▓▓▓
 ▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  ▓    ▓▓    ▓  
   ▓   ▓▓   ▓   
    ▓  ▓▓  ▓    
    ▓  ▓▓  ▓    
    ▓▓ ▓▓ ▓▓    
    ▓▓ ▓▓ ▓▓    
   ▓▓▓    ▓▓▓   
   ▓▓▓▓▓▓▓▓▓▓   
                
       ▓▓       
       ▓▓       
     ▓▓▓▓▓▓     
```
Balance/scales with pans and central beam. Used in: Decision scenario.

---

### 🎉 ICON_STAR — Star

```
       ▓▓       
       ▓▓       
      ▓▓▓▓      
      ▓▓▓▓      
 ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  ▓▓▓▓▓▓▓▓▓▓▓▓▓ 
   ▓▓▓▓▓▓▓▓▓▓▓  
    ▓▓▓▓▓▓▓▓    
    ▓▓▓▓▓▓▓▓    
   ▓▓▓▓▓▓▓▓▓▓   
   ▓▓▓    ▓▓▓   
  ▓▓▓      ▓▓▓  
  ▓▓        ▓▓  
 ▓▓          ▓▓ 
 ▓            ▓ 
                
```
5-pointed star shape. Used in: Celebration scenario.

---

### ↩️ ICON_REPLY — Reply Arrow

```
                
                
     ▓           
    ▓▓           
   ▓▓▓▓▓▓▓▓▓▓▓▓ 
  ▓▓▓▓▓▓▓▓▓▓▓▓▓ 
 ▓▓▓▓▓▓▓▓▓▓▓▓▓▓ 
  ▓▓▓▓      ▓▓  
   ▓▓▓      ▓▓  
    ▓▓      ▓▓  
     ▓     ▓▓▓  
          ▓▓▓   
         ▓▓▓    
                
                
                
```
Curved arrow pointing left (reply symbol). Used in: Worth Reply scenario.

---

### 👀 ICON_EYE — Eye

```
                
                
                
      ▓▓▓▓▓▓    
   ▓▓▓▓▓▓▓▓▓▓   
  ▓▓▓▓▓▓▓▓▓▓▓▓  
 ▓▓▓▓     ▓▓▓▓  
 ▓▓▓  ▓▓▓▓  ▓▓  
 ▓▓▓  ▓▓▓▓  ▓▓  
 ▓▓▓▓     ▓▓▓▓  
  ▓▓▓▓▓▓▓▓▓▓▓▓  
   ▓▓▓▓▓▓▓▓▓▓   
      ▓▓▓▓▓▓    
                
                
                
```
Eye with iris/pupil. Used in: Quiet Too Long, Doomscrolling.

---

## The 1-Bit Bitmap Format

### Structure

Each icon is a `const uint8_t[]` array stored in PROGMEM:

```c
static const uint8_t ICON_NAME[] PROGMEM = {
  0xHH, 0xHH,   // row 0  (2 bytes = 16 bits = 16 pixels)
  0xHH, 0xHH,   // row 1
  // ... 14 more rows ...
  0xHH, 0xHH,   // row 15
};
```

### Byte Layout

```
Row N:  byte[N*2]         byte[N*2 + 1]
        ┌─┬─┬─┬─┬─┬─┬─┬─┐ ┌─┬─┬─┬─┬─┬─┬─┬─┐
Bits:   │7│6│5│4│3│2│1│0│ │7│6│5│4│3│2│1│0│
        └─┴─┴─┴─┴─┴─┴─┴─┘ └─┴─┴─┴─┴─┴─┴─┴─┘
Pixels: 0 1 2 3 4 5 6 7   8 9 10 11 12 13 14 15
```

- **MSB-first**: Bit 7 of the first byte = leftmost pixel (column 0)
- **1 = pixel on**, 0 = pixel off (transparent)
- 2 bytes per row × 16 rows = 32 bytes total

### Example: Simple Square

A filled 4×4 square centered in the 16×16 grid:

```
Row  0: 0x00, 0x00  →  ................
Row  1: 0x00, 0x00  →  ................
...
Row  6: 0x03, 0xC0  →  ......████......
Row  7: 0x03, 0xC0  →  ......████......
Row  8: 0x03, 0xC0  →  ......████......
Row  9: 0x03, 0xC0  →  ......████......
...
Row 15: 0x00, 0x00  →  ................
```

---

## How PROGMEM Works on ESP32

### The Problem

ESP32 has limited RAM (~320KB SRAM) but abundant Flash (4MB). Without PROGMEM, constant arrays consume RAM at startup.

### The Solution

```c
static const uint8_t ICON_NAME[] PROGMEM = { ... };
```

`PROGMEM` tells the compiler to keep the data in Flash memory. To read it at runtime:

```c
uint8_t byte = pgm_read_byte(&ICON_NAME[index]);
```

On ESP32, PROGMEM is technically mapped to the same address space (unlike AVR), but using `pgm_read_byte` ensures code portability and clarity of intent.

### Memory Savings

Without PROGMEM: 16 icons × 32 bytes = 512 bytes **RAM**
With PROGMEM: 16 icons × 32 bytes = 512 bytes **Flash** + 0 bytes RAM

---

## Creating a New 16×16 Icon — Step by Step

### Step 1: Design on Grid Paper

Draw your icon on a 16×16 grid. Each cell is either filled (1) or empty (0).

Tips:
- Keep it simple — 16×16 is tiny
- Use clear silhouettes, not fine detail
- Test readability at actual size (about 4mm on the 172px display)
- Bold shapes work better than thin lines

### Step 2: Convert to Binary

For each row, write the 16 pixels as bits:

```
Row 0: _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ → 0b00000000 0b00000000
Row 1: _ _ _ _ _ _ _ ▓ ▓ _ _ _ _ _ _ _ → 0b00000001 0b10000000
...
```

### Step 3: Convert to Hex

Convert each 8-bit binary to hex:
- `0b00000001` → `0x01`
- `0b10000000` → `0x80`

### Step 4: Write the Array

```c
// 🆕 My Icon — description of what it looks like
static const uint8_t ICON_MY_ICON[] PROGMEM = {
  0x00,0x00, // row 0
  0x01,0x80, // row 1
  // ... rows 2-14 ...
  0x00,0x00, // row 15
};
```

### Step 5: Add to sprites.h

Place the array in `sprites.h` with the other icons.

### Step 6: Use It

```c
// Direct draw (transparent background)
drawIcon16(x, y, ICON_MY_ICON, 0xFFFF); // white icon

// Badge style (with colored circle background)
drawIconBadge(x, y, ICON_MY_ICON, 0xFFFF, 0x07E0); // white on green
```

### Automation Alternative

Use an image editor (GIMP, Aseprite, Piskel) to draw 16×16 monochrome:
1. Export as raw binary or `.pbm` (portable bitmap)
2. Convert bytes to C array format
3. Paste into `sprites.h`

Online tools like [image2cpp](https://javl.github.io/image2cpp/) can convert images directly to Arduino-compatible byte arrays.

---

## Memory Efficiency Comparison

| Format | Bits/pixel | Size per 16×16 icon | 16 icons | Notes |
|--------|-----------|---------------------|----------|-------|
| **1-bit PROGMEM** (ours) | 1 | 32 B | 512 B | Flash only, no RAM |
| 8-bit grayscale | 8 | 256 B | 4 KB | Needs palette |
| RGB565 (16-bit) | 16 | 512 B | 8 KB | Full color |
| RGBA8888 (32-bit) | 32 | 1,024 B | 16 KB | Overkill for embedded |

Our approach uses **97% less memory** than full-color sprites while still looking great on a small TFT (where you can't see individual pixel colors at icon-size anyway).

---

## Drawing Functions Reference

### `drawIcon16(x, y, icon, color)`

Draws a 16×16 1-bit icon at screen position (x, y) with the given RGB565 color. Only "on" pixels are drawn — the background shows through.

```c
inline void drawIcon16(int x, int y, const uint8_t* icon, uint16_t color) {
  for (int row = 0; row < 16; row++) {
    uint8_t b0 = pgm_read_byte(&icon[row * 2]);
    uint8_t b1 = pgm_read_byte(&icon[row * 2 + 1]);
    uint16_t rowBits = ((uint16_t)b0 << 8) | b1;
    for (int col = 0; col < 16; col++) {
      if (rowBits & (0x8000 >> col)) {
        tft.drawPixel(x + col, y + row, color);
      }
    }
  }
}
```

**Performance**: 16 rows × 2 `pgm_read_byte` + up to 256 `drawPixel` calls. Fast enough at 12fps for badges.

### `drawIconBadge(x, y, icon, iconColor, bgColor)`

Draws a filled circle as background, then the icon on top. Creates a notification-badge aesthetic.

```c
inline void drawIconBadge(int x, int y, const uint8_t* icon, uint16_t iconColor, uint16_t bgColor) {
  int cx = x + 8;   // center of 16×16 area
  int cy = y + 8;
  tft.fillCircle(cx, cy, 10, bgColor);  // 20px diameter circle
  drawIcon16(x, y, icon, iconColor);
}
```

---

## Color Reference (RGB565)

Common colors used with icons:

| Name | RGB565 | Used As |
|------|--------|---------|
| White | `0xFFFF` | Default icon color on dark backgrounds |
| Green | `0x07E0` | Positive/success (Focus, Done, Celebration) |
| Blue | `0x2C9F` | Notifications (Email, Meeting) |
| Purple | `0x541F` | Teams |
| Orange | `0xFD20` | Warnings (Late, Long Session) |
| Gold | `0xFE8C` | Gentle nudges (Task, Hydrate, Morning) |
| Red | `0xE1C8` | Urgent |
| Gray | `0x7BEF` | Passive (Idle, Shutdown, Quiet) |

---

*Sprite docs maintained by Red 🦞*
