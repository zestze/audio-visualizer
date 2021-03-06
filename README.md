# Audio-Visualizer 
[![Build Status](https://travis-ci.org/zestze/audio-visualizer.svg?branch=master)](https://travis-ci.org/zestze/audio-visualizer)

Summary
-------
This is an audio visualizer written in C++.
Currently it only works on wav files. They
must be placed in the `res/` folder, and 
need to be passed as arguments to the `test.sh` script.

Usage
-----
Create a `res/` folder at the top level.
Switch to the `test/` folder. In there, run
`install.sh` and then `test.sh` with the given
file name.

Example
-------
[Streamable link](https://streamable.com/02xn8)

Features
--------
- uses the FFT to display the samples on a spectrum
- designed to be quick and efficient

REQUIREMENTS
------------
- sfml
- fftw3
- aquila
- c++ compiler capable of c++14
- range-v3
- pugixml
