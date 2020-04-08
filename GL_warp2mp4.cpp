#ifdef _WIN64
#include "windows.h"
#endif

/*
 * GL_warp2mp4.cpp
 * 
 * Warps video files using Paul Bourke's mesh files. 
 * http://paulbourke.net/dataformats/meshwarp/
 * Appends W to the filename and saves as default codec (DIVX avi) in the same folder.
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
 * http://www.songho.ca/opengl/gl_pbo.html
 * http://www.songho.ca/opengl/gl_fbo.html
 * 
 * */
// GL_warp2mp4.cpp
// using FBO to pre-warp and save video files
///////////////////////////////////////////////////////////////////////////////
// modifed from main.cpp by Song Ho Ahn
// from fbo example, http://www.songho.ca/opengl/gl_fbo.html
// ========
// testing Frame Buffer Object (FBO) for "Render To Texture" with MSAA
// OpenGL draws the scene directly to a texture object.
//
// GL_EXT_framebuffer_object extension is promoted to a core feature of OpenGL
// version 3.0 (GL_ARB_framebuffer_object)
//
//  AUTHOR: Song Ho Ahn (song.ahn@gmail.com)
// CREATED: 2008-05-16
// UPDATED: 2016-11-14

// Modified by Hari Nandakumar
// 2020-04-06
// www.saispace.in
///////////////////////////////////////////////////////////////////////////////

// in order to get function prototypes from glext.h, define GL_GLEXT_PROTOTYPES before including glext.h
#define GL_GLEXT_PROTOTYPES

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>
#include <cstdlib>
#include "glext.h"
#include "glInfo.h"                             // glInfo struct
#include "Timer.h"
#include <opencv2/opencv.hpp>
#include "GL_warp2mp4.h"
#include "tinyfiledialogs.h"


using std::stringstream;
using std::string;
using std::cout;
using std::endl;
using std::ends;


// GLUT CALLBACK functions ////////////////////////////////////////////////////
void displayCB();
void reshapeCB(int w, int h);
void timerCB(int millisec);
void idleCB();
void keyboardCB(unsigned char key, int x, int y);
void mouseCB(int button, int stat, int x, int y);
void mouseMotionCB(int x, int y);

// CALLBACK function when exit() called ///////////////////////////////////////
void exitCB();

// function declearations /////////////////////////////////////////////////////
void initGL();
int  initGLUT(int argc, char **argv);
bool initSharedMem();
void clearSharedMem();
void initLights();
void setCamera(float posX, float posY, float posZ, float targetX, float targetY, float targetZ);
void drawString(const char *str, int x, int y, float color[4], void *font);
void drawString3D(const char *str, float pos[3], float color[4], void *font);
void showInfo();
void showFPS();
void toOrtho();
void toPerspective();
void draw();

// FBO utils
bool checkFramebufferStatus(GLuint fbo);
void printFramebufferInfo(GLuint fbo);
std::string convertInternalFormatToString(GLenum format);
std::string getTextureParameters(GLuint id);
std::string getRenderbufferParameters(GLuint id);


// constants
int   SCREEN_WIDTH    = 400;
int   SCREEN_HEIGHT   = 300;
const float CAMERA_DISTANCE = 6.0f;
const int   TEXT_WIDTH      = 8;
const int   TEXT_HEIGHT     = 13;
int   TEXTURE_WIDTH   = 1920;  // NOTE: texture size cannot be larger than
int   TEXTURE_HEIGHT  = 1080;  // the rendering window size in non-FBO mode

// global variables
GLuint fboId;                       // ID of FBO
GLuint fbotextureId;                   // ID of texture
GLuint srctextureId;
GLuint rboColorId, rboDepthId;      // IDs of Renderbuffer objects
void *font = GLUT_BITMAP_8_BY_13;
int screenWidth;
int screenHeight;
bool mouseLeftDown;
bool mouseRightDown;
float mouseX, mouseY;
float cameraAngleX;
float cameraAngleY;
float cameraDistance;
bool fboSupported;
bool fboUsed;
int fboSampleCount;
int drawMode;
Timer timer, t1;
float playTime;                     // to compute rotation angle
float renderToTextureTime;          // elapsed time for render-to-texture


// function pointers for FBO extension
// Windows needs to get function pointers from ICD OpenGL drivers,
// because opengl32.dll does not support extensions higher than v1.1.
#ifdef _WIN32
// ARB Framebuffer object
PFNGLGENFRAMEBUFFERSPROC                     pglGenFramebuffers = 0;                      // FBO name generation procedure
PFNGLDELETEFRAMEBUFFERSPROC                  pglDeleteFramebuffers = 0;                   // FBO deletion procedure
PFNGLBINDFRAMEBUFFERPROC                     pglBindFramebuffer = 0;                      // FBO bind procedure
PFNGLCHECKFRAMEBUFFERSTATUSPROC              pglCheckFramebufferStatus = 0;               // FBO completeness test procedure
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC pglGetFramebufferAttachmentParameteriv = 0;  // return various FBO parameters
PFNGLGENERATEMIPMAPPROC                      pglGenerateMipmap = 0;                       // FBO automatic mipmap generation procedure
PFNGLFRAMEBUFFERTEXTURE1DPROC                pglFramebufferTexture1D = 0;                 // FBO texdture attachement procedure
PFNGLFRAMEBUFFERTEXTURE2DPROC                pglFramebufferTexture2D = 0;                 // FBO texdture attachement procedure
PFNGLFRAMEBUFFERTEXTURE3DPROC                pglFramebufferTexture3D = 0;                 // FBO texdture attachement procedure
PFNGLFRAMEBUFFERTEXTURELAYERPROC             pglFramebufferTextureLayer = 0;              // FBO texdture attachement procedure
PFNGLFRAMEBUFFERRENDERBUFFERPROC             pglFramebufferRenderbuffer = 0;              // FBO renderbuffer attachement procedure
PFNGLISFRAMEBUFFERPROC                       pglIsFramebuffer = 0;                        // FBO state = true/false
PFNGLBLITFRAMEBUFFERPROC                     pglBlitFramebuffer = 0;                      // FBO copy
// Renderbuffer object
PFNGLGENRENDERBUFFERSPROC                    pglGenRenderbuffers = 0;                     // renderbuffer generation procedure
PFNGLDELETERENDERBUFFERSPROC                 pglDeleteRenderbuffers = 0;                  // renderbuffer deletion procedure
PFNGLBINDRENDERBUFFERPROC                    pglBindRenderbuffer = 0;                     // renderbuffer bind procedure
PFNGLRENDERBUFFERSTORAGEPROC                 pglRenderbufferStorage = 0;                  // renderbuffer memory allocation procedure
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC      pglRenderbufferStorageMultisample = 0;       // renderbuffer memory allocation with multisample
PFNGLGETRENDERBUFFERPARAMETERIVPROC          pglGetRenderbufferParameteriv = 0;           // return various renderbuffer parameters
PFNGLISRENDERBUFFERPROC                      pglIsRenderbuffer = 0;                       // determine renderbuffer object type

#define glGenFramebuffers                        pglGenFramebuffers
#define glDeleteFramebuffers                     pglDeleteFramebuffers
#define glBindFramebuffer                        pglBindFramebuffer
#define glCheckFramebufferStatus                 pglCheckFramebufferStatus
#define glGetFramebufferAttachmentParameteriv    pglGetFramebufferAttachmentParameteriv
#define glGenerateMipmap                         pglGenerateMipmap
#define glFramebufferTexture1D                   pglFramebufferTexture1D
#define glFramebufferTexture2D                   pglFramebufferTexture2D
#define glFramebufferTexture3D                   pglFramebufferTexture3D
#define glFramebufferTextureLayer                pglFramebufferTextureLayer
#define glFramebufferRenderbuffer                pglFramebufferRenderbuffer
#define glIsFramebuffer                          pglIsFramebuffer
#define glBlitFramebuffer                        pglBlitFramebuffer

#define glGenRenderbuffers                       pglGenRenderbuffers
#define glDeleteRenderbuffers                    pglDeleteRenderbuffers
#define glBindRenderbuffer                       pglBindRenderbuffer
#define glRenderbufferStorage                    pglRenderbufferStorage
#define glRenderbufferStorageMultisample         pglRenderbufferStorageMultisample
#define glGetRenderbufferParameteriv             pglGetRenderbufferParameteriv
#define glIsRenderbuffer                         pglIsRenderbuffer

#endif


// function pointers for WGL_EXT_swap_control
#ifdef _WIN32
typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC) (int interval);
typedef int (WINAPI * PFNWGLGETSWAPINTERVALEXTPROC) (void);
PFNWGLSWAPINTERVALEXTPROC pwglSwapIntervalEXT = 0;
PFNWGLGETSWAPINTERVALEXTPROC pwglGetSwapIntervalEXT = 0;
#define wglSwapIntervalEXT      pwglSwapIntervalEXT
#define wglGetSwapIntervalEXT   pwglGetSwapIntervalEXT
#endif


///////////////////////////////////////////////////////////////////////////////
// draw a textured cube with GL_TRIANGLES
///////////////////////////////////////////////////////////////////////////////
void draw()
{
    glBindTexture(GL_TEXTURE_2D, fbotextureId);

    glColor4f(1, 1, 1, 1);
    // Draw a textured quad
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0, -1.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0, 1.0);
	glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, 1.0);
	glEnd();
    //~ just draw the texture on the screen instead of the cube below.
    
    
    //~ glBegin(GL_TRIANGLES);
        //~ // front faces
        //~ glNormal3f(0,0,1);
        //~ // face v0-v1-v2
        //~ glTexCoord2f(1,1);  glVertex3f(1,1,1);
        //~ glTexCoord2f(0,1);  glVertex3f(-1,1,1);
        //~ glTexCoord2f(0,0);  glVertex3f(-1,-1,1);
        //~ // face v2-v3-v0
        //~ glTexCoord2f(0,0);  glVertex3f(-1,-1,1);
        //~ glTexCoord2f(1,0);  glVertex3f(1,-1,1);
        //~ glTexCoord2f(1,1);  glVertex3f(1,1,1);

        //~ // right faces
        //~ glNormal3f(1,0,0);
        //~ // face v0-v3-v4
        //~ glTexCoord2f(0,1);  glVertex3f(1,1,1);
        //~ glTexCoord2f(0,0);  glVertex3f(1,-1,1);
        //~ glTexCoord2f(1,0);  glVertex3f(1,-1,-1);
        //~ // face v4-v5-v0
        //~ glTexCoord2f(1,0);  glVertex3f(1,-1,-1);
        //~ glTexCoord2f(1,1);  glVertex3f(1,1,-1);
        //~ glTexCoord2f(0,1);  glVertex3f(1,1,1);

        //~ // top faces
        //~ glNormal3f(0,1,0);
        //~ // face v0-v5-v6
        //~ glTexCoord2f(1,0);  glVertex3f(1,1,1);
        //~ glTexCoord2f(1,1);  glVertex3f(1,1,-1);
        //~ glTexCoord2f(0,1);  glVertex3f(-1,1,-1);
        //~ // face v6-v1-v0
        //~ glTexCoord2f(0,1);  glVertex3f(-1,1,-1);
        //~ glTexCoord2f(0,0);  glVertex3f(-1,1,1);
        //~ glTexCoord2f(1,0);  glVertex3f(1,1,1);

        //~ // left faces
        //~ glNormal3f(-1,0,0);
        //~ // face  v1-v6-v7
        //~ glTexCoord2f(1,1);  glVertex3f(-1,1,1);
        //~ glTexCoord2f(0,1);  glVertex3f(-1,1,-1);
        //~ glTexCoord2f(0,0);  glVertex3f(-1,-1,-1);
        //~ // face v7-v2-v1
        //~ glTexCoord2f(0,0);  glVertex3f(-1,-1,-1);
        //~ glTexCoord2f(1,0);  glVertex3f(-1,-1,1);
        //~ glTexCoord2f(1,1);  glVertex3f(-1,1,1);

        //~ // bottom faces
        //~ glNormal3f(0,-1,0);
        //~ // face v7-v4-v3
        //~ glTexCoord2f(0,0);  glVertex3f(-1,-1,-1);
        //~ glTexCoord2f(1,0);  glVertex3f(1,-1,-1);
        //~ glTexCoord2f(1,1);  glVertex3f(1,-1,1);
        //~ // face v3-v2-v7
        //~ glTexCoord2f(1,1);  glVertex3f(1,-1,1);
        //~ glTexCoord2f(0,1);  glVertex3f(-1,-1,1);
        //~ glTexCoord2f(0,0);  glVertex3f(-1,-1,-1);

        //~ // back faces
        //~ glNormal3f(0,0,-1);
        //~ // face v4-v7-v6
        //~ glTexCoord2f(0,0);  glVertex3f(1,-1,-1);
        //~ glTexCoord2f(1,0);  glVertex3f(-1,-1,-1);
        //~ glTexCoord2f(1,1);  glVertex3f(-1,1,-1);
        //~ // face v6-v5-v4
        //~ glTexCoord2f(1,1);  glVertex3f(-1,1,-1);
        //~ glTexCoord2f(0,1);  glVertex3f(1,1,-1);
        //~ glTexCoord2f(0,0);  glVertex3f(1,-1,-1);
    //~ glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
}



///////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
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
			infile >> windoww;
			infile >> tempstring;
			infile >> windowh;
			infile >> tempstring;
			infile >> texturew;
			infile >> tempstring;
			infile >> textureh;			
			infile >> tempstring;
			infile >> outputfourccstr;
			infile >> tempstring;
			infile >> strpathtowarpfile;
			infile.close();
			
		  }

	else std::cout << "Unable to open ini file, using defaults." << std::endl;
	
	std::cout << "Output codec type: " << outputfourccstr << std::endl;
	TEXTURE_WIDTH = outputw;
	TEXTURE_HEIGHT = outputh;
	SCREEN_WIDTH = windoww;
	SCREEN_HEIGHT = windowh;
	
		// video init
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
    const std::string NAME = OpenFileNamestr.substr(0, pAt) + "W" + ".avi";   // Form the new name with container
    int ex = static_cast<int>(inputVideo.get(CAP_PROP_FOURCC));     // Get Codec Type- Int form
    // Transform from int to char via Bitwise operators
    char EXT[] = {(char)(ex & 0XFF) , (char)((ex & 0XFF00) >> 8),(char)((ex & 0XFF0000) >> 16),(char)((ex & 0XFF000000) >> 24), 0};
    
    int inputw = (int) inputVideo.get(CAP_PROP_FRAME_WIDTH);
    int inputh = (int) inputVideo.get(CAP_PROP_FRAME_HEIGHT);
    Size S = Size(inputw, inputh);   // Acquire input size
                  
    Size Sout = Size(outputw,outputh);
    
    // initialize src with black frame, for the sake of initgl code
    src.create(inputh, inputw, CV_8UC3);	// (rows, columns, type)  
    src = 0;   
    
    nFrames = inputVideo.get(CAP_PROP_FRAME_COUNT);
    std::cout << "Input frame resolution: Width=" << S.width << "  Height=" << S.height
         << " of nr#: " << nFrames << std::endl;
    std::cout << "Input codec type: " << EXT << std::endl;
        
    t_start = time(NULL);
	fps = 0;
	
	ReadMesh(strpathtowarpfile);
		
    // init global vars
    initSharedMem();

    // register exit callback
    atexit(exitCB);

    // init GLUT and GL
    initGLUT(argc, argv);
    initGL();

    // create a texture object for fbo
    glGenTextures(1, &fbotextureId);
    glBindTexture(GL_TEXTURE_2D, fbotextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); // automatic mipmap generation included in OpenGL v1.4
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
    // BGRA doesn't work - framebuffer incomplete error.
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // create texture object for src - from GL2AviView
    glGenTextures(1, &srctextureId);
   
	glBindTexture(GL_TEXTURE_2D, srctextureId);
   
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); 
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	//~ glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texturew, textureh, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
    
    // get OpenGL info
    glInfo glInfo;
    glInfo.getInfo();
    //glInfo.printSelf();

#ifdef _WIN32
    // check if FBO is supported by your video card
    if(glInfo.isExtensionSupported("GL_ARB_framebuffer_object"))
    {
        // get pointers to GL functions
        glGenFramebuffers                     = (PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffers");
        glDeleteFramebuffers                  = (PFNGLDELETEFRAMEBUFFERSPROC)wglGetProcAddress("glDeleteFramebuffers");
        glBindFramebuffer                     = (PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer");
        glCheckFramebufferStatus              = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)wglGetProcAddress("glCheckFramebufferStatus");
        glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)wglGetProcAddress("glGetFramebufferAttachmentParameteriv");
        glGenerateMipmap                      = (PFNGLGENERATEMIPMAPPROC)wglGetProcAddress("glGenerateMipmap");
        glFramebufferTexture1D                = (PFNGLFRAMEBUFFERTEXTURE1DPROC)wglGetProcAddress("glFramebufferTexture1D");
        glFramebufferTexture2D                = (PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress("glFramebufferTexture2D");
        glFramebufferTexture3D                = (PFNGLFRAMEBUFFERTEXTURE3DPROC)wglGetProcAddress("glFramebufferTexture3D");
        glFramebufferTextureLayer             = (PFNGLFRAMEBUFFERTEXTURELAYERPROC)wglGetProcAddress("glFramebufferTextureLayer");
        glFramebufferRenderbuffer             = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)wglGetProcAddress("glFramebufferRenderbuffer");
        glIsFramebuffer                       = (PFNGLISFRAMEBUFFERPROC)wglGetProcAddress("glIsFramebuffer");
        glBlitFramebuffer                     = (PFNGLBLITFRAMEBUFFERPROC)wglGetProcAddress("glBlitFramebuffer");
        glGenRenderbuffers                    = (PFNGLGENRENDERBUFFERSPROC)wglGetProcAddress("glGenRenderbuffers");
        glDeleteRenderbuffers                 = (PFNGLDELETERENDERBUFFERSPROC)wglGetProcAddress("glDeleteRenderbuffers");
        glBindRenderbuffer                    = (PFNGLBINDRENDERBUFFERPROC)wglGetProcAddress("glBindRenderbuffer");
        glRenderbufferStorage                 = (PFNGLRENDERBUFFERSTORAGEPROC)wglGetProcAddress("glRenderbufferStorage");
        glRenderbufferStorageMultisample      = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)wglGetProcAddress("glRenderbufferStorageMultisample");
        glGetRenderbufferParameteriv          = (PFNGLGETRENDERBUFFERPARAMETERIVPROC)wglGetProcAddress("glGetRenderbufferParameteriv");
        glIsRenderbuffer                      = (PFNGLISRENDERBUFFERPROC)wglGetProcAddress("glIsRenderbuffer");

        // check once again FBO extension
        if(glGenFramebuffers && glDeleteFramebuffers && glBindFramebuffer && glCheckFramebufferStatus &&
           glGetFramebufferAttachmentParameteriv && glGenerateMipmap && glFramebufferTexture1D && glFramebufferTexture2D && glFramebufferTexture3D &&
           glFramebufferTextureLayer && glFramebufferRenderbuffer && glIsFramebuffer && glBlitFramebuffer &&
           glGenRenderbuffers && glDeleteRenderbuffers && glBindRenderbuffer && glRenderbufferStorage &&
           glRenderbufferStorageMultisample && glGetRenderbufferParameteriv && glIsRenderbuffer)
        {
            fboSupported = fboUsed = true;
            std::cout << "Video card supports GL_ARB_framebuffer_object." << std::endl;
        }
        else
        {
            fboSupported = fboUsed = false;
            std::cout << "Video card does NOT support GL_ARB_framebuffer_object." << std::endl;
        }
    }

    // check EXT_swap_control is supported
    if(glInfo.isExtensionSupported("WGL_EXT_swap_control"))
    {
        // get pointers to WGL functions
        wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
        wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");
        if(wglSwapIntervalEXT && wglGetSwapIntervalEXT)
        {
            // disable v-sync
            wglSwapIntervalEXT(0);
            std::cout << "Video card supports WGL_EXT_swap_control." << std::endl;
        }
    }

#else // for linux, do not need to get function pointers, it is up-to-date
    if(glInfo.isExtensionSupported("GL_ARB_framebuffer_object"))
    {
        fboSupported = fboUsed = true;
        std::cout << "Video card supports GL_ARB_framebuffer_object." << std::endl;
    }
    else
    {
        fboSupported = fboUsed = false;
        std::cout << "Video card does NOT support GL_ARB_framebuffer_object." << std::endl;
    }
#endif

    if(fboSupported)
    {
        // create a framebuffer object, you need to delete them when program exits.
        glGenFramebuffers(1, &fboId);
        glBindFramebuffer(GL_FRAMEBUFFER, fboId);

        // create a renderbuffer object to store depth info
        // NOTE: A depth renderable image should be attached the FBO for depth test.
        // If we don't attach a depth renderable image to the FBO, then
        // the rendering output will be corrupted because of missing depth test.
        // If you also need stencil test for your rendering, then you must
        // attach additional image to the stencil attachement point, too.
        glGenRenderbuffers(1, &rboDepthId);
        glBindRenderbuffer(GL_RENDERBUFFER, rboDepthId);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, TEXTURE_WIDTH, TEXTURE_HEIGHT);
        //glRenderbufferStorageMultisample(GL_RENDERBUFFER, fboSampleCount, GL_DEPTH_COMPONENT, TEXTURE_WIDTH, TEXTURE_HEIGHT);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        // attach a texture to FBO color attachement point
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbotextureId, 0);
        //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);

        // attach a renderbuffer to depth attachment point
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepthId);

        //@@ disable color buffer if you don't attach any color buffer image,
        //@@ for example, rendering the depth buffer only to a texture.
        //@@ Otherwise, glCheckFramebufferStatus will not be complete.
        //glDrawBuffer(GL_NONE);
        //glReadBuffer(GL_NONE);

        // check FBO status
        printFramebufferInfo(fboId);
        bool status = checkFramebufferStatus(fboId);
        if(!status)
            fboUsed = false;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    //debug
    //fboUsed = 0;
    
    if (fboUsed)
    {
		// for export
		dst.create(TEXTURE_HEIGHT, TEXTURE_WIDTH, CV_8UC4);	// (rows, columns, type)
		// https://stackoverflow.com/questions/9097756/converting-data-from-glreadpixels-to-opencvmat/9098883
		//use fast 4-byte alignment (default anyway) if possible
		glPixelStorei(GL_PACK_ALIGNMENT, (dst.step & 3) ? 1 : 4);
		//set length of one complete row in destination data (doesn't need to equal img.cols)
		glPixelStorei(GL_PACK_ROW_LENGTH, dst.step/dst.elemSize());
		
		if (!(outputfourccstr[0] == 'N' &&
				outputfourccstr[1] == 'U' &&
				outputfourccstr[2] == 'L' &&
				outputfourccstr[3] == 'L'))
        outputVideo.open(NAME, outputVideo.fourcc(outputfourccstr[0], outputfourccstr[1], outputfourccstr[2], outputfourccstr[3]), 
        inputVideo.get(CAP_PROP_FPS), Size(TEXTURE_WIDTH,TEXTURE_HEIGHT), true);
		else
        outputVideo.open(NAME, ex, inputVideo.get(CAP_PROP_FPS), Size(TEXTURE_WIDTH,TEXTURE_HEIGHT), true);
    
	}
	else
	{
		// SCREEN_HEIGHT instead of TEXTURE_HEIGHT etc
		dst.create(SCREEN_HEIGHT, SCREEN_WIDTH, CV_8UC4);	// (rows, columns, type)
		// https://stackoverflow.com/questions/9097756/converting-data-from-glreadpixels-to-opencvmat/9098883
		//use fast 4-byte alignment (default anyway) if possible
		glPixelStorei(GL_PACK_ALIGNMENT, (dst.step & 3) ? 1 : 4);
		//set length of one complete row in destination data (doesn't need to equal img.cols)
		glPixelStorei(GL_PACK_ROW_LENGTH, dst.step/dst.elemSize());
		
		if (!(outputfourccstr[0] == 'N' &&
				outputfourccstr[1] == 'U' &&
				outputfourccstr[2] == 'L' &&
				outputfourccstr[3] == 'L'))
        outputVideo.open(NAME, outputVideo.fourcc(outputfourccstr[0], outputfourccstr[1], outputfourccstr[2], outputfourccstr[3]), 
        inputVideo.get(CAP_PROP_FPS), Size(SCREEN_WIDTH,SCREEN_HEIGHT), true);
		else
        outputVideo.open(NAME, ex, inputVideo.get(CAP_PROP_FPS), Size(SCREEN_WIDTH,SCREEN_HEIGHT), true);
	}
	
		

    // start timer
    timer.start();

    glutMainLoop(); /* Start GLUT event-processing loop */

    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// initialize GLUT for windowing
///////////////////////////////////////////////////////////////////////////////
int initGLUT(int argc, char **argv)
{
    // GLUT stuff for windowing
    // initialization openGL window.
    // It must be called before any other GLUT routine.
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);   // display mode

    glutInitWindowSize(screenWidth, screenHeight);              // window size

    glutInitWindowPosition(100, 100);                           // window location

    // finally, create a window with openGL context
    // Window will not displayed until glutMainLoop() is called
    // It returns a unique ID.
    int handle = glutCreateWindow(argv[0]);     // param is the title of window

    // register GLUT callback functions
    glutDisplayFunc(displayCB);
    //glutTimerFunc(33, timerCB, 33);             // redraw only every given millisec
    glutIdleFunc(idleCB);                       // redraw whenever system is idle
    glutReshapeFunc(reshapeCB);
    glutKeyboardFunc(keyboardCB);
    glutMouseFunc(mouseCB);
    glutMotionFunc(mouseMotionCB);

    return handle;
}



///////////////////////////////////////////////////////////////////////////////
// initialize OpenGL
// disable unused features
///////////////////////////////////////////////////////////////////////////////
void initGL()
{
    glShadeModel(GL_SMOOTH);                    // shading mathod: GL_SMOOTH or GL_FLAT
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);      // 4-byte pixel alignment

    // enable /disable features
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);

     // track material ambient and diffuse from surface color, call it before glEnable(GL_COLOR_MATERIAL)
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);

    glClearColor(0, 0, 0, 0);                   // background color
    glClearStencil(0);                          // clear stencil buffer
    glClearDepth(1.0f);                         // 0 is near, 1 is far
    glDepthFunc(GL_LEQUAL);

    initLights();
}



///////////////////////////////////////////////////////////////////////////////
// write 2d text using GLUT
// The projection matrix must be set to orthogonal before call this function.
///////////////////////////////////////////////////////////////////////////////
void drawString(const char *str, int x, int y, float color[4], void *font)
{
    glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT); // lighting and color mask
    glDisable(GL_LIGHTING);     // need to disable lighting for proper text color
    glDisable(GL_TEXTURE_2D);

    glColor4fv(color);          // set text color
    glRasterPos2i(x, y);        // place text position

    // loop all characters in the string
    while(*str)
    {
        glutBitmapCharacter(font, *str);
        ++str;
    }

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glPopAttrib();
}



///////////////////////////////////////////////////////////////////////////////
// draw a string in 3D space
///////////////////////////////////////////////////////////////////////////////
void drawString3D(const char *str, float pos[3], float color[4], void *font)
{
    glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT); // lighting and color mask
    glDisable(GL_LIGHTING);     // need to disable lighting for proper text color
    glDisable(GL_TEXTURE_2D);

    glColor4fv(color);          // set text color
    glRasterPos3fv(pos);        // place text position

    // loop all characters in the string
    while(*str)
    {
        glutBitmapCharacter(font, *str);
        ++str;
    }

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glPopAttrib();
}



///////////////////////////////////////////////////////////////////////////////
// initialize global variables
///////////////////////////////////////////////////////////////////////////////
bool initSharedMem()
{
    screenWidth = SCREEN_WIDTH;
    screenHeight = SCREEN_HEIGHT;

    mouseLeftDown = mouseRightDown = false;
    mouseX = mouseY = 0;

    cameraAngleX = cameraAngleY = 45;
    cameraDistance = 4;

    drawMode = 0; // 0:fill, 1: wireframe, 2:points

    fboId = rboColorId = rboDepthId = fbotextureId = srctextureId = 0;
    fboSupported = fboUsed = false;
    playTime = renderToTextureTime = 0;

    return true;
}



///////////////////////////////////////////////////////////////////////////////
// clean up global variables
///////////////////////////////////////////////////////////////////////////////
void clearSharedMem()
{
	free(mesh);
    glDeleteTextures(1, &fbotextureId);
    glDeleteTextures(1, &srctextureId);
    srctextureId = fbotextureId = 0;

    // clean up FBO, RBO
    if(fboSupported)
    {
        glDeleteFramebuffers(1, &fboId);
        fboId = 0;
        glDeleteRenderbuffers(1, &rboDepthId);
        rboDepthId = 0;
    }
    std::cout << std::endl << "Finished writing." << std::endl;
}



///////////////////////////////////////////////////////////////////////////////
// initialize lights
///////////////////////////////////////////////////////////////////////////////
void initLights()
{
    // set up light colors (ambient, diffuse, specular)
    GLfloat lightKa[] = {.2f, .2f, .2f, 1.0f};  // ambient light
    GLfloat lightKd[] = {.7f, .7f, .7f, 1.0f};  // diffuse light
    GLfloat lightKs[] = {1, 1, 1, 1};           // specular light
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightKa);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightKd);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightKs);

    // position the light
    float lightPos[4] = {0, 0, 20, 1}; // positional light
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    glEnable(GL_LIGHT0);                        // MUST enable each light source after configuration
}



///////////////////////////////////////////////////////////////////////////////
// set camera position and lookat direction
///////////////////////////////////////////////////////////////////////////////
void setCamera(float posX, float posY, float posZ, float targetX, float targetY, float targetZ)
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(posX, posY, posZ, targetX, targetY, targetZ, 0, 1, 0); // eye(x,y,z), focal(x,y,z), up(x,y,z)
}



///////////////////////////////////////////////////////////////////////////////
// display info messages
///////////////////////////////////////////////////////////////////////////////
void showInfo()
{
    // backup current model-view matrix
    glPushMatrix();                     // save current modelview matrix
    glLoadIdentity();                   // reset modelview matrix

    // set to 2D orthogonal projection
    glMatrixMode(GL_PROJECTION);        // switch to projection matrix
    glPushMatrix();                     // save current projection matrix
    glLoadIdentity();                   // reset projection matrix
    gluOrtho2D(0, screenWidth, 0, screenHeight);  // set to orthogonal projection

    float color[4] = {1, 1, 1, 1};

    stringstream ss;
    ss << "FBO: ";
    if(fboUsed)
        ss << "on" << ends;
    else
        ss << "off" << ends;

    drawString(ss.str().c_str(), 1, screenHeight-TEXT_HEIGHT, color, font);
    ss.str(""); // clear buffer

    ss << std::fixed << std::setprecision(3);
    ss << "Render-To-Texture Time: " << renderToTextureTime << " ms" << ends;
    drawString(ss.str().c_str(), 1, screenHeight-(2*TEXT_HEIGHT), color, font);
    ss.str("");

    ss << "If FBO not used, output has to be smaller than screen size." << ends;
    drawString(ss.str().c_str(), 1, 1, color, font);

    // unset floating format
    ss << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield);

    // restore projection matrix
    glPopMatrix();                   // restore to previous projection matrix

    // restore modelview matrix
    glMatrixMode(GL_MODELVIEW);      // switch to modelview matrix
    glPopMatrix();                   // restore to previous modelview matrix
}



///////////////////////////////////////////////////////////////////////////////
// display frame rates
///////////////////////////////////////////////////////////////////////////////
void showFPS()
{
    static Timer timer;
    static int count = 0;
    static std::string fps = "0.0 FPS";
    double elapsedTime = 0.0;;

    ++count;

    // backup current model-view matrix
    glPushMatrix();                     // save current modelview matrix
    glLoadIdentity();                   // reset modelview matrix

    // set to 2D orthogonal projection
    glMatrixMode(GL_PROJECTION);        // switch to projection matrix
    glPushMatrix();                     // save current projection matrix
    glLoadIdentity();                   // reset projection matrix
    gluOrtho2D(0, screenWidth, 0, screenHeight); // set to orthogonal projection

    float color[4] = {1, 1, 0, 1};

    // update fps every second
    elapsedTime = timer.getElapsedTime();
    if(elapsedTime >= 1.0)
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1);
        ss << (count / elapsedTime) << " FPS" << std::ends; // update fps string
        ss << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield);
        fps = ss.str();
        count = 0;                      // reset counter
        timer.start();                  // restart timer
    }
    int textWidth = (int)fps.size() * TEXT_WIDTH;
    drawString(fps.c_str(), screenWidth-textWidth, screenHeight-TEXT_HEIGHT, color, font);

    // restore projection matrix
    glPopMatrix();                      // restore to previous projection matrix

    // restore modelview matrix
    glMatrixMode(GL_MODELVIEW);         // switch to modelview matrix
    glPopMatrix();                      // restore to previous modelview matrix
}



///////////////////////////////////////////////////////////////////////////////
// check FBO completeness
///////////////////////////////////////////////////////////////////////////////
bool checkFramebufferStatus(GLuint fbo)
{
    // check FBO status
    glBindFramebuffer(GL_FRAMEBUFFER, fbo); // bind
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    switch(status)
    {
    case GL_FRAMEBUFFER_COMPLETE:
        std::cout << "Framebuffer complete." << std::endl;
        return true;

    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        std::cout << "[ERROR] Framebuffer incomplete: Attachment is NOT complete." << std::endl;
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        std::cout << "[ERROR] Framebuffer incomplete: No image is attached to FBO." << std::endl;
        return false;
/*
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
        std::cout << "[ERROR] Framebuffer incomplete: Attached images have different dimensions." << std::endl;
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
        std::cout << "[ERROR] Framebuffer incomplete: Color attached images have different internal formats." << std::endl;
        return false;
*/
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        std::cout << "[ERROR] Framebuffer incomplete: Draw buffer." << std::endl;
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        std::cout << "[ERROR] Framebuffer incomplete: Read buffer." << std::endl;
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        std::cout << "[ERROR] Framebuffer incomplete: Multisample." << std::endl;
        return false;

    case GL_FRAMEBUFFER_UNSUPPORTED:
        std::cout << "[ERROR] Framebuffer incomplete: Unsupported by FBO implementation." << std::endl;
        return false;

    default:
        std::cout << "[ERROR] Framebuffer incomplete: Unknown error." << std::endl;
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);   // unbind
}



///////////////////////////////////////////////////////////////////////////////
// print out the FBO infos
///////////////////////////////////////////////////////////////////////////////
void printFramebufferInfo(GLuint fbo)
{
    // bind fbo
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    std::cout << "\n===== FBO STATUS =====\n";

    // print max # of colorbuffers supported by FBO
    int colorBufferCount = 0;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &colorBufferCount);
    std::cout << "Max Number of Color Buffer Attachment Points: " << colorBufferCount << std::endl;

    // get max # of multi samples
    int multiSampleCount = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &multiSampleCount);
    std::cout << "Max Number of Samples for MSAA: " << multiSampleCount << std::endl;

    int objectType;
    int objectId;

    // print info of the colorbuffer attachable image
    for(int i = 0; i < colorBufferCount; ++i)
    {
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,
                                              GL_COLOR_ATTACHMENT0+i,
                                              GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
                                              &objectType);
        if(objectType != GL_NONE)
        {
            glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,
                                                  GL_COLOR_ATTACHMENT0+i,
                                                  GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
                                                  &objectId);

            std::string formatName;

            std::cout << "Color Attachment " << i << ": ";
            if(objectType == GL_TEXTURE)
            {
                std::cout << "GL_TEXTURE, " << getTextureParameters(objectId) << std::endl;
            }
            else if(objectType == GL_RENDERBUFFER)
            {
                std::cout << "GL_RENDERBUFFER, " << getRenderbufferParameters(objectId) << std::endl;
            }
        }
    }

    // print info of the depthbuffer attachable image
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,
                                          GL_DEPTH_ATTACHMENT,
                                          GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
                                          &objectType);
    if(objectType != GL_NONE)
    {
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,
                                              GL_DEPTH_ATTACHMENT,
                                              GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
                                              &objectId);

        std::cout << "Depth Attachment: ";
        switch(objectType)
        {
        case GL_TEXTURE:
            std::cout << "GL_TEXTURE, " << getTextureParameters(objectId) << std::endl;
            break;
        case GL_RENDERBUFFER:
            std::cout << "GL_RENDERBUFFER, " << getRenderbufferParameters(objectId) << std::endl;
            break;
        }
    }

    // print info of the stencilbuffer attachable image
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,
                                          GL_STENCIL_ATTACHMENT,
                                          GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
                                          &objectType);
    if(objectType != GL_NONE)
    {
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,
                                              GL_STENCIL_ATTACHMENT,
                                              GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
                                              &objectId);

        std::cout << "Stencil Attachment: ";
        switch(objectType)
        {
        case GL_TEXTURE:
            std::cout << "GL_TEXTURE, " << getTextureParameters(objectId) << std::endl;
            break;
        case GL_RENDERBUFFER:
            std::cout << "GL_RENDERBUFFER, " << getRenderbufferParameters(objectId) << std::endl;
            break;
        }
    }

    std::cout << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}



///////////////////////////////////////////////////////////////////////////////
// return texture parameters as string using glGetTexLevelParameteriv()
///////////////////////////////////////////////////////////////////////////////
std::string getTextureParameters(GLuint id)
{
    if(glIsTexture(id) == GL_FALSE)
        return "Not texture object";

    int width, height, format;
    std::string formatName;
    glBindTexture(GL_TEXTURE_2D, id);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);            // get texture width
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);          // get texture height
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format); // get texture internal format
    glBindTexture(GL_TEXTURE_2D, 0);

    formatName = convertInternalFormatToString(format);

    std::stringstream ss;
    ss << width << "x" << height << ", " << formatName;
    return ss.str();
}



///////////////////////////////////////////////////////////////////////////////
// return renderbuffer parameters as string using glGetRenderbufferParameteriv
///////////////////////////////////////////////////////////////////////////////
std::string getRenderbufferParameters(GLuint id)
{
    if(glIsRenderbuffer(id) == GL_FALSE)
        return "Not Renderbuffer object";

    int width, height, format, samples;
    std::string formatName;
    glBindRenderbuffer(GL_RENDERBUFFER, id);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);       // get renderbuffer width
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);     // get renderbuffer height
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &format); // get renderbuffer internal format
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &samples);   // get multisample count
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    formatName = convertInternalFormatToString(format);

    std::stringstream ss;
    ss << width << "x" << height << ", " << formatName << ", MSAA(" << samples << ")";
    return ss.str();
}



///////////////////////////////////////////////////////////////////////////////
// convert OpenGL internal format enum to string
///////////////////////////////////////////////////////////////////////////////
std::string convertInternalFormatToString(GLenum format)
{
    std::string formatName;

    switch(format)
    {
    case GL_STENCIL_INDEX:      // 0x1901
        formatName = "GL_STENCIL_INDEX";
        break;
    case GL_DEPTH_COMPONENT:    // 0x1902
        formatName = "GL_DEPTH_COMPONENT";
        break;
    case GL_ALPHA:              // 0x1906
        formatName = "GL_ALPHA";
        break;
    case GL_RGB:                // 0x1907
        formatName = "GL_RGB";
        break;
    case GL_RGBA:               // 0x1908
        formatName = "GL_RGBA";
        break;
    case GL_LUMINANCE:          // 0x1909
        formatName = "GL_LUMINANCE";
        break;
    case GL_LUMINANCE_ALPHA:    // 0x190A
        formatName = "GL_LUMINANCE_ALPHA";
        break;
    case GL_R3_G3_B2:           // 0x2A10
        formatName = "GL_R3_G3_B2";
        break;
    case GL_ALPHA4:             // 0x803B
        formatName = "GL_ALPHA4";
        break;
    case GL_ALPHA8:             // 0x803C
        formatName = "GL_ALPHA8";
        break;
    case GL_ALPHA12:            // 0x803D
        formatName = "GL_ALPHA12";
        break;
    case GL_ALPHA16:            // 0x803E
        formatName = "GL_ALPHA16";
        break;
    case GL_LUMINANCE4:         // 0x803F
        formatName = "GL_LUMINANCE4";
        break;
    case GL_LUMINANCE8:         // 0x8040
        formatName = "GL_LUMINANCE8";
        break;
    case GL_LUMINANCE12:        // 0x8041
        formatName = "GL_LUMINANCE12";
        break;
    case GL_LUMINANCE16:        // 0x8042
        formatName = "GL_LUMINANCE16";
        break;
    case GL_LUMINANCE4_ALPHA4:  // 0x8043
        formatName = "GL_LUMINANCE4_ALPHA4";
        break;
    case GL_LUMINANCE6_ALPHA2:  // 0x8044
        formatName = "GL_LUMINANCE6_ALPHA2";
        break;
    case GL_LUMINANCE8_ALPHA8:  // 0x8045
        formatName = "GL_LUMINANCE8_ALPHA8";
        break;
    case GL_LUMINANCE12_ALPHA4: // 0x8046
        formatName = "GL_LUMINANCE12_ALPHA4";
        break;
    case GL_LUMINANCE12_ALPHA12:// 0x8047
        formatName = "GL_LUMINANCE12_ALPHA12";
        break;
    case GL_LUMINANCE16_ALPHA16:// 0x8048
        formatName = "GL_LUMINANCE16_ALPHA16";
        break;
    case GL_INTENSITY:          // 0x8049
        formatName = "GL_INTENSITY";
        break;
    case GL_INTENSITY4:         // 0x804A
        formatName = "GL_INTENSITY4";
        break;
    case GL_INTENSITY8:         // 0x804B
        formatName = "GL_INTENSITY8";
        break;
    case GL_INTENSITY12:        // 0x804C
        formatName = "GL_INTENSITY12";
        break;
    case GL_INTENSITY16:        // 0x804D
        formatName = "GL_INTENSITY16";
        break;
    case GL_RGB4:               // 0x804F
        formatName = "GL_RGB4";
        break;
    case GL_RGB5:               // 0x8050
        formatName = "GL_RGB5";
        break;
    case GL_RGB8:               // 0x8051
        formatName = "GL_RGB8";
        break;
    case GL_RGB10:              // 0x8052
        formatName = "GL_RGB10";
        break;
    case GL_RGB12:              // 0x8053
        formatName = "GL_RGB12";
        break;
    case GL_RGB16:              // 0x8054
        formatName = "GL_RGB16";
        break;
    case GL_RGBA2:              // 0x8055
        formatName = "GL_RGBA2";
        break;
    case GL_RGBA4:              // 0x8056
        formatName = "GL_RGBA4";
        break;
    case GL_RGB5_A1:            // 0x8057
        formatName = "GL_RGB5_A1";
        break;
    case GL_RGBA8:              // 0x8058
        formatName = "GL_RGBA8";
        break;
    case GL_RGB10_A2:           // 0x8059
        formatName = "GL_RGB10_A2";
        break;
    case GL_RGBA12:             // 0x805A
        formatName = "GL_RGBA12";
        break;
    case GL_RGBA16:             // 0x805B
        formatName = "GL_RGBA16";
        break;
    case GL_DEPTH_COMPONENT16:  // 0x81A5
        formatName = "GL_DEPTH_COMPONENT16";
        break;
    case GL_DEPTH_COMPONENT24:  // 0x81A6
        formatName = "GL_DEPTH_COMPONENT24";
        break;
    case GL_DEPTH_COMPONENT32:  // 0x81A7
        formatName = "GL_DEPTH_COMPONENT32";
        break;
    case GL_DEPTH_STENCIL:      // 0x84F9
        formatName = "GL_DEPTH_STENCIL";
        break;
    case GL_RGBA32F:            // 0x8814
        formatName = "GL_RGBA32F";
        break;
    case GL_RGB32F:             // 0x8815
        formatName = "GL_RGB32F";
        break;
    case GL_RGBA16F:            // 0x881A
        formatName = "GL_RGBA16F";
        break;
    case GL_RGB16F:             // 0x881B
        formatName = "GL_RGB16F";
        break;
    case GL_DEPTH24_STENCIL8:   // 0x88F0
        formatName = "GL_DEPTH24_STENCIL8";
        break;
    default:
        std::stringstream ss;
        ss << "Unknown Format(0x" << std::hex << format << ")" << std::ends;
        formatName = ss.str();
    }

    return formatName;
}



///////////////////////////////////////////////////////////////////////////////
// set projection matrix as orthogonal
///////////////////////////////////////////////////////////////////////////////
void toOrtho()
{
    // set viewport to be the entire window
    glViewport(0, 0, (GLsizei)screenWidth, (GLsizei)screenHeight);

    // set orthographic viewing frustum
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, screenWidth, 0, screenHeight, -1, 1);

    // switch to modelview matrix in order to set scene
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}



///////////////////////////////////////////////////////////////////////////////
// set the projection matrix as perspective
///////////////////////////////////////////////////////////////////////////////
void toPerspective()
{
    // set viewport to be the entire window
    glViewport(0, 0, (GLsizei)screenWidth, (GLsizei)screenHeight);

    // set perspective viewing frustum
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0f, (float)(screenWidth)/screenHeight, 1.0f, 1000.0f); // FOV, AspectRatio, NearClip, FarClip

    // switch to modelview matrix in order to set scene
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}







//=============================================================================
// CALLBACKS
//=============================================================================

void displayCB()
{
    // get the total elapsed time
    playTime = (float)timer.getElapsedTime();

    // compute rotation angle
    const float ANGLE_SPEED = 90;   // degree/s
    float angle = ANGLE_SPEED * playTime;

    // render to texture //////////////////////////////////////////////////////
    t1.start();

    // adjust viewport and projection matrix to texture dimension
    glViewport(0, 0, TEXTURE_WIDTH, TEXTURE_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0f, (float)(TEXTURE_WIDTH)/TEXTURE_HEIGHT, 1.0f, 100.0f);
    glMatrixMode(GL_MODELVIEW);

    // camera transform
    glLoadIdentity();
    glTranslatef(0, 0, -CAMERA_DISTANCE);

    // with FBO
    // render directly to a texture
    if(fboUsed)
    {
        // set the rendering destination to FBO
        glBindFramebuffer(GL_FRAMEBUFFER, fboId);

        // clear buffer
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw a rotating teapot at the origin
        //~ glPushMatrix();
        //~ glRotatef(angle*0.5f, 1, 0, 0);
        //~ glRotatef(angle, 0, 1, 0);
        //~ glRotatef(angle*0.7f, 0, 0, 1);
        //~ glTranslatef(0, -1.575f, 0);
        //~ drawTeapot();
        //~ glPopMatrix();
        
        // draw frame from video onto texture
        getNextFrame();
        CreateGrid();
        
        glReadPixels(0, 0, TEXTURE_WIDTH, TEXTURE_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, dst.data);
		//~ //glReadPixels(0,0,lpbih->biWidth,lpbih->biHeight,GL_BGR_EXT,GL_UNSIGNED_BYTE,bmBits);
		//~ // GL_RGBA8 makes it much faster.
		cvtColor(dst, dstbgr, CV_RGBA2BGR);
		flip(dstbgr, flipped, 0);
		outputVideo << flipped;

        // back to normal window-system-provided framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // unbind

        // trigger mipmaps generation explicitly
        // NOTE: If GL_GENERATE_MIPMAP is set to GL_TRUE, then glCopyTexSubImage2D()
        // triggers mipmap generation automatically. However, the texture attached
        // onto a FBO should generate mipmaps manually via glGenerateMipmap().
        glBindTexture(GL_TEXTURE_2D, fbotextureId);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // without FBO
    // render to the backbuffer and copy the backbuffer to a texture
    else
    {
        // clear buffer
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glPushAttrib(GL_COLOR_BUFFER_BIT | GL_PIXEL_MODE_BIT); // for GL_DRAW_BUFFER and GL_READ_BUFFER
        glDrawBuffer(GL_BACK);
        glReadBuffer(GL_BACK);

        // draw a rotating teapot at the origin
        //~ glPushMatrix();
        //~ glRotatef(angle*0.5f, 1, 0, 0);
        //~ glRotatef(angle, 0, 1, 0);
        //~ glRotatef(angle*0.7f, 0, 0, 1);
        //~ glTranslatef(0, -1.575f, 0);
        //~ drawTeapot();
        //~ glPopMatrix();
        
        // draw frame from video onto texture
        getNextFrame();
        CreateGrid();
        
        glReadPixels(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_BGRA, GL_UNSIGNED_BYTE, dst.data);
		//glReadPixels(0,0,lpbih->biWidth,lpbih->biHeight,GL_BGR_EXT,GL_UNSIGNED_BYTE,bmBits);
		// GL_BGRA makes it much faster.
		cvtColor(dst, dstbgr, CV_BGRA2BGR);
		flip(dstbgr, flipped, 0);
		outputVideo << flipped;

        // copy the framebuffer pixels to a texture
        glBindTexture(GL_TEXTURE_2D, fbotextureId);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, TEXTURE_WIDTH, TEXTURE_HEIGHT);
        glBindTexture(GL_TEXTURE_2D, 0);

        glPopAttrib(); // GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT
    }

    // measure the elapsed time of render-to-texture
    t1.stop();
    renderToTextureTime = t1.getElapsedTimeInMilliSec();
    ///////////////////////////////////////////////////////////////////////////


    // rendering as normal ////////////////////////////////////////////////////

    // back to normal viewport and projection matrix
    glViewport(0, 0, screenWidth, screenHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //~ gluPerspective(60.0f, (float)(screenWidth)/screenHeight, 1.0f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //~ // tramsform camera
    //~ glTranslatef(0, 0, -cameraDistance);
    //~ glRotatef(cameraAngleX, 1, 0, 0);   // pitch
    //~ glRotatef(cameraAngleY, 0, 1, 0);   // heading

    // clear framebuffer
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glPushMatrix();

    // draw with the dynamic texture
    draw();

    glPopMatrix();

    // draw info messages
    showInfo();
    showFPS();
    glutSwapBuffers();
}


void reshapeCB(int width, int height)
{
	//~ // don't resize, as this will break the exported video
    //~ screenWidth = width;
    //~ screenHeight = height;
    //~ toPerspective();
}


void timerCB(int millisec)
{
    glutTimerFunc(millisec, timerCB, millisec);
    glutPostRedisplay();
}


void idleCB()
{
    glutPostRedisplay();
}


void keyboardCB(unsigned char key, int x, int y)
{
    switch(key)
    {
    case 27: // ESCAPE
        exit(0);
        break;

    
    default:
        ;
    }
}


void mouseCB(int button, int state, int x, int y)
{
    
}


void mouseMotionCB(int x, int y)
{
    
}


void exitCB()
{
    clearSharedMem();
}

void getNextFrame()
{
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, srctextureId);
	// Capture next frame
	inputVideo >> src; // gets the next frame into image
	if (src.empty()) // end of video;
	{
		//onExitCleanup();
		//clearSharedMem(); no need to call it, it is called as a callback
		exit(0);
	}

	// update Texture
	flip(src, flipped, 0);	// flip up down
	resize(flipped, srcres, Size(texturew,textureh), 0, 0, INTER_CUBIC);
	returncode = gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, srcres.cols, srcres.rows, GL_BGR, GL_UNSIGNED_BYTE, srcres.data);
	if (returncode)	// if success, returncode=0
		std::cout << "Errorcode for gluBuild2DMipmaps = " << returncode;
	
	
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

bool ReadMesh(std::string strpathtowarpfile)
{
	//from https://github.com/hn-88/GL_warp2Avi/blob/master/GL2AviView.cpp
	FILE *input = NULL;

   input = fopen(strpathtowarpfile.c_str(), "r");

   /* Set rows and columns to 2 initially, as this is the size of the default mesh. */
    int dummy, rows = 2, cols = 2;

    if (input != NULL)  {
		fscanf(input, " %d %d %d ", &dummy, &cols, &rows) ;
		float x, y, u, v, l;
		meshrows=rows;
		meshcolumns=cols;

		// the following is adapted from the code at
		// http://paulbourke.net/dataformats/meshwarp/


		mesh		= (meshpoint*)calloc(rows*cols, sizeof(meshpoint));
		 
		//  (meshpoint*) explicit cast required for c++
		//	  have to do an explicit cast
		// since this is C++ and not C
	// http://cboard.cprogramming.com/windows-programming/69549-%27initializing%27-cannot-convert-%27void-*%27-%27int-*%27.html


	     for (int r = 0; r < rows ; r++) {
             for (int c = 0; c < cols ; c++) {
		                
                fscanf(input, "%f %f %f %f %f", &x, &y, &u, &v, &l) ;                 
                //   using the code adapted from 
				// http://paulbourke.net/dataformats/meshwarp/
				// and from vlc-warp
				//   We pack the values for each node into a 1d array.  
    
                mesh[cols*r+c].x = x;
                mesh[cols*r+c].y = y;
                mesh[cols*r+c].u = u;
                mesh[cols*r+c].v = v;
                mesh[cols*r+c].i = l;
 
				

			}
			 
		}
	}
	else // unable to read mesh
	{
		std::cout << "Unable to read mesh data file (similar to EP_xyuv_1920.map), exiting!" << std::endl;
		//onExitCleanup();
		//clearSharedMem(); no need to explicitly call it.
		exit(0);
	}
}
		

void CreateGrid()
{
	// Set Projection Matrix
	//~ glMatrixMode (GL_PROJECTION);
	//~ glLoadIdentity();
	gluOrtho2D(-0.2885 , 0.2885 , -0.2885 , 0.2885 );
	// these seem to be due to the camera distance and 60 degree FOV
	// found these numbers by trial and error!
	glDisable(GL_LIGHTING);
	glShadeModel(GL_SMOOTH);
	
	// from https://github.com/hn-88/GL_warp2Avi/blob/master/GL2AviView.cpp
   int i,j;
      
   /*
   using the code from 
   http://paulbourke.net/dataformats/meshwarp/ */
   
   int nx = meshcolumns;
   int ny = meshrows;
   // here, the mesh[i][j].i etc are translated into the
   // mesh[cols*r+c] variable etc in the 1d array
   //  
   //   mesh[i][j].i <===> mesh[nx*j+i].i
   // Thanks, Paul!
     
   glBegin(GL_QUADS);
   for (i=0;i<nx-1;i++) {
      for (j=0;j<ny-1;j++) {
         if (mesh[nx*j+i].i < 0 || mesh[(nx*(j+1))+i].i < 0 || mesh[(nx*(j+1))+(i+1)].i < 0 || mesh[nx*j+i+1].i < 0)
            continue;

         glColor3f(mesh[nx*j+i].i,mesh[nx*j+i].i,mesh[nx*j+i].i);
         glTexCoord2f(mesh[nx*j+i].u,mesh[nx*j+i].v);
         glVertex3f(mesh[nx*j+i].x,mesh[nx*j+i].y,0.0);

         glColor3f(mesh[nx*j+i+1].i,mesh[nx*j+i+1].i,mesh[nx*j+i+1].i);
         glTexCoord2f(mesh[nx*j+i+1].u,mesh[nx*j+i+1].v); 
         glVertex3f(mesh[nx*j+i+1].x,mesh[nx*j+i+1].y,0.0);

         glColor3f(mesh[nx*(j+1)+i+1].i,mesh[nx*(j+1)+i+1].i,mesh[nx*(j+1)+i+1].i);
         glTexCoord2f(mesh[nx*(j+1)+i+1].u,mesh[nx*(j+1)+i+1].v);
         glVertex3f(mesh[nx*(j+1)+i+1].x,mesh[nx*(j+1)+i+1].y,0.0);

         glColor3f(mesh[nx*(j+1)+i].i,mesh[nx*(j+1)+i].i,mesh[nx*(j+1)+i].i);
         glTexCoord2f(mesh[nx*(j+1)+i].u,mesh[nx*(j+1)+i].v);
         glVertex3f(mesh[nx*(j+1)+i].x,mesh[nx*(j+1)+i].y,0.0);

       }
   }
   //~ // testing code for checking if glortho has correct params
    //~ glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.77778f,  1.0f, -0.0f);
	//~ glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.77778f,  1.0f, -0.0f);
	//~ glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.77778f, -1.0f, -0.0f);
	//~ glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.77778f, -1.0f, -0.0f);
	
	//~ glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -0.0f);
	//~ glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -0.0f);
	//~ glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -0.0f);
	//~ glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -0.0f);
	
	//~ glTexCoord2f(1.0f, 1.0f); glVertex3f( 3.0f,  3.0f, -0.0f);
	//~ glTexCoord2f(0.0f, 1.0f); glVertex3f(-3.77778f,  3.0f, -0.0f);
	//~ glTexCoord2f(0.0f, 0.0f); glVertex3f(-3.77778f, -3.0f, -0.0f);
	//~ glTexCoord2f(1.0f, 0.0f); glVertex3f( 3.77778f, -3.0f, -0.0f);
	

   
   glEnd();
}
