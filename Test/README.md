PMCheck Benchmarks Setup
=====================================================

PMCheck is a TSO simlator for testing crash consistency in non-volatile memory tools. For testing a program with
PMCheck, either an actual non-volatile memory is needed or running an emulator would be sufficient. Here you can find
steps to set up an emulator, compile (or install) PMEM library. Then, use PMEM APIs to access the memory and develop your tool. 

Run unit tests
---------------

If your intention is just run our test cases and unit tests, use the following commands and ignore the rest of the document:
```bash
    make
    cd ../bin
```
For running the 'test1' use the following command:
```bash
    ./run.sh ./test1
```

In case of debugging our test cases, use gdb in child fork-follow mode:
```bash
    ./run.sh gdb ./test1
    set follow-fork-mode child
    run
```

PMCheck developers
---------------

If you are one of the PMCheck developers and want to collaborate to PMCheck. We already setup a virtual machine with the emulators and library installed on it. You can access this virtual machine to expedite the development of the benchmark and test it with PMCheck. In order to access the machine please contact Hamed Gorjiara at <hgorjiar@uci.edu>. Otherwise, feel free to take the following steps to install the emulator and PMDK.

Setup Intel Emulator
---------------
```bash
    cd /usr/src/linux 
    apt-get install libncurses5-dev libncursesw5-dev
    make config
    [Enabling DAX and NVM in Emulator:](https://software.intel.com/en-us/articles/how-to-emulate-persistent-memory-on-an-intel-architecture-server)
    Make –jX #(X is number of cores)
    #X can be calculated with the following command:
    #grep -c ^processor /proc/cpuinfo
    sudo vi /etc/default/grub
```
In grub file set the following configuration and then update the grub and mount the logical persistent memory:
```bash
    GRUB_CMDLINE_LINUX="memmap=nn[KMG]!ss[KMG]"
    sudo update-grub2
    sudo mkdir /mnt/mem
    sudo mkfs.ext4 /dev/pmem0   
    sudo mount -o dax /dev/pmem0 /mnt/mem
```
Now, /mnt/mem is pointing to the logical persistent memory.

Setup PMDK
---------------

You can either [install PMDK](https://docs.pmem.io/persistent-memory/getting-started-guide/installing-pmdk/compiling-pmdk-from-source) and [install ndclt](https://docs.pmem.io/persistent-memory/getting-started-guide/installing-ndctl) and start developping your tool or take the following steps:

```bash
    sudo apt-get install -y git gcc g++ autoconf automake asciidoc asciidoctor bash-completion xmlto libtool pkg-config libglib2.0-0 libglib2.0-dev doxygen graphviz pandoc libncurses5 libkmod2 libkmod-dev libudev-dev uuid-dev libjson-c-dev libkeyutils-dev
    git clone https://github.com/pmem/ndctl
    cd ndctl
    ./autogen.sh
	./configure CFLAGS='-g -O2' --prefix=/usr/local --sysconfdir=/etc --libdir=/usr/local/lib
	Make
	sudo make install
	# VALIDATION: /usr/local/lib/pkgconfig# ls 
    # OUTPUT: libdaxctl.pc  libndctl.pc
	export PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig:/usr/local/lib/pkgconfig:/usr/lib64/pkgconfig:/usr/lib/pkgconfig
	apt install autoconf automake pkg-config libglib2.0-0 libglib2.0-dev doxygen graphviz pandoc libncurses5
	git clone https://github.com/pmem/pmdk 
    cd pmdk
	Make
	make install prefix=/usr/local
```
Add these /usr/local/... paths to your LD_LIBRARY_PATH and PATH shell environment if it does not exist.


Contact
-------

Please feel free to contact us for more information. Bug reports are welcome,
and we are happy to hear from our users. We are also very interested to know if
PMCheck catches bugs in your programs.

Contact Hamed Gorjiara at <hgorjiar@uci.edu> or Brian Demsky at <bdemsky@uci.edu>.


Copyright
---------

Copyright &copy; 2020 Regents of the University of California. All rights reserved.

