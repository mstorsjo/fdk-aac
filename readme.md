#FDK AAC

This is an audio codec. AAC can be seen as a "more modern MP3", and it is used in many systems. This code is freely available, but if you want to incorporate it or share it in your project or distribute it in binary form, especially for commercial, then you may need a license. This is not legal council, please research this yourself.


### Installation

First, install dependencies required to build it.

`sudo apt install build-essential autoconf automake libtool`

#you can also use mingw-w64 or other compilers, cmake or other tools as you want

Then follow these steps:

git clone https://github.com/mstorsjo/fdk-aac.git
cd fdk-aac
autoreconf -i
./configure
make
sudo make install
cd ..


