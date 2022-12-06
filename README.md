# m1-server
ODROID-M1 stand-alone jig app
### Image used for testing.
* https://dn.odroid.com/RK3568/ODROID-M1/Ubuntu/ubuntu-20.04-server-odroidm1-20220531.img.xz
* Add overlay "uart0" and remove "spi0" to config.ini file.
```
[generic]
overlay_resize=16384
overlay_profile=
overlays="uart0 i2c0 i2c1"

[overlay_custom]
overlays="i2c0 i2c1"

[overlay_hktft32]
overlays="hktft32 ads7846"
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
