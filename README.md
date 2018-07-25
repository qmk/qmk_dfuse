# QMKDfuSe

A small command-line tool for flashing STM32 based devices for **Windows**. It's based on 
[this](https://www.st.com/en/development-tools/stsw-stm32080.html) demo project created by STM.
The latest binary version can be downloaded from the GitHub releases tab

```
Usage:
  qmk-dfuse [OPTION...]

  -h, --help              Print this help message
  -u, --upload <file>     Read firmware from device into <file>
  -d, --download <file>   Write firmware from <file> into device
  -r, --restart           Restart the device
  -D, --device <devpath>  Device path
  -v, --verify            Verify after download
  ```
  
  If you need to compile or modify the sources, then a Visual Studio 2017 solution is provided and contain several different projecs.
  The interesting one is the modified command-line project QMKDfuSe.
  
  Check SLA0044.txt for license information
