# Steps to install 

1. install requirements libraries: 
````bash
sudo apt update
sudo apt-get install build-essential flex bison
sudo apt-get install libgmp-dev
````

2. Download PBC source code and compile it
````bash
wget https://crypto.stanford.edu/pbc/files/pbc-0.5.14.tar.gz
tar -xf pbc-0.5.14.tar.gz
cd pbc-0.5.14
./configure --prefix=/usr --enable-shared
make
sudo make install
cd ..
````

3. re-configure the ldconfig
````bash
 sudo ldconfig
````

4. Install the pypbc

````bash
sudo pip3 install .
````

