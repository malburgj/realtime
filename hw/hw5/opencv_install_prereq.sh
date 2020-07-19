sudo apt-get update

sudo apt-get install build-essential cmake pkg-config -y
sudo apt-get install libavcodec-dev -y
sudo apt-get install libavformat-dev -y
sudo apt-get install libswscale-dev -y
sudo apt-get install libgstreamer0.10-dev libgstreamer-plugins-base0.10-dev -y

sudo apt-get install libgtk2.0-dev -y
sudo apt-get install libpng-dev libpng12-dev -y
sudo apt-get install libjpeg-dev libjpeg8-dev -y
sudo apt-get install libopenexr-dev -y
sudo apt-get install libtiff-dev libtiff5-dev -y
sudo apt-get install libwebp-dev -y
sudo apt-get install libxine2-dev -y
sudo apt-get install libjasper-dev -y
sudo apt-get install libdc1394-22-dev -y
sudo apt-get install libv4l-dev v4l-utils -y
sudo apt-get install libtbb2 libtbb-dev -y
sudo apt-get install libeigen3-dev -y
sudo apt-get install yasm -y
sudo apt-get install checkinstall -y
sudo apt-get install libfaac-dev -y
sudo apt-get install libmp3lame-dev -y
sudo apt-get install libtheora-dev -y
sudo apt-get install libvorbis-dev -y
sudo apt-get install libxvidcore-dev -y
sudo apt-get install libopencore-amrnb-dev libopencore-amrwb-dev -y
sudo apt-get install libavresample-dev -y

sudo apt-get remove x264 -y
sudo apt-get install x264 libx264-dev -y
sudo apt-get install libprotobuf-dev -y
sudo apt-get install protobuf-compiler -y
sudo apt-get install libgoogle-glog-dev -y
sudo apt-get install libgflags-dev -y
sudo apt-get install libgphoto2-dev -y
sudo apt-get install libhdf5-dev -y
sudo apt-get install gfortran -y
sudo apt-get install libatlas-base-dev -y

mkdir -p ~/tmp/opencv_repos && cd ~/tmp/opencv_repos
cwd=$(pwd)
cd $cwd

OPENCV_VERSION=3.4.10
git clone -b ${OPENCV_VERSION} https://github.com/opencv/opencv.git
git clone -b ${OPENCV_VERSION} https://github.com/opencv/opencv_contrib.git