@ECHO OFF
REM BFCPEOPTIONSTART
REM Advanced BAT to EXE Converter www.BatToExeConverter.com
REM BFCPEEXE=C:\Users\nonó\Desktop\bootloader2\program.exe
REM BFCPEICON=
REM BFCPEICONINDEX=-1
REM BFCPEEMBEDDISPLAY=0
REM BFCPEEMBEDDELETE=1
REM BFCPEADMINEXE=0
REM BFCPEINVISEXE=0
REM BFCPEVERINCLUDE=0
REM BFCPEVERVERSION=1.0.0.0
REM BFCPEVERPRODUCT=Product Name
REM BFCPEVERDESC=Product Description
REM BFCPEVERCOMPANY=Your Company
REM BFCPEVERCOPYRIGHT=Copyright Info
REM BFCPEWINDOWCENTER=1
REM BFCPEDISABLEQE=0
REM BFCPEWINDOWHEIGHT=30
REM BFCPEWINDOWWIDTH=120
REM BFCPEWTITLE=Window Title
REM BFCPEOPTIONEND
set target_exe="esptool.exe"


esptool.exe --chip esp32s3 --before=default_reset --after=hard_reset write_flash --flash_mode dio --flash_size detect --flash_freq 80m 0x8000 partition-table.bin 0xe000 otadata_boot_from_ota0.bin 0x10000 grid_fw_1_3_6.bin 0x410000 tinyuf2.bin 0x0 bootloader.bin"


pause
