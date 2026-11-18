name: Build GBA
on:
  push:
  workflow_dispatch:
jobs:
  build:
    runs-on: ubuntu-latest
    container: devkitpro/devkitarm
    steps:
      - uses: actions/checkout@v4
      - name: Compile
        run: |
          export PATH=/opt/devkitpro/devkitARM/bin:/opt/devkitpro/tools/bin:$PATH
          arm-none-eabi-gcc -mthumb -mthumb-interwork -O2 -specs=gba.specs main.c -o laeken.elf
          arm-none-eabi-objcopy -O binary laeken.elf laeken.gba
          gbafix laeken.gba
      - uses: actions/upload-artifact@v4
        with:
          name: laeken-gba
          path: laeken.gba
