PMCheck: A modle checker for persistent memory
=====================================================

PMCheck is a tool for testing persistent memory tools. It is fast because it lazily simulate crashes in the code. 

Getting Started
---------------

You need to download and compile the PMCheck LLVM pass. First download the llvm source code:
    git clone https://github.com/llvm/llvm-project.git

Checkout the LLVM pass and copy and paste the pass to the LLVM pass directory:
    git clone ssh://plrg.ics.uci.edu/home/git/PMCPass.git
    mv PMCPass llvm-project/llvm/lib/Transforms/

Then compile the our pass:
    cd llvm-project
    mkdir build
    cd build
    cmake -DLLVM_ENABLE_PROJECTS=clang -G "Unix Makefiles" ../llvm
    make

Now the pass is compiled and it should be accessible in the following directory:
    llvm-project/build/lib/libPMCPass.so

Check out the PMCheck library and build the library. It will be available in 'bin' directory as libpmcheck.so:
    git clone ssh://plrg.ics.uci.edu/home/git/pmcheck.git
    make

To run the test cases, you need to modify 4 files in the 'Test' directory including g++, gcc, clang, and clang++. Modify 3 following address to 
point to Clang, the LLVM pass, and the PMCheck library:
    {/path/to/}/llvm-project/build/bin/clang -Xclang -load -Xclang {/path/to/}llvm-project/build/lib/libPMCPass.so -Wno-unused-command-line-argument -L{/path/to/}pmcheck/bin -lpmcheck $@

Once you modified the these files, go back to the root 'pmcheck' directory and run the following comamand to compile all the test cases:
    make test

Once test cases are compiled, go to the bin directory and run the test cases with 'run.sh' script. For example in order to run test1 do the following:
    cd bin
    ./run.sh ./test1

In case you want to debug the project, open common.h and define CONFIG_DEBUG. If you need to run the gdb to debug any test cases, you can use it as the following:
    cd bin
    ./run.sh gdb test1


Contact
-------

Please feel free to contact us for more information. Bug reports are welcome,
and we are happy to hear from our users. We are also very interested to know if
PMCheck catches bugs in your programs.

Contact Hamed Gorjiara at <hgorjiar@uci.edu> or Brian Demsky at <bdemsky@uci.edu>.


Copyright
---------

Copyright &copy; 2020 Regents of the University of California. All rights reserved.

