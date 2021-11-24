Jaaru: A model checker for persistent memory
=====================================================

Jaaru is a tool for testing persistent memory programs. Jaaru is capable of model-checking programs with different sizes and simulate crashes in critical places to test crash consistency of programs. It has two different modes of operations: (1) Random mode which explore random executions and insert crashes randomly (2) Model Checking mode which systematically explore executions and insert crashes before every flush operation. Jaaru has an LLVM pass to instrument memory and cache operations in the program.

Jaaru supports two popular APIs for accessing persistent memory. [PMDK](https://pmem.io/pmdk/) and volatile memory allocator ([libvmmalloc](https://pmem.io/pmdk/manpages/linux/v1.3/libvmmalloc.3.html)) which can be specified at compile time.

Getting Started
---------------

Below you can find step-by-step instructions to setup Jaaru on *JAARUHOME* directory of your machine. *JAARUHOME* environment variable should be set to your desired location as following:
```
	$ export JAARUHOME=/path/to/your/directory
```

1. You need to download and compile the Jaaru's LLVM pass. First download the llvm source code and compile our llvm pass with:
```
	cd $JAARUHOME
	# Download LLVM Source Code
	$ git clone https://github.com/llvm/llvm-project.git
	$ cd llvm-project
	$ git checkout 7899fe9da8d8df6f19ddcbbb877ea124d711c54b
	$ cd ..
	# Download Jaaru's LLVM Pass Source Code
	$ git clone https://github.com/uci-plrg/jaaru-llvm-pass.git
        $ mv jaaru-llvm-pass PMCPass
        $ cd PMCPass
        $ git checkout vagrant
        $ cd ..
	# Compile Jaaru's LLVM Pass
	$ mv PMCPass llvm-project/llvm/lib/Transforms/
        $ echo "add_subdirectory(PMCPass)" >> llvm-project/llvm/lib/Transforms/CMakeLists.txt
        $ cd $JAARUHOME/llvm-project
        $ mkdir build
        $ cd build
        $ cmake -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" ../llvm
        $ make -j 4
```

2. You need to download and update Jaaru's scripts with the location of LLVM and Jaaru's LLVM Pass. Then, compile Jaaru and its test cases:
```
	# Download Jaaru's source code
	$ cd $JAARUHOME
	$ git clone https://github.com/uci-plrg/jaaru.git
	$ mv jaaru pmcheck
	$ cd pmcheck/
	$ git checkout psan
	# Updating the location of llvm and jaaru's pass in Jaaru's c and c++ wrapper script
	$ sed -i "s/LLVMDIR=.*/LLVMDIR=${JAARUHOME}\/llvm-project\//g" Test/gcc
	$ sed -i "s/JAARUDIR=.*/JAARUDIR=${JAARUHOME}\/pmcheck\/bin\//g" Test/gcc
	$ sed -i "s/LLVMDIR=.*/LLVMDIR=${JAARUHOME}\/llvm-project\//g" Test/g++
	$ sed -i "s/JAARUDIR=.*/JAARUDIR=${JAARUHOME}\/pmcheck\/bin\//g" Test/g++
	# Compile Jaaru and its test cases
	$ make test
```

After finishing step 2, Jaaru is now ready to use!

Playing With Jaaru
------------------

Once test cases are compiled, go to the *$JAARUHOME/pmcheck/bin* directory and run the test cases with `run.sh` script. For example in order to run `test1`, type the following command:
```
	$ cd bin
	$ ./run.sh ./test1
```

### Running with PMDK

By default, Jaaru enables [libvmmalloc](https://pmem.io/pmdk/manpages/linux/v1.3/libvmmalloc.3.html) which maps dynamic memory allocation APIs (e.g., `malloc`) to persistent memory. In this mode, the data structure under test should survive the crash event. `setRegionFromID` stores the pointer to the data structure which can be retrived via `getRegionFromID` API in the post-crash execution. To see how these APIs work check out our test cases in *pmcheck/Test* directory.

To enable debugging PMDK library, the `ENABLE_VMEM` flag has to be disabled in `config.h` file:
```
	$ cd $JAARUHOME/pmcheck/
	$ sed -i 's/.*\#define ENABLE_VMEM.*/\#define ENABLE_VMEM/g' config.h
	$ make test
```

Then, compile the application that uses PMDK with Jaaru and run the application. In the *$JAARUHOME/pmcheck/Test/* directory, `testpmdk.cc` test case uses PMDK and you can run it as following:
```
	$ cd $JAARUHOME/pmcheck/bin
	$ ./run.sh ./testpmdk
```

### Debugging

In case you want to debug the project, define `CONFIG_DEBUG` macro in `common.h`. If you need to run the [gdb](https://sourceware.org/gdb/current/onlinedocs/gdb/) to debug any test cases, you can use it as the following:
```
	$ cd $JAARUHOME/pmcheck/bin
	./run.sh gdb test1
```
Set a breakpoint in gdb and use 'set follow-fork-mode child' to debug your program.

Contact
-------

Please feel free to contact us for more information. Bug reports are welcome,
and we are happy to hear from our users. We are also very interested to know if
PMCheck catches bugs in your programs.

Contact Hamed Gorjiara at <hgorjiar@uci.edu> or Brian Demsky at <bdemsky@uci.edu>.


Copyright
---------

Copyright &copy; 2022 Regents of the University of California. All rights reserved.

