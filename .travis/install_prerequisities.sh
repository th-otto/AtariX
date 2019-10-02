#!/bin/bash

echo rvm_autoupdate_flag=0 >> ~/.rvmrc

case "$TRAVIS_OS_NAME" in
linux)
	sudo apt-get update
	sudo apt-get install -y \
		curl \
		libosmesa6-dev \
		libgl1-mesa-dev \
		libglu1-mesa-dev \
		libsdl1.2-dev \
		libsdl-image1.2-dev \
		libusb-dev \
		libusb-1.0-0-dev \
		libudev-dev \
		zsync \
		xz-utils \
		libjson-perl \
		libwww-perl
	;;

osx)
	;;

*)
	exit 1
	;;
esac

