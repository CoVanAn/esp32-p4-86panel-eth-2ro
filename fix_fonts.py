import os
import glob

files = glob.glob('/media/covanan/data/Wokr/KisGroup/ESP32-P4-86PANEL-ETH-2RO/Arduino/libraries/lvgl/src/font/lv_font_*.c')
files.extend(glob.glob('/home/covanan/Documents/PlatformIO/Projects/PlatformIO/src/fonts/ui_font_*.c'))

for f in files:
    with open(f, 'r') as file:
        content = file.read()
    
    # We want to make sure it always defines it as const lv_font_t
    # so we replace `lv_font_t lv_font_` with `const lv_font_t lv_font_`
    # and also `#if LV_VERSION_CHECK` can be bypassed by just making it const anyway.
    content = content.replace('\nlv_font_t ', '\nconst lv_font_t ')
    
    with open(f, 'w') as file:
        file.write(content)

print(f"Fixed {len(files)} font files.")
