
#Build and Usage-Requirements

Author: Daniel Andersen - http://www.Dan-And.de 
Contributors: 


##Intro:

Using the original SINOVOIP Package was quite painful as there is basically no documentation at all. So I tried to align the packages 
as good as possible to get that stuff running until the Allwinner R40 and SINOVOIP BananaPI Ultra M2 board is available in the mainline 
Kernel and mainline U-Boot repository. 


###WARNING: 
There are (at least for me) unknown binaries which you need to use. I haven't found any source code for it and they are provided as compiled 
32-Bit (!) binaries only. 
See: https://github.com/BPI-SINOVOIP/BPI-M2U-bsp/tree/master/out/host/bin 

BPI-M2U-bsp/out/host/bin# file *
AwPluginVector.dll: ELF 32-bit LSB shared object, Intel 80386, version 1 (SYSV), dynamically linked, not stripped
config.dll:         PE32 executable (DLL) (GUI) Intel 80386, for MS Windows
dragon:             ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), dynamically linked, interpreter /lib/ld-linux.so.2, for GNU/Linux 2.6.24, BuildID[sha1]=694c09489e8ca9a2ab6fa3168cebe93697eded28, not stripped
dragonsecboot:      ELF 64-bit LSB executable, x86-64, version 1 (GNU/Linux), statically linked, for GNU/Linux 2.6.24, BuildID[sha1]=01cf8aba45296ed37a8ff8415724922ad6c091fe, not stripped
fsbuild:            ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), dynamically linked, interpreter /lib/ld-linux.so.2, for GNU/Linux 2.6.9, not stripped
FSProcess.dll:      PE32 executable (DLL) (GUI) Intel 80386, for MS Windows
fstool.dll:         ELF 32-bit LSB shared object, Intel 80386, version 1 (SYSV), dynamically linked, not stripped
ImageBuilder.dll:   ELF 32-bit LSB shared object, Intel 80386, version 1 (SYSV), dynamically linked, BuildID[sha1]=26ef821d21ee90c3c5c046e94d205309bcb47dff, not stripped
IniParasPlg.dll:    ELF 32-bit LSB shared object, Intel 80386, version 1 (SYSV), dynamically linked, not stripped
IniParasPlgex.dll:  ELF 32-bit LSB shared object, Intel 80386, version 1 (SYSV), dynamically linked, BuildID[sha1]=a86726716141bfd5ccb4e6e7a21f8fbc48afb54b, not stripped
plgvector.dll:      ELF 32-bit LSB shared object, Intel 80386, version 1 (SYSV), dynamically linked, BuildID[sha1]=e23d4679cc40924b30f7916f3fd2345b2ce85b60, not stripped
RAMPart.dll:        PE32 executable (DLL) (GUI) Intel 80386, for MS Windows
script:             ELF 64-bit LSB executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, for GNU/Linux 2.6.15, BuildID[sha1]=d9de39e5335569a9b98b879b84c62811a43177b1, not stripped
ScriptParser.dll:   PE32 executable (DLL) (GUI) Intel 80386, for MS Windows
u_boot_env_gen:     ELF 64-bit LSB executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, for GNU/Linux 2.6.15, BuildID[sha1]=7bc95164072c8c83c55138507a73dadda21509e7, stripped
update_boot0:       ELF 64-bit LSB executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, for GNU/Linux 2.6.24, BuildID[sha1]=345304d3467c455b2f5c3e6a4e6cfba1e74dbd13, not stripped
update_fes1:        ELF 64-bit LSB executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, for GNU/Linux 2.6.15, BuildID[sha1]=18600c480fea97af58c3c05f81f057b936e14b59, not stripped
update_mbr:         ELF 64-bit LSB executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, for GNU/Linux 2.6.15, BuildID[sha1]=23acde6f21548cc19f9e1805014bb14930655f6e, not stripped
update_uboot:       ELF 64-bit LSB executable, x86-64, version 1 (GNU/Linux), statically linked, for GNU/Linux 2.6.24, BuildID[sha1]=600eb8f2669a19c7fa6882cdd429f24f214f8a5c, not stripped
update_uboot_fdt:   ELF 64-bit LSB executable, x86-64, version 1 (GNU/Linux), statically linked, for GNU/Linux 2.6.24, BuildID[sha1]=6e70f48bf894fac773d7ee3b9a50d32b0bd437ba, not stripped


It is required to use dragon and fsbuild, which are 32-Bit Linux executables. The other executables are 64-Bit Linux executables. You can ignore the Windows binaries (Phew!). 
If you know where to get the original source-code (and what kind of licence these files have!), please get in touch with me: https://github.com/dan-and/ 


##Installation: 

It should be possible to use any kind of Linux Distribution, but I would recommend Ubuntu 16.04 (NOT 16.10 for now, because they upgraded GCC). 
you 

Distribution:  UBUNTU Linux 16.04: 64 Bit version only.

sudo apt install bc make git gcc gcc-arm-linux-gnueabihf u-boot-tools ncurses-devel

Due to the unsafe binary tool mentioned abouve, you are required to install 32-Bit in your 64-Bit environment. You can do it as following: 

sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install libc6:i386 libncurses5:i386 libstdc++6:i386


## Build: 

git clone <LINK-TO-GITHUB-REPO> 
cd BPI-M2U-bsp
./build.sh


## Files: 

You will find the results at:     out/azalea-m2ultra/


