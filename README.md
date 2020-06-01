# labrador-linux-32
Linux 4 for Caninos Labrador V2

@What is all about?
>This repository contains the source code of Labrador's 32bits linux kernel (version 4.14.13).

>Labrador Core Boards v2.x should work fine with this kernel!

@What is needed?
>These tools are necessary for compilation, most of them are available at official repositories.
git, libncurses-dev, gcc-arm-linux-gnueabihf, u-boot-tools

@How to compile?
>Clone the git repository, go to the main path and run make from your favourite terminal.

$ make

@How can I execute an "incremental" build?
>Just execute the command below;

$ make kernel

@How to choose what modules are compiled in?
>Just execute the command below;

$ make menuconfig

@How to transfer the compiled kernel to my board?

>After a successful build, the kernel should be avaliable at output folder.

>The modules are located at output/lib/modules. Copy them to the folder /lib/modules at your Linux system's root.

$ sudo rm -r /lib/modules
$ sudo cp -r output/lib/modules /lib/modules

>Copy the uImage file to /media/caninos/BOOT/ folder.

$ sudo rm /media/caninos/BOOT/uImage
$ cp output/uImage /media/caninos/BOOT/

>Copy device tree files to /media/caninos/BOOT/ folder.

$ rm /media/caninos/BOOT/kernel.dtb
$ cp output/kernel.dtb /media/caninos/BOOT/

@About examples

>The folder 'examples' has some examples of how to use some features of Labrador. 

@where to find out more about Caninos Loucos and Labrador?

>Access the forum https://forum.caninosloucos.org.br/

>Access our site https://caninosloucos.org/pt/

>Acess the wiki https://wiki.caninosloucos.org.br/index.php

