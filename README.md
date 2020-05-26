# stark
[![License](http://img.shields.io/badge/license-MIT-brightgreen.svg)](https://opensource.org/licenses/MIT)
[![c++11](https://img.shields.io/badge/platform-c++/11-blue.svg)](https://isocpp.org/wiki/faq/cpp11)

A tool for bluntifying a bidirected de bruijn graph by removing overlaps.

## Compilation

    git clone --recursive https://github.com/hnikaein/stark 
    cd stark/build
    cmake ..;  cmake --build . -- -j 8
    
## Dependencies

* CMake 3.10+

## Input

You should give a bidirected De Bruijn Graph as input. The tool doesn't check the 
input to see if it is a valid bidirected DBG or not. Also, the tool doesn't preserve
the tags. 