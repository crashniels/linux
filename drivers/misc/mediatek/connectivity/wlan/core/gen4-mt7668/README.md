# MT7668S WiFi Driver for Linux

an initiative to port the Android MT7668S driver into Linux (OpenWrt).

<!-- ## Current State

### OpenWrt

- [x] Station Mode
- [x] AP Mode
- [x] P2P Mode
- [x] Driver can load correctly after reboot

#### Known Bugs in Openwrt
- [ ] Luci not showing connected devices
- [ ] NULL MAC address in `/etc/config/wireless`
- [ ] Need to specify interface `ap0` when creating Access Point

### Armbian

- [ ] Untested

### Other Linux Distros

- [ ] Untested -->

<!-- ## Installation

To install the MT7668S driver on your Linux system (as an external module), follow these steps:

1. **Clone the repository:**
   ```sh
   git clone https://github.com/yhpgi/mt7668s.git
   cd mt7668s
   ```

2. **Edit the build files:**

   * Change `KERNELDIR` with your actual kernel/headers path.
   * Change `CONFIG_MT7668S_WIFI_MESON_G12A_PATCH=y` to `CONFIG_MT7668S_WIFI_MESON_G12A_PATCH=n` if your target is not an Amlogic g12a.

3. **Build the driver:**
   ```sh
   ./build.sh
   ```

4. **Install the driver:**
   ```sh
   sudo cp mt7668s.ko /lib/modules/$(uname -r)/
   ```

5. **Copy firmware files:**
   ```sh
   sudo cp firmware/* /lib/firmware/
   ```

6. **Load the driver:**
   ```sh
   sudo modprobe mt7668s
   # or
   sudo insmod /lib/modules/$(uname -r)/mt7668s.ko
   ```

7. **Load driver at every reboot:**
   ```sh
   echo mt7668s | sudo tee -a /etc/modules.d/mt7668s
   # or
   echo mt7668s | sudo tee -a /etc/modules-load.d/mt7668s
   # depends on your distro

   # or you can input it directly using a file manager or your favorite text editor
   ```

## Compiling the Driver In-tree with Your Kernel

1. **Move working directory to `linux-x.y/drivers`**
   ```sh
   cd linux-x.y/drivers
   ```

2. **Clone the repository:**
   ```sh
   git clone https://github.com/yhpgi/mt7668s.git
   ```

3. **Edit `Makefile` and `Kconfig` in `linux-x.y/drivers`:**

   **Makefile:**
   ```makefile
   obj-y += mt7668s/
   ```

   **Kconfig:**
   ```kconfig
   source "drivers/mt7668s/Kconfig"
   ```

4. **Run menuconfig:**
   ```sh
   cd linux-x.y

   make menuconfig
   # or
   make ARCH=arm64 menuconfig
   ```

   Enable this driver in the `Device Driver` section.
-->

> [!IMPORTANT]
> You must enable the `MT7668S WiFi Meson G12A workaround` at compile time if your target device is Amlogic g12a.
>
> DO NOT ENABLE FOR OTHER TARGETS as it will degrade WiFi performance.

<!-- 5. **Compile the kernel as usual.**

## Usage

After installing and loading the driver, you can configure the WiFi settings using standard Linux networking tools such as `iwconfig` or `nmcli`. For OpenWrt, the configuration can be done via the LuCI interface or by editing `/etc/config/wireless`.

## Troubleshooting

If you encounter any issues, please check the following:

- Ensure that your kernel version is compatible with the driver.
- Check the system logs for any error messages related to the driver.
-->

## Contributing

If you find any issues or have suggestions for improvements, please submit a pull request or open an issue in the repository.

## Acknowledgements

Special thanks to:

- MediaTek Inc.
- Amazon Inc.
- [DBAI](https://github.com/armarchindo)
- Everyone who is contributing to porting this driver to Linux

## License

- Original FOSS code from MediaTek Ltd. is licensed under the [GPLv2 License](LICENSE), yet firmware files are licensed under Dual License BSD/GPL.
- This project is also licensed under the [GPLv2 License](LICENSE).
