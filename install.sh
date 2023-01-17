#!/bin/bash
##################################
#
# Installation of pypbc
# Note: We using latest version at this moment, please update the PBC version which you download at line -10-
#
#
# Written by Arash Baheri <arashbaheri@icloud.com>
#
#
##################################
echo "Start installing pypbc ..." &&
sudo apt update &&
sudo apt-get install -y build-essential flex bison &&
sudo apt-get install -y libgmp-dev && 
wget https://crypto.stanford.edu/pbc/files/pbc-0.5.14.tar.gz &&
tar -xf pbc-0.5.14.tar.gz &&
cd pbc-0.5.14 &&
./configure --prefix=/usr --enable-shared &&
make &&
sudo make install &&
sudo ldconfig &&
cd .. &&
sudo pip3 install . &&
echo "...PYPBC installed successfully..."

