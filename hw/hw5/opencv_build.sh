cd ~/tmp/opencv_repos
cwd=$(pwd)

mkdir opencv/build
cd $cwd/opencv/build

cmake -D CMAKE_BUILD_TYPE=RELEASE \
-D CMAKE_INSTALL_PREFIX=/usr/local \
-D OPENCV_EXTRA_MODULES_PATH=$cwd/opencv_contrib/modules \
-D OPENCV_ENABLE_NONFREE=ON \
-D WITH_TBB=ON \
-D WITH_V4L=ON \
-D WITH_QT=ON \
-D WITH_OPENGL=ON \
-D WITH_OPENCL=ON \
-D WITH_EIGEN=ON \
-D WITH_IPP=OFF \
-D WITH_OPENMP=ON \
-D ENABLE_NEON=ON \
-D ENABLE_VFPV3=ON \
-D BUILD_TESTS=OFF \
-D INSTALL_C_EXAMPLES=OFF \
-D INSTALL_PYTHON_EXAMPLES=OFF \
-D BUILD_EXAMPLES=OFF \
-D WITH_PYTHON=OFF \
-D WITH_JAVA=OFF \
-D BUILD_opencv_java=OFF \
-D BUILD_opencv_python=OFF \
..

make -j4
sudo make install
cd $cwd