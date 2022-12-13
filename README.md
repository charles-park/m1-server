# m1-server
ODROID-M1 stand-alone jig app
### Image used for testing.
* https://dn.odroid.com/RK3568/ODROID-M1/Ubuntu/ubuntu-20.04-server-odroidm1-20220531.img.xz
* Add overlay "uart0" and remove "spi0" to config.ini file.
* Add resolution and refresh values for using vu7 hdmi display.
```
[generic]
resolution=1920x1080
refresh=60

overlay_resize=16384
overlay_profile=
overlays="uart0 i2c0 i2c1"

[overlay_custom]
overlays="i2c0 i2c1"

[overlay_hktft32]
overlays="hktft32 ads7846"
```
* add root user, ssh root enable (https://www.leafcats.com/176)
```
root@odroid:~# passwd root
```

### Install package
* vim, build-essentail, git, overlayroot, python3, python3-pip,samba, pkg-config, ssh, ethtool, nmap, usbutils, evtest, iperf3 (in install folder)
* python3 -m pip install aiohttp asyncio
* wiring Pi install (https://wiki.odroid.com/odroid-m1/application_note/gpio/wiringpi#tab__odroid-m1)

### pettiboot skip
* Create a petitboot.cfg file in /boot to skip petitboot.
```
root@server:~# vi /boot/petitboot.cfg
[petitboot]
petitboot,timeout=2
```

### Clone repository for Header40 port testing
* for header40 testing.
```
root@odroid:~# git clone https://github.com/charles-park/header40
```
### Sound setup
```
root@server:~# aplay -l
**** List of PLAYBACK Hardware Devices ****
card 0: ODROIDM1HDMI [ODROID-M1-HDMI], device 0: fe400000.i2s-i2s-hifi i2s-hifi-0 [fe400000.i2s-i2s-hifi i2s-hifi-0]
  Subdevices: 1/1
  Subdevice #0: subdevice #0
card 1: ODROIDM1FRONT [ODROID-M1-FRONT], device 0: fe410000.i2s-rk817-hifi rk817-hifi-0 [fe410000.i2s-rk817-hifi rk817-hifi-0]
  Subdevices: 1/1
  Subdevice #0: subdevice #0

root@server:~# amixer -c 1
Simple mixer control 'Playback Path',0
  Capabilities: enum
  Items: 'OFF' 'RCV' 'SPK' 'HP' 'HP_NO_MIC' 'BT' 'SPK_HP' 'RING_SPK' 'RING_HP' 'RING_HP_NO_MIC' 'RING_SPK_HP'
  Item0: 'OFF'
Simple mixer control 'Capture MIC Path',0
  Capabilities: enum
  Items: 'MIC OFF' 'Main Mic' 'Hands Free Mic' 'BT Sco Mic'
  Item0: 'MIC OFF'

root@server:~# amixer -c 1 sset 'Playback Path' 'HP'
root@server:~# amixer -c 1
Simple mixer control 'Playback Path',0
  Capabilities: enum
  Items: 'OFF' 'RCV' 'SPK' 'HP' 'HP_NO_MIC' 'BT' 'SPK_HP' 'RING_SPK' 'RING_HP' 'RING_HP_NO_MIC' 'RING_SPK_HP'
  Item0: 'HP'
Simple mixer control 'Capture MIC Path',0
  Capabilities: enum
  Items: 'MIC OFF' 'Main Mic' 'Hands Free Mic' 'BT Sco Mic'
  Item0: 'MIC OFF'
```
### Use the lib_fbui submodule.
* Add the lib fbui submodule to the m1-server repository.
```
root@odroid:~/m1-server# git add submodule https://github.com/charles-park/lib_fbui
Cloning into '/root/m1-server/lib_fbui'...
remote: Enumerating objects: 20, done.
remote: Counting objects: 100% (20/20), done.
remote: Compressing objects: 100% (15/15), done.
remote: Total 20 (delta 4), reused 17 (delta 4), pack-reused 0
Unpacking objects: 100% (20/20), 44.66 KiB | 1.17 MiB/s, done.

root@odroid:~/m1-server# git commit -am "lib_fbui submodule add"
[main f23978c] lib_fbui submodule add
 2 files changed, 4 insertions(+)
 create mode 100644 .gitmodules
 create mode 160000 lib_fbui

root@odroid:~/m1-server# git push origin
Username for 'https://github.com': charles-park
Password for 'https://charles-park@github.com': 
Enumerating objects: 4, done.
Counting objects: 100% (4/4), done.
Delta compression using up to 4 threads
Compressing objects: 100% (3/3), done.
Writing objects: 100% (3/3), 395 bytes | 395.00 KiB/s, done.
Total 3 (delta 0), reused 0 (delta 0)
To https://github.com/charles-park/m1-server
   8147015..f23978c  main -> main
root@odroid:~/m1-server# 

```

* Clone the reopsitory with submodule.
```
root@odroid:~# git clone --recursive https://github.com/charles-park/m1-server

or

root@odroid:~# git clone https://github.com/charles-park/m1-server
root@odroid:~# cd m1-server
root@odroid:~/m1-server# git submodule init
root@odroid:~/m1-server# git submodule update
```
### Overlay root
* overlay-root enable
```
root@odroid:~# update-initramfs -c -k $(uname -r)
update-initramfs: Generating /boot/initrd.img-4.9.277-75
root@odroid:~#
root@odroid:~# mkimage -A arm64 -O linux -T ramdisk -C none -a 0 -e 0 -n uInitrd -d /boot/initrd.img-$(uname -r) /boot/uInitrd 
Image Name:   uInitrd
Created:      Wed Feb 23 09:31:01 2022
Image Type:   AArch64 Linux RAMDisk Image (uncompressed)
Data Size:    13210577 Bytes = 12900.95 KiB = 12.60 MiB
Load Address: 00000000
Entry Point:  00000000
root@odroid:~#

overlayroot.conf 파일의 overlayroot=””를 overlayroot=”tmpfs”로 변경합니다.
vi /etc/overlayroot.conf
overlayroot_cfgdisk="disabled"
overlayroot="tmpfs"
```
* overlay-root modified/disable  
```
[get write permission]
odroid@hirsute-server:~$ sudo overlayroot-chroot 
INFO: Chrooting into [/media/root-ro]
root@hirsute-server:/# 

[disable overlayroot]
overlayroot.conf 파일의 overlayroot=”tmpfs”를 overlayroot=””로 변경합니다.
vi /etc/overlayroot.conf
overlayroot_cfgdisk="disabled"
overlayroot=""

```
