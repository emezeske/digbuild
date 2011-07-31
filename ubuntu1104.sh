#!/usr/bin/env bash

set -o errexit -o nounset -o pipefail

tmp_dir="$(mktemp -d)"

sudo apt-get update

# Ubuntu native packages
sudo apt-get install exuberant-ctags freeglut3-dev libboost-dev libboost-thread-dev libfreetype6-dev libglew1.5-dev libsdl1.2-dev libsdl-image1.2-dev scons scons-doc

# GMTL
wget -O- http://downloads.sourceforge.net/project/ggt/Generic%20Math%20Template%20Library/0.6.1/gmtl-0.6.1.tar.gz | tar -zxvC "$tmp_dir"
cd -- "$tmp_dir/gmtl-0.6.1"
sudo scons install

# Threadpool
if [ ! -d /usr/include/boost/ ]
then
    cd -- "$tmp_dir"
    wget http://downloads.sourceforge.net/project/threadpool/threadpool/0.2.5%20%28Stable%29/threadpool-0_2_5-src.zip
    unzip threadpool-0_2_5-src.zip
    sudo mv threadpool-0_2_5-src/threadpool/boost/* /usr/include/boost/
fi

# Agar
wget -O- http://stable.hypertriton.com/agar/agar-1.4.1.tar.gz | tar -zxvC "$tmp_dir"
cd -- "$tmp_dir/agar-1.4.1"
./configure
make depend all
sudo make install

echo "Installation done; you can now remove $tmp_dir and build + run:"
echo "scons --gmtl-include-dir=/usr/local/include/gmtl-0.6.1 run"
