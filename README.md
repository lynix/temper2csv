# temper2csv

Simple logging application for the Temper(1) USB thermo sensor device

[![Build Status](https://travis-ci.org/lynix/temper2csv.svg?branch=master)](https://travis-ci.org/lynix/temper2csv)


## Dependencies
 * libtemper1 (included as submodule)
 * libusb (v1.0 API)

## Installation
None required. But you may wish to install the `udev`-Rule file from `udev.rules`.
To do so, simply copy it to `/etc/udev/rules.d` and make sure your user is a
member of the *plugdev* group.

## Usage
To give it a try, just plug in the device and do
```
$ ./temper2csv -v
```

This fires up **temper2csv** with some default settings in verbose mode, which
should give you the current temperature reading within a couple of seconds.

For further usage information, please try the `-h` commandline option.

## License

This software is released under the terms of the MIT License, see file
'LICENSE'.
