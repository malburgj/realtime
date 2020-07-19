sudo apt-get install build-essential cmake pkg-config \
libavcodec-dev libavformat-dev libswscale-dev libgstreamer0.10-dev \
libgstreamer-plugins-base0.10-dev libgtk2.0-dev libpng-dev \
libjpeg-dev libopenexr-dev libtiff-dev libwebp-dev libxine2-dev \
libjasper-dev libdc1394-22-dev libv4l-dev libtbb2 libtbb-dev \
qt5-default libeigen3-dev yasm checkinstall libjpeg8-dev libpng12-dev \
libtiff5-dev libfaac-dev libmp3lame-dev libtheora-dev libvorbis-dev \
libxvidcore-dev libopencore-amrnb-dev libopencore-amrwb-dev \
libavresample-dev -y

sudo apt-get remove x264

sudo apt-get install x264 libx264-dev v4l-utils libprotobuf-dev protobuf-compiler \
libgoogle-glog-dev libgflags-dev libgphoto2-dev libhdf5-dev \
doxygen gfortran libatlas-base-dev

mkdir -p ~/tmp/opencv_repos && cd ~/tmp/opencv_repos
cwd=$(pwd)
cd $cwd

OPENCV_VERSION=3.4.10
git clone -b ${OPENCV_VERSION} https://github.com/opencv/opencv.git
git clone -b ${OPENCV_VERSION} https://github.com/opencv/opencv_contrib.git

mkdir opencv/build

cd $cwd/opencv/build

cmake -D CMAKE_BUILD_TYPE=RELEASE \
-D CMAKE_INSTALL_PREFIX=/usr/local \
-D OPENCV_EXTRA_MODULES_PATH=$cwd/opencv_contrib/modules \
-D OPENCV_ENABLE_NONFREE=ON \
-D WITH_TBB=ON \
-D WITH_OPENMP=ON \
-D WITH_IPP=ON \
-D WITH_V4L=ON \
-D WITH_OPENGL=ON \
-D WITH_OPENCL=ON \
-D WITH_EIGEN=ON \
-D ENABLE_NEON=ON \
-D ENABLE_VFPV3=ON \
..

make -j2
sudo make install
cd $cwd