# GL_warp2mp4
Utility to pre-warp a movie similar to TGAWarp at http://paulbourke.net/dome/tgawarp/

Trying to generalize [GL_warp2Avi](https://github.com/hn-88/GL_warp2Avi) to make it cross-platform, or at least run on Linux.

If OpenCV, GLUT (freeglut3) and CMake are installed on your system, instructions for building:

```
cd build
cmake ..
make GL_warp2mp4.bin

```
Parameters are set using GL_warp2mp4.ini in the build folder.

A file open dialog asks you for the input file. The output file is put in the same directory, with F.avi appended to the input filename. The codec used for the output is the same codec as for the input if available on your system, or as chosen in the ini file. (If the input file's codec is not available, the output is saved as an uncompressed avi, which can quickly become huge.)

Keyboard commands are
```
ESC, x or X to exit before the end of the video.

```



