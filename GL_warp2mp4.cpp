#ifdef _WIN64
#include "windows.h"
#endif



/*
 * GL_warp2mp4.cpp
 * 
 * Warps video files using Paul Bourke's mesh files. 
 * Appends F to the filename and saves as default codec (DIVX avi) in the same folder.
 * 
 * first commit:
 * Hari Nandakumar
 * 03 Apr 2020
 * 
 * MIT License.
 * 
 * 
 */

//#define _WIN64
//#define __unix__

/* references: 
 * http://paulbourke.net/dataformats/meshwarp/
 * https://github.com/pdbourke/vlc-warp-2.1/blob/vlc_warp/modules/video_output/opengl.c
 * http://paulbourke.net/miscellaneous/lens/
 * https://github.com/hn-88/GL_warp2Avi
 * https://www.codeproject.com/Articles/34472/Video-Texture-in-OpenGL
 * https://stackoverflow.com/questions/9097756/converting-data-from-glreadpixels-to-opencvmat/9098883
 * 
 * */


#include <stdio.h>
#include <stdlib.h>

#ifdef __unix__
#include <unistd.h>
#endif

#include <string.h>

#include <time.h>
//#include <sys/stat.h>
// this is for mkdir

#include <opencv2/opencv.hpp>
#include <GL/glut.h>

#include "tinyfiledialogs.h"

#define CV_PI   3.1415926535897932384626433832795

using namespace cv;

// some global variables
GLint g_hWindow;

GLvoid InitGL();
GLvoid OnDisplay();
GLvoid OnReshape(GLint w, GLint h);
GLvoid OnKeyPress (unsigned char key, GLint x, GLint y);
GLvoid OnIdle();

VideoCapture inputVideo; 
VideoWriter outputVideo;
Mat src, dst, flipped;
int  fps, key;
int t_start, t_end;
unsigned long long framenum = 0;
int outputw = 1920;
int outputh = 1080;
char *pixels;  

GLvoid InitGL()
{  

	glClearColor (0.0, 0.0, 0.0, 0.0);

	glutDisplayFunc(OnDisplay);
	glutReshapeFunc(OnReshape);
	glutKeyboardFunc(OnKeyPress);
	glutIdleFunc(OnIdle);

}

GLvoid OnDisplay(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);

	// Set Projection Matrix
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, outputw, outputh, 0);

	// Switch to Model View Matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Draw a textured quad
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex2f(outputw, 0.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex2f(outputw, outputh);
	glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, outputh);
	glEnd();
	
	
	
	glReadPixels(0,0,outputw,outputh,GL_BGR ,GL_UNSIGNED_BYTE,dst.data);
	//glReadPixels(0,0,lpbih->biWidth,lpbih->biHeight,GL_BGR_EXT,GL_UNSIGNED_BYTE,bmBits);
	flip(dst, flipped, 0);
	outputVideo << flipped;
	
	glFlush();
	glutSwapBuffers();

}


GLvoid OnReshape(GLint w, GLint h)
{
	glViewport(0, 0, w, h);
}

GLvoid OnKeyPress(unsigned char key, int x, int y)
{
	switch (key) {
	  case 27:	// ESC key
		  
		  glutDestroyWindow(g_hWindow);
		  std::cout << std::endl << "Finished writing" << std::endl;
		  exit(0);
		  break;
	}
	glutPostRedisplay();
}


GLvoid OnIdle()
{
	// Capture next frame
	//IplImage *image = cvQueryFrame(inputVideo);
	Mat srcRGB;
	inputVideo >> src; // gets the next frame into image
	if (src.empty()) // end of video;
	{
		glutDestroyWindow(g_hWindow);
		std::cout << std::endl << "Finished writing" << std::endl;
		exit(0);
	}
	
	// Convert to RGB
	//cvCvtColor(image, image, CV_BGR2RGB);
	cvtColor(src, srcRGB, CV_BGR2RGB);

	// Create Texture
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, srcRGB.cols, srcRGB.rows, GL_RGB, GL_UNSIGNED_BYTE, srcRGB.data);
	
	// Update View port
	glutPostRedisplay();
	
	std::cout << "\x1B[0E"; // Move to the beginning of the current line.
	fps++;
	t_end = time(NULL);
	if (t_end - t_start >= 5)
	{
		std::cout << "Frame: " << framenum++ << " fps: " << fps/5 <<  std::flush;
		t_start = time(NULL);
		fps = 0;
	}
	else
	std::cout << "Frame: " << framenum++ << std::flush;
        
}



int main(int argc,char *argv[])
{
	
	////////////////////////////////////////////////////////////////////
	// Initializing variables
	////////////////////////////////////////////////////////////////////
	bool doneflag = 0, interactivemode = 0;
        
    std::string tempstring;
    std::string strpathtowarpfile;
    strpathtowarpfile = "EP_xyuv_1920.map";
    char outputfourccstr[40];	// leaving extra chars for not overflowing too easily
    outputfourccstr[0] = 'X';
    outputfourccstr[1] = 'V';
    outputfourccstr[2] = 'I';
    outputfourccstr[3] = 'D';
    
    const bool askOutputType = argv[3][0] =='Y';  // If false it will use the Xvid codec type
    // askOutputType=1 works only on Windows currently
    
    std::ifstream infile("GL_warp2mp4.ini");
    
    // inputs from ini file
    if (infile.is_open())
		  {
			
			infile >> tempstring;
			infile >> tempstring;
			infile >> tempstring;
			// first three lines of ini file are comments
			infile >> outputw;
			infile >> tempstring;
			infile >> outputh;
			infile >> tempstring;
			infile >> outputfourccstr;
			infile >> tempstring;
			infile >> strpathtowarpfile;
			infile.close();
			
		  }

	else std::cout << "Unable to open ini file, using defaults." << std::endl;
	
	std::cout << "Output codec type: " << outputfourccstr << std::endl;
	
	char const * FilterPatterns[2] =  { "*.avi","*.*" };
	char const * OpenFileName = tinyfd_openFileDialog(
		"Open a video file",
		"",
		2,
		FilterPatterns,
		NULL,
		0);

	if (! OpenFileName)
	{
		tinyfd_messageBox(
			"Error",
			"No file chosen. ",
			"ok",
			"error",
			1);
		return 1 ;
	}
	
	// reference:
	// https://docs.opencv.org/3.4/d7/d9e/tutorial_video_write.html
	
	inputVideo = VideoCapture(OpenFileName);              // Open input
	if (!inputVideo.isOpened())
    {
        std::cout  << "Could not open the input video: " << OpenFileName << std::endl;
        return -1;
    }
     
    std::string OpenFileNamestr = OpenFileName;    
    std::string::size_type pAt = OpenFileNamestr.find_last_of('.');                  // Find extension point
    const std::string NAME = OpenFileNamestr.substr(0, pAt) + "F" + ".avi";   // Form the new name with container
    int ex = static_cast<int>(inputVideo.get(CAP_PROP_FOURCC));     // Get Codec Type- Int form
    // Transform from int to char via Bitwise operators
    char EXT[] = {(char)(ex & 0XFF) , (char)((ex & 0XFF00) >> 8),(char)((ex & 0XFF0000) >> 16),(char)((ex & 0XFF000000) >> 24), 0};
    Size S = Size((int) inputVideo.get(CAP_PROP_FRAME_WIDTH),    // Acquire input size
                  (int) inputVideo.get(CAP_PROP_FRAME_HEIGHT));
    Size Sout = Size(outputw,outputh);            
    
    if (!(outputfourccstr[0] == 'N' &&
    outputfourccstr[1] == 'U' &&
    outputfourccstr[2] == 'L' &&
    outputfourccstr[3] == 'L'))
        outputVideo.open(NAME, outputVideo.fourcc(outputfourccstr[0], outputfourccstr[1], outputfourccstr[2], outputfourccstr[3]), 
        inputVideo.get(CAP_PROP_FPS), Sout, true);
    else
        outputVideo.open(NAME, ex, inputVideo.get(CAP_PROP_FPS), Sout, true);
    if (!outputVideo.isOpened())
    {
        std::cout  << "Could not open the output video for write: " << OpenFileName << std::endl;
        return -1;
    }
    std::cout << "Input frame resolution: Width=" << S.width << "  Height=" << S.height
         << " of nr#: " << inputVideo.get(CAP_PROP_FRAME_COUNT) << std::endl;
    std::cout << "Input codec type: " << EXT << std::endl;
        
    t_start = time(NULL);
	fps = 0;
	
	dst.create(outputh, outputw, CV_8UC3);	// (rows, columns, type)
	// https://stackoverflow.com/questions/9097756/converting-data-from-glreadpixels-to-opencvmat/9098883
	//use fast 4-byte alignment (default anyway) if possible
	glPixelStorei(GL_PACK_ALIGNMENT, (dst.step & 3) ? 1 : 4);
	//set length of one complete row in destination data (doesn't need to equal img.cols)
	glPixelStorei(GL_PACK_ROW_LENGTH, dst.step/dst.elemSize());
	
	// Create GLUT Window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(outputw, outputh);
	g_hWindow = glutCreateWindow("Display");

    // Initialize OpenGL
	InitGL();
	glutMainLoop();
    
    return 0;
	   
	   
} // end main
