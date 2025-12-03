# BootSel2Reset

A mod chip that adds a reset function to the Raspberry Pi Pico's BOOTSEL switch.

- Click BOOTSEL to reset the Pico
- Hold BOOTSEL to enter mass storage mode
- Selectable hold time: 500ms or 4sec

## Video

https://github.com/user-attachments/assets/045de19f-95cd-49d6-bd9e-31ea8d296164

## Connection

![](./img/connection.png)

## Hold Time Selection

|TIMESEL|Hold Time|
|:--|:--|
|Open|4 sec|
|Connect to GND|500 ms|

## For ATtiny85

### Pin Assign

|Pin|Signal|
|:--:|:--|
|1|n/a|
|2|RUN|
|3|TIMESEL|
|4|GND|
|5|n/a|
|6|n/a|
|7|BOOTSEL|
|8|3V3|

### Fuse

Use the default values ​​for the fuse bytes.

|Byte|Value|
|:--|:--|
|High Byte|0xDF|
|Low Byte|0x62|