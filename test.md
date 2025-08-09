Project Description

- The goal of this project is to evaluate the real-time computing abilities of the Raspberry Pi 4B utilizing Ubuntu 24.04’s real time branch for Raspberry Pi’s (RT_PREEMPT).
- First, the installation process of Ubuntu 24.04 real-time branch on the Raspberry Pi 4B is evaluated and verified against official documentation.
- The PREEMPT_RT kernel is fine-tuned to maximize real-time performance on the Raspberry Pi 4B by isolating CPU’s, utilizing real-time scheduling, setting task priorities, deactivating power saving measures and disabling frequency scaling.
- Upon successful deployment and tuning, the real-time capabilities of the system are measured and evaluated, specifically worst-case latency and jitter using benchmarking tools such as cyclictest and stress-ng under a typical heavy load for a Raspberry Pi 4B (4 cores at high utilization).
- Finally, an existing RTAI lab exercise is migrated and re-implemented to function as a Linux user-space application utilizing the PREEMPT_RT kernel patch.
- The exercise in question is lab exercise 2, which covers parallel/GPIO port programming by the example of generating a real-time rectangle signal of a specified frequency.
- Below is a block diagram (Figure 1) to illustrate the original setup with RTAI and the new Figure 1: Initial setup with RTAIsetup with a Raspberry Pi (Figure 2).

Figure 2: New setup with a Raspberry Pi 4B

Analysis

- Requirements for the new Real-time Ubuntu based system:(Hard) Real-Time behaviorAn environment as close as possible to the existing RTAI environment
- Analysis objectives:Documenting the required steps and resources to transform a Raspberry Pi 4B into a real-time system using Real-Time Ubuntu 24.04.Measuring the hard real-time capability of the Raspberry Pi 4B with Real-Time Ubuntu 24.04.If hard real-time is not possible: Measuring the soft real-time capability of the system instead.Assessing the feasibility of replacing the current RTAI systems with Raspberry Pi 4Bs with Real-Time Ubuntu 24.04.Analyzing and defining the migration process of a RTAI lab exercise to the new PREEMPT_RT user-space application system and identifying challengesShowing the real-time capabilities of the system by implementing exercise 2 of the lab as a Linux user-space C program
- Out of scope:Custom kernel patches beyond the official Real-Time Ubuntu 24.04 image for Raspberry Pi 4Bs provided by CanonicalExtreme tuning measures (e.g. disabling the video output through HDMI of the Raspberry Pi and using SSH)A complete and comprehensive guide to PREEMPT_RT user-space application development.A guide setting up real-time permissions; Root access is still required similar to the original RTAI setup.

Concept and Implementation

- The documentation featured in this section of the document is focused on the re-implementation of the RTAI lab exercise 2 as a Linux user-space application in C.
- For a cookbook-style guide of setting up the Real-Time Ubuntu 24.04 environment, please refer to the section “System Startup”.
- As a brief reminder, the application to implement is a parallel/GPIO port square signal generator with an adjustable frequency.
- The structure of the program remains similar to the original RTAI implementation with two tasks; one for setting the square output signal to a high level and one for setting the output signal to a low level.
- But the tasks themselves are translated to the POSIX pthread model with a FIFO scheduler and a task priority of 99 [1].
- To emulate the periodic task type helper functions are implemented:inc_period(): Increases the period of the task → Sets the time when a task has to wake up [2].do_rt_task(): Defines what the task has to do [2].wait_rest_of_period(): Let the task wait until the period ends [2].simple_cyclic_task(): Main function of the pthread, houses the while loop and calls the helper functions.
- For precise timing the clock_nanosleep() method is used, which sets the tasks to sleep until a specific time point [2].
- inc_period() ensures absolute timing and guards against time drift [2].
- The function init_GPIO() initializes global variables for GPIO communication such as opening a line and setting it to the output mode [3].­
- In order to give the task information about its specific period and starting time, the time_info struct was created. This struct is filled with the wake up time of the next period and period length at runtime. This struct is passed to the pthreads [2].
- In the function tasks_init() the starting times are initialized and the tasks are started [2].
- Figure 3: Caller graph on start-upTo give a better overview of the programs structure, a caller graph on start-up and of the cyclic task loop is available. They are under figure 3 and 4 respectively.

Figure 4: Caller graph of the main cyclic loop of the program

Evaluation and Conclusion

- Attempts to make the Raspberry Pi a hard real-time systems have not been successful. Hard real-time capability on the Raspberry Pi 4B is not achieved. After extensive attempts to isolate the hardware as much as possible there are still nondeterministic factors which can cause unbounded latency. Some factors would be:USB communication (VL805 USB-3-Controller interrupts)The USB bus can restart or hang/crash unexpectedly, causing a large latency spike.HDMI communication (VideoCore interrupts)The video interrupts are all outside of the ARM CPU and thus cannot be controlled.However they must be handled. These video interrupts can arrive at any time and reserve the bus for a long time, causing a latency spike.These interrupts can be further examined as well in the official Raspberry Pi BCM2711 documentation on page 86 in figure 5 and further [4].ARM GIC-400 interrupt controller hard-coding interrupts to specific coresThe interrupts 32-35 have a fixed affinity mask for each one of the cores as they handle mailboxes. These cannot be changed and as a result a core may be interrupted to handle ARM mailbox messages, causing latency spikes.This assignment can be seen in the official Raspberry Pi BCM2711 ARM Peripherals documentation on page 90 under figure 7, where ARM Mailbox IRQs are bound to SPI IDs 32-47 (Linux only utilizes the first 4 Mailboxes) [4].
- With that being said, the performance of the Raspberry Pi as a soft real-time system is excellent. A maximum latency of 190us and an average latency of 24us under a high work load is measured in the 24 hour test. The PREEMPT_RT kernel with tuning does lead to a large improvement in determinism. Only a few factors remain that unbound maximum latency.
- The transition of lab exercise 2 to a PREEMPT_RT-based user-space application is successful, closely replicating the original RTAI behavior.
- Transitioning from RTAI to Real-Time Ubuntu 24.04 on the Raspberry Pi is feasible.
- The comforts of RTAI’s rich library are not present anymore, but equivalent functionality can be achieved using POSIX and other libraries.
- When starting the program however, a Jitter of about 50µs is visible when moving the mouse.One reason could be that clock_nanosleep() has its limits.Another reason could be, that the library libgpiod is the bottleneck. In one version of the program the hardware register was attempted to be used of the GPIO pins more directly with mmap, but even then the jitter was the same [5].For extremely high speed PWM signals this setup can not be recommended, but there are workarounds, e.g. the pigpio library can be used for very precise hardware PWM signals [6].

- For the demonstration and use of a real-time system in the lab, the Raspberry Pi provides a cost-effective adequate alternative to RTAI. The exercises are able to be done in a similar manner to RTAI and the deployment of the environment is easier than the RTAI variant.

System Startup

Installation of Ubuntu 24.04 on the Raspberry Pi

- The setup begins with preparing a flash memory card using the Raspberry Pi Imager, a tool provided by Raspberry Pi Ltd.On Windows hosts, the imager must be downloaded from the official Raspberry Pi Software website[7].On Linux systems that use the apt package manager (e.g., Debian or Ubuntu), the following command installs the imager[7]: sudo apt install rpi-imager
- The next step is to acquire the Ubuntu image for the Raspberry Pi:The image can be downloaded manually from the official Ubuntu Raspberry Pi page[8].Alternatively, the Raspberry Pi Imager can be used to fetch the image directly (Explained in the next step).
- The Raspberry Pi Imager is used to flash the new operating system to a microSD card:The correct device model (e.g., Raspberry Pi 4) is selected.The Ubuntu Desktop 24.04 image is chosen in one of two ways:If no image was downloaded from Canonicals website, one can be chosen here by selecting “Other general-purpose OS” > “Ubuntu” > “Ubuntu Desktop 24.04.X LTS (64-Bit)”If an image has been already downloaded from Canonicals website, there is an option at the very bottom of the list called “Own Image”After selecting this option the path to the downloaded image has to be specifiedThe microSD card is inserted and selected as the flash target.Additional configuration options are also presented:Setting a default username/passwordOptionally pre-configuring a Wi-Fi connection beforehandetc.After confirming all selections, the flashing process is initiated.The image is written and verified automatically.
- While the SD card is being prepared, an Ubuntu Pro account should be created.Ubuntu Pro is required to enable the real-time kernel.An Ubuntu Pro account can be created free of charge for personal use on the Ubuntu Pro website[9].The following information is required:E-MailFull NameDesired usernamePasswordAfter the account is created, a personal Ubuntu Pro token can be obtained via the Ubuntu dashboard [10].This token will be needed for linking a machine to a Ubuntu Pro subscription later.
- After the flashing process has completed, the SD card is inserted into the Raspberry Pi, and the system is powered on.
- Ubuntu will automatically start the install/setup procedure upon the first boot.Some more configuration options will be asked such as:Keyboard layoutTime zoneetc.It is also possible to activate Ubuntu Pro at this step.If an account has been already set up, then the instructions in the installer can be followed to link the machine to a subscription.This can also be done at a later point using the terminal or the system settings.
- On setup completion, the system reboots.
- After rebooting, the user must log in using their credentials.
- A terminal needs to be started:This can be either done via the graphical application launcher by finding the “Terminal” application.Or by alternatively using the keyboard shortcut CTRL+ALT+T.
- It is recommended to update the system to the latest version. This is achieved with the following command: sudo apt update [11].
- If Ubuntu Pro was not activated previously, then it can be done now by retrieving the token from the Ubuntu Pro dashboard [10] and running: sudo pro attach <YOUR-TOKEN> [11].
- It should also be ensured that the Ubuntu Advantage client (responsible for handling Ubuntu Pro subscriptions) is installed and up-to-date. To check, the following command has to be executed: sudo apt install ubuntu-advantage-tools [11].
- Once everything is ready, the real-time variant of the Ubuntu kernel can be installed using the following command: sudo pro enable realtime-kernel –-variant=raspi [11].
- Warning: Do not omit --variant=raspi or the wrong variant may be installed, resulting in a broken system![11]
- A warning will appear that Canonical Livepatch must be disabled.
- This must be accepted to proceed and can be done by pressing y [11].
- After the installation of the real-time kernel package runs its course and completes, the system must be restarted. This can be done with: sudo reboot
- The real-time kernel is installed after the system restarts.
- This can be verified by executing the command: uname -r -v [12]The -r flag specifies the kernel release version stringThe -v flag specifies the kernel build versionAn output after a successful installation is:6.8.0-2019-raspi-realtime #20-Ubuntu SMP PREEMPT_RT Thu Feb 27 14:16:18 UTC 2025
- If the system reports as PREEMPT_RT, then the real-time kernel is active and everything is functioning correctly.

Installing testing software and testing

- To determine the real-time capabilities, dedicated tools for measuring scheduling latency and stressing the CPU are needed.
- Two commonly utilized tools for this purpose are cyclictest and stress-ng [13].
- cyclictest is a common benchmarking suite specifically designed to access the determinism of real-time systems [14].
- It measures latency by creating one or more POSIX thread(s), which continuously schedule a high-resolution timer to fire at a fixed interval specified by the user [14].
- The latency measured is the delay from the interrupt trigger to the handling of the request and waking up [14].
- cyclictest repeats these measurements for millions of cycles and notes all the times, creating a minimum, a maximum and an average.
- The maximum is especially important, as a system is considered hard real-time when it defines the upper bound (maximum time) a system needs to respond to an event.
- To install cyclictest, the rt-tests package is used: sudo apt install rt-tests
- stress-ng is a versatile software suite of workload generators to stress test a system using a wide variety of configurable stressor programs [15].
- In the context of real-time systems, stress-ng is usually run alongside cyclictest to assess whether a system can remain within its maximum latency bounds even under load.
- It is installed by installing its package: sudo apt install stress-ng
- Together, cyclictest and stress-ng provide a comprehensive testing environment for benchmarking and validating the effectiveness of a real-time kernel.
- The detailed usage of these tools, including the specific parameters and a test scenario, is covered in a later section.

Tuning the system

- The RT_PREEMPT kernel already provides a large improvement over the standard kernel (~10x improvement in latency).
- However further optimizations can be performed to minimize latency sources.
- The first fundamental improvement is to disable unused peripheral interfaces. On the Raspberry Pi, this can be achieved using the raspi-config tool provided by Raspberry Pi Ltd. [16]:The raspi-config tool is originally designed for Raspberry Pi OS.However, as Raspberry Pi OS is simply a Debian-based distribution tailored for the Raspberry Pi OS, this tool is also available on Ubuntu, which is Debian-based as well.It is installed via: sudo apt install raspi-configIt is then launched with: sudo raspi-configThis will start the utility, which presents a text-based menu similar to legacy BIOS interfaces.Navigation is done with Arrow keys, options are selected with ENTER and going back is possible with ESCAPE.Before making changes, one should select Option 8 “Update” and perform a check to make sure the software is up-to-date.Next, interface settings are accessed by selecting Option 3 “Interface Options”.This menu has options for enabling/disabling interfaces, such as:Legacy CameraSSHVNCSPII2CSerial Port1-WireRemote GPIOOnly GPIO is relevant for the given real-time use caseTherefore it is recommended to disable all Options except “Remote GPIO”This minimizes the number of active kernel drivers and interrupt handlers, which in turn eliminates possible interrupt sources and improves determinism.Once the changes are applied, you can exit the configuration.
- Another essential optimization includes isolating a CPU core from the system to be a core exclusively dedicated for real-time tasks:This is done through the kernel command line, which is in the file /boot/firmware/cmdline.txt [20].This is a single-line configuration file parsed by the kernel during the boot process.Root privileges are required to edit it: sudo nano /boot/firmware/cmdline.txtWarning: This file must contain only one line. Do not insert line breaks. Do not edit existing parameters. Improper modification will make the system unbootable!If in doubt, cross-reference with Appendix A at the end of this document for a working configuration.To configure core isolation, the following keywords have to be added after the fixrtc keyword:nohz=on: Enables tickless idle/adaptive-tick mode, reducing timer interrupts [17].nohz_full=1: Specifies which CPU(s) is/are in adaptive tick mode, meaning they run without periodic scheduling interrupts [17].isolcpus=1: Isolate the specified CPU(s), preventing the scheduler from assigning regular tasks to these cores [18].rcu_nocbs=1: Deactivates callbacks for the specified CPU(s) core, further reducing non-deterministic workloads [17].For all the settings, CPU core 1 is taken as a setup example, but any other core combination can be specified using e.g. 0,2,3 or 1-3 [18].CPU core 1 is taken in this case because CPU core 0 is typically mandated to do system-critical operations such as handling timer ticks, IRQs and housekeeping.Designating CPU core 1 as a real-time core instead avoids any unnecessary interference and unpredictability, further improving temporal determinism.To apply the settings after making the changes, one must save with CTRL+S and then exit nano with CTRL+X.The system must be rebooted for the changes to take effect: sudo reboot.After rebooting, the isolated cores can be verified with: cat /sys/devices/system/cpu/isolatedThis command should output the numbers of the isolated cores if everything was applied successfullyBy having an isolated core, it can be ensured that the system exhibits stronger temporal determinism, as there are no interferences from the operating system or tasks that might impact the execution of real-time tasks.

- In addition of isolating a CPU core, it is also necessary to redirect hardware interrupts away from the isolated core:This setting is again applied in the /boot/firmware/cmdline.txt file [20].To edit this file: sudo nano /boot/firmware/cmdline.txtSame precautions as earlier apply.To redirect interrupts away from Core 1, the following parameter is applied after the previously added options: irqaffinity=0,2,3 [18]irqaffinity specifies which CPU cores are permitted to be handling interrupts [18].After omitting Core 1 from this parameter the system is instructed to not deliver any IRQs to that core.This is essential for ensuring core isolation and protecting real-time tasks from latency spikes caused by hardware and the operating system.After modifying the line, the file must be saved with CTRL+S and closed with CTRL+X.The system has to be rebooted again to apply the changes.To inspect the individual interrupt affinities of each interrupt, the content of /proc/irq/X/smp_affinity has to be checked [21].For example with cat: cat /proc/irq/X/smp_affinity (X being the number of interrupt to check) [21].For automation of the verfication of the IRQ affinity settings, a helper script is included in Appendix B, which iterates over all interrupt affinities and outputs them to the console.To use it, it has to be copied into a file (for example affinitytest.sh) and then made executable with chmod +x affinitytest.shAfter that it can be started by writing ./affinitytest.shAll interrupts but 32-35 should have the desired interrupt affinity set.NOTE: The interrupt affinity mask is a bit mask (for example d = 1101).As already mentioned in the evaluation, interrupts 32-35 are used to handle ARM mailboxes and are hard-coded to one CPU core each [4]. 
- For consistent real-time performance, CPU frequency must be disabled as well:Dynamic frequency scaling introduces variability in execution timing, which hinders temporal determinism:To disable it the systems CPU governor has to be configuredFor this, an utility has to be installed: sudo apt install cpufrequtilsAfter installing the configuration file under /etc/default/cpufrequtils has to be opened: sudo nano /etc/default/cpufrequtilsIn the file, two new lines have to be addedGOVERNOR=”performance” [22]The GOVERNOR option “performance” turns off power-saving measures by setting the CPU into performance mode [22].MIN_SPEED=”1.8GHz”The MIN_SPEED option specifies to what speed the CPU is allowed to throttle down to if idle.By setting it to 1.8GHz (Raspberry Pi 4Bs maximum turbo frequency) frequency scaling is effectively disabled as the CPU is not allowed to go below its maximumTo apply the changes, the file has to be saved and exited.It is then applied with: sudo /etc/init.d/cpufrequtils restartWith this it has been observed that some frequency scaling still happens howeverTo fix that, a more drastic measure is taken by setting an overclocking flag called force_turbo=1: [23]This is done within /boot/firmware/config.txt!!! Warning: Doing this sets a permanent bit on the SoC that voids the warranty of the Raspberry Pi !!! [24]When doing this a cooling solution will also need to be installed on the Raspberry Pi [24]. In the case of the test Pi a heat sink with 2 fans was installed (Pictured in figure 5).This cooling solution is sufficient for keeping the Raspberry Pi under ~60°C.

Figure 5: Cooling setup of the Raspberry Pi 4B with attached heat sink and fans.

- As a final tuning step, onboard wireless communication modules (Wi-Fi and Bluetooth) are disabled to reduce interrupt load and background activity:Wi-Fi and Bluetooth can be disabled at the firmware level using device tree overlays.For this the file /boot/firmware/config.txt has to be edited.This is achieved by modifying the /boot/firmwre/config.txt file in the boot partition: sudo nano /boot/firmware/config.txtTwo lines have to be added at the end of the file:dtoverlay = disable-wifi [25]dtoverlay = disable-bt [25]These overlays instruct the Raspberry Pi firmware to fully disable the Wi-Fi and Bluetooth subsystems, preventing their kernel drivers from loading and eliminating related background tasks and interrupts [25].This is of importance because WiFi and Bluetooth send out constant signals and poll various things, meaning they generate constant interrupts.Access to the internet can still be provided via Ethernet.

Testing real-time performance

- To assess the effectiveness of the applied system tuning and validate the real-time capabilities of the Raspberry Pi 4 running Ubuntu 24.04 with the PREEMPT_RT kernel, a 24-hour latency stress test was conducted.
- The objective of this test was to evaluate the maximum interrupt latency experienced by a high-priority thread bound to the isolated core, under continuous CPU load on the remaining cores.
- This simulates a worst-case scenario in which real-time responsiveness must be guaranteed despite high system utilization.
- Two tools were used in parallel:cyclictest to measure scheduling latency.stress-ng to generate background CPU load.
- The test was executed with the following parameters:cyclictest was pinned to CPU core 1, the previously isolated real-time core: sudo taskset -c 1 cyclictest -m --smp --priority=99 –interval=200-m: Lock memory to prevent paging.--priority=99: Use real-time FIFO policy with the highest priority.--interval=200: Set a 200us interval between timer expirations.--smp: Enable multiprocessing support.stress-ng was configured to fully utilize the remaining cores (0, 2, and 3): sudo stress-ng --cpu 3 --cpu-load 100 --taskset 0,2,3 --all 4
- The system was left running for 24 hours to capture a representative and statistically significant distribution of latencies under sustained load.
- The test results from cyclictest were as follows:Minimum latency: 15usAverage latency: 23usMaximum latency: 190us
- These results indicate that even under sustained system load, maximum latency remained below 200us, which is a strong result for a non-dedicated embedded system.
- The low worst-case latency demonstrates the effectiveness of the combined tuning strategies.
- The accompanying stress-ng output confirmed a successful run with all stressors active and no failures or skipped workloads, further validating the load conditions.
- The raw output log of the tests can be found under Appendix C.

Appendix A: A complete example of /boot/firmware/cmdline.txt (isolating CPU 1)

zswap.enabled=1 zswap.zpool=z3fold zswap.compressor=zstd multipath=off dwc_otg.lpm_enable=0 console=tty1 root=LABEL=writable rootfstype=ext4 rootwait fixrtc nohz=on nohz_full=1 isolcpus=1 rcu_nocbs=1 irqaffinity=0,2,3 cpuidle.off=1 quiet splash cfg80211.ieee80211_regdom=DE

Appendix B: Bash script for batch checking interrupt affinities

affinitytest.sh:

#!/bin/bash

for filename in /proc/irq/*; do

echo $filename/smp_affinity ":" $(cat $filename/smp_affinity)

done

Appendix C: 24h test log

sudo taskset -c 1 cyclictest -m --smp --priority=99 --interval=200

# /dev/cpu_dma_latency set to 0us

policy: fifo: loadavg: 3.10 3.66 4.79 2/674 6498

T: 0 ( 3186) P:99 I:200 C:435323964 Min: 15 Act: 38 Avg: 23 Max: 190

-----------------------------------------------------

sudo stress-ng --cpu 3 --cpu-load 100 --taskset 0,2,3 --all 4

stress-ng: info: [3203] defaulting to a 1 day, 0 secs run per stressor

stress-ng: info: [3203] dispatching hogs: 3 cpu

stress-ng: info: [3204] cpu: for stable load results, select a specific cpu stress method with --cpu-method other than 'all'

stress-ng: info: [3205] cpu: for stable load results, select a specific cpu stress method with --cpu-method other than 'all'

stress-ng: info: [3206] cpu: for stable load results, select a specific cpu stress method with --cpu-method other than 'all'

stress-ng: info: [3203] skipped: 0

stress-ng: info: [3203] passed: 3: cpu (3)

stress-ng: info: [3203] failed: 0

stress-ng: info: [3203] metrics untrustworthy: 0

stress-ng: info: [3203] successful run completed in 1 day, 0.11 secs

References

[1] “HOWTO build a simple RT application”, The Linux Foundation, https://wiki.linuxfoundation.org/realtime/documentation/howto/applications/application_base, 07.06.2025

[2] “HOWTO build a basic cyclic application”, The Linux Foundation, https://wiki.linuxfoundation.org/realtime/documentation/howto/applications/cyclic, 07.06.2025

[3] “libgpiod 2.0-devel”, libgpiod project, https://phwl.org/assets/images/2021/02/libgpiod-ref.pdf, 07.06.2025

[4] “BCM2711 ARM peripherals”, Raspberry Pi Ltd., https://datasheets.raspberrypi.com/bcm2711/bcm2711-peripherals.pdf, 04.05.2025

[5] “mmap – map pages of memory”, The Open Group Base Specifications Issue 8, The IEEE and The Open Group, https://pubs.opengroup.org/onlinepubs/9799919799/, 07.06.2025

[6]  “The pigpio library”, pigpio project, https://abyz.me.uk/rpi/pigpio/, 07.06.2025

[7] “Install Raspberry Pi OS using Raspberry Pi Imager”, Raspberry Pi Ltd., https://www.raspberrypi.com/software/, 28.04.2025

[8]  “Install Ubuntu on a Raspberry Pi”, Canonical Ltd., https://ubuntu.com/download/raspberry-pi, 28.04.2025

[9] “Ubuntu Pro”, Canonical Ltd., https://ubuntu.com/pro, 28.04.2025

[10] “Your subscriptions”, Canonical Ltd., https://ubuntu.com/pro/dashboard, 28.04.2025

[11] “How to enable Real-time Ubuntu”, Canonical Ltd., https://documentation.ubuntu.com/pro-client/en/latest/howtoguides/enable_realtime_kernel/, 28.04.2025

[12] “uname – print system information”, Free Software Foundation, https://www.man7.org/linux/man-pages/man1/uname.1.html, 28.04.2025

[13] “Discover real-time kernel on Ubuntu 24.04 LTS and Raspberry Pi”, ca. 42:00, Canonical Ubuntu, https://www.youtube.com/watch?v=lu9tMy2Ylhg, 28.04.2025

[14] “Cyclictest”, The Linux Foundation, https://wiki.linuxfoundation.org/realtime/documentation/howto/tools/cyclictest/start, 04.05.2025

[15] “stress-ng - a tool to load and stress a computer system”, Colin King & Amos Waterland, https://manpages.ubuntu.com/manpages/bionic/man1/stress-ng.1.html, 04.05.2025

[16] “Configuration – raspi-config”, Raspberry Pi Ltd., https://www.raspberrypi.com/documentation/computers/configuration.html, 12.05.2025

[17] “NO_HZ: Reducing Scheduling-Clock Ticks”, The kernel development community, https://www.kernel.org/doc/Documentation/timers/NO_HZ.txt, 12.05.2025

[18] “The kernel’s commandline parameters”, The kernel development community, https://docs.kernel.org/admin-guide/kernel-parameters.html, 12.05.2025

[19] “CPU Idle Time Management”, The kernel development community, https://kernel.org/doc/html/v5.0/admin-guide/pm/cpuidle.html#idle-states-control-via-kernel-command-line, 12.05.2025

[20] “How to modify kernel boot parameters”, Canonical Ltd., https://documentation.ubuntu.com/real-time/en/latest/how-to/modify-kernel-boot-parameters/, 12.05.2025

[21] “SMP IRQ affinity”, The kernel development community, https://docs.kernel.org/core-api/irq/irq-affinity.html, 18.05.2025

[22] “CPU performance scaling”, Rafael J. Wysocki, Intel Corporation, https://www.kernel.org/doc/html/latest/admin-guide/pm/cpufreq.html, 21.05.2025

[23] “config.txt”, Raspberry Pi Ltd., https://www.raspberrypi.com/documentation/computers/config_txt.html#force_turbo, 21.05.2025

[24] “Overclocking options”, Raspberry Pi Ltd., https://github.com/raspberrypi/documentation/blob/develop/documentation/asciidoc/computers/config_txt/overclocking.adoc, 21.05.2025

[25] “firmware/boot/overlays”, Raspberry Pi Ltd., https://github.com/raspberrypi/firmware/tree/master/boot/overlays, 21.05.2025