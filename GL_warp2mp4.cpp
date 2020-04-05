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
 * 
 * */
 
// for PBO,
#define GL_GLEXT_PROTOTYPES


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

// for PBO,
#include "glExtension.h"

#include "tinyfiledialogs.h"
#include "GL_warp2mp4.h"

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
		onExitCleanup();
		exit(0);
	}
}
		

void CreateGrid()
{
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
   // testing code for checking if glortho has correct params
    //~ glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.77778f,  1.0f, -0.0f);
	//~ glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.77778f,  1.0f, -0.0f);
	//~ glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.77778f, -1.0f, -0.0f);
	//~ glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.77778f, -1.0f, -0.0f);
	

   
   glEnd();
}

void CreateGridNoColor()
{
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

         //glColor3f(mesh[nx*j+i].i,mesh[nx*j+i].i,mesh[nx*j+i].i);
         glTexCoord2f(mesh[nx*j+i].u,mesh[nx*j+i].v);
         glVertex3f(mesh[nx*j+i].x,mesh[nx*j+i].y,0.0);

         //glColor3f(mesh[nx*j+i+1].i,mesh[nx*j+i+1].i,mesh[nx*j+i+1].i);
         glTexCoord2f(mesh[nx*j+i+1].u,mesh[nx*j+i+1].v); 
         glVertex3f(mesh[nx*j+i+1].x,mesh[nx*j+i+1].y,0.0);

         //glColor3f(mesh[nx*(j+1)+i+1].i,mesh[nx*(j+1)+i+1].i,mesh[nx*(j+1)+i+1].i);
         glTexCoord2f(mesh[nx*(j+1)+i+1].u,mesh[nx*(j+1)+i+1].v);
         glVertex3f(mesh[nx*(j+1)+i+1].x,mesh[nx*(j+1)+i+1].y,0.0);

         //glColor3f(mesh[nx*(j+1)+i].i,mesh[nx*(j+1)+i].i,mesh[nx*(j+1)+i].i);
         glTexCoord2f(mesh[nx*(j+1)+i].u,mesh[nx*(j+1)+i].v);
         glVertex3f(mesh[nx*(j+1)+i].x,mesh[nx*(j+1)+i].y,0.0);

       }
   }
   // testing code for checking if glortho has correct params
    //~ glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.77778f,  1.0f, -0.0f);
	//~ glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.77778f,  1.0f, -0.0f);
	//~ glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.77778f, -1.0f, -0.0f);
	//~ glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.77778f, -1.0f, -0.0f);
	

   
   glEnd();
}

void PreCreateGrid()
{
	//from https://github.com/hn-88/GL_warp2Avi/blob/master/GL2AviView.cpp
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer

	// from NeHe lesson 35
	
	//GrabAVIFrame(frame);										// Grab A Frame From The AVI
	
	//glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 2048, 2048, GL_BGR, GL_UNSIGNED_BYTE, src.data);
	// Hack - change 2048 to a variable

	//glLoadIdentity();										// Reset The Modelview Matrix

	 
	// code below is adapted from 
	// lens.c  
	
	
	GLfloat white[4] = {1.0,1.0,1.0,1.0};

	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); 
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

	glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	//glDrawBuffer(GL_BACK_LEFT);
	//let's draw directly to front buffer, since glutSwapBuffers crashes my system
	
	// we're reading from GL_BACK with readpixels, so
	//glDrawBuffer(GL_BACK);

	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	//glLoadIdentity();

	glOrtho(-1.77778 , 1.77778 , -1.0 , 1.0 , 1.0 , 10000.0); 
	// hard-coded for 16:9 output! Hack!

	glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();

	gluLookAt(0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0,0.0);
	glNormal3f(0.0,0.0,1.0);
	glColor3f(1.0,1.0,1.0);
	glDisable(GL_LIGHTING);
	glShadeModel(GL_SMOOTH); // changed to GL_FLAT to GL_SMOOTH to remove "tiling" effect
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT,white);
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); 
	//glEnable(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D,textureid);
	// CreateGrid()
}

bool DrawGL()
{
	//from https://github.com/hn-88/GL_warp2Avi/blob/master/GL2AviView.cpp
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer

	// from NeHe lesson 35
	
	//GrabAVIFrame(frame);										// Grab A Frame From The AVI
	inputVideo >> src; // gets the next frame into image
	if (src.empty()) // end of video;
	{
		onExitCleanup();
		exit(0);
	}
	glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 2048, 2048, GL_BGR, GL_UNSIGNED_BYTE, src.data);
	// Hack - change 2048 to a variable

	glLoadIdentity();										// Reset The Modelview Matrix

	 
	// code below is adapted from 
	// lens.c  
	int i,j; 
	//XYZ right,focus;
	unsigned int textureid;
	GLfloat white[4] = {1.0,1.0,1.0,1.0};

	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); 
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

	glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	glDrawBuffer(GL_BACK_LEFT);
	//let's draw directly to front buffer, since glutSwapBuffers crashes my system
	
	// we're reading from GL_BACK with readpixels, so
	//glDrawBuffer(GL_BACK);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(-1.77778 , 1.77778 , -1.0 , 1.0 , 1.0 , 10000.0); 
	// hard-coded for 16:9 output! Hack!

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	gluLookAt(0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0,0.0);
	glNormal3f(0.0,0.0,1.0);
	glColor3f(1.0,1.0,1.0);
	glDisable(GL_LIGHTING);
	glShadeModel(GL_SMOOTH); // changed to GL_FLAT to GL_SMOOTH to remove "tiling" effect
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT,white);
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); 
	//glEnable(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D,textureid);
	CreateGrid();
	//glDisable(GL_TEXTURE_2D);
	glutSwapBuffers(); 	
	// for some reason, drawing to back buffer and then calling
	// glutSwapBuffers crashes on my system
	 
	glFlush ();	
	// added this to see if it draws	
	glutPostRedisplay();											// Flush The GL Rendering Pipeline
	
	//frame++;
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

		

	return 1;		
}

void GenerateMovie()
{
	// from void CMainFrame::OnAvigenerationGenerate()
	// reading back buffer
	glReadBuffer(GL_BACK);
	for(int i=0;i<nFrames;i++)
	{
		// render frame
		DrawGL();
		// SwapBuffers();
		glutSwapBuffers();
		// Copy from OpenGL to buffer
		glReadPixels(0,0,outputw,outputh,GL_BGRA,GL_UNSIGNED_BYTE,dst.data);
		//glReadPixels(0,0,lpbih->biWidth,lpbih->biHeight,GL_BGR_EXT,GL_UNSIGNED_BYTE,bmBits);
		// GL_BGRA makes it much faster.
		// https://stackoverflow.com/questions/11409693/asynchronous-glreadpixels-with-pbo
		cvtColor(dst, dstbgr, CV_BGRA2BGR);
		flip(dst, flipped, 0);
		outputVideo << flipped; 
		
		//Bar.StepIt();
	}


}

void onExitCleanup()
{
	
    // deallocate texture buffer
    delete [] imageData;
    imageData = 0;

    // clean up texture
    glDeleteTextures(1, &textureId);

    // clean up PBOs
    if(pboSupported)
    {
        glDeleteBuffers(2, pboIds);
    }
    
    glutDestroyWindow(g_hWindow);
	std::cout << std::endl << "Finished writing" << std::endl;
}

GLvoid InitGL()
{  
	// old initgl code
	glClearColor (0.0, 0.0, 0.0, 0.0);

	glutDisplayFunc(OnDisplay);
	glutReshapeFunc(OnReshape);
	glutKeyboardFunc(OnKeyPress);
	glutIdleFunc(OnIdle);
	
	// initgl code from https://github.com/hn-88/GL_warp2Avi/blob/master/GL2AviView.cpp
	
	glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);				// Black Background
	glClearDepth(1.0f);									// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
	 

	//below this is from NeHe lesson 35
	 

	// Start Of User Initialization
	//angle		= 0.0f;											// Set Starting Angle To Zero
	//hdd = DrawDibOpen();										// Grab A Device Context For Our Dib
	glClearColor (0.0f, 0.0f, 0.0f, 0.5f);						// Black Background
	glClearDepth (1.0f);										// Depth Buffer Setup
	glDepthFunc (GL_LEQUAL);									// The Type Of Depth Testing (Less Or Equal)
	glEnable(GL_DEPTH_TEST);									// Enable Depth Testing
	  
	quadratic=gluNewQuadric();									// Create A Pointer To The Quadric Object
	gluQuadricNormals(quadratic, GLU_SMOOTH);					// Create Smooth Normals 
	gluQuadricTexture(quadratic, GL_TRUE);						// Create Texture Coords 

	glEnable(GL_TEXTURE_2D);									// Enable Texture Mapping
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);	// Set Texture Max Filter
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);	// Set Texture Min Filter

	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);		// Set The Texture Generation Mode For S To Sphere Mapping
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);		// Set The Texture Generation Mode For T To Sphere Mapping

	
	//GrabAVIFrame(frame); // adding this to initialize data
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, 2048, 2048, 0, GL_BGR, GL_UNSIGNED_BYTE, src.data);
	// changed source format to BGR and internal format to BGRA

	///////////////////////////
	// the below calls are from
	// lens.c 

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_POINT_SMOOTH);
	glDisable(GL_POLYGON_SMOOTH); 
	glDisable(GL_DITHER);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	glLineWidth(1.0);
	glPointSize(1.0);

	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	glFrontFace(GL_CW);
	glClearColor(0.0,0.0,0.0,0.0);
	glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);


}

GLvoid OnDisplay(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);

	// Set Projection Matrix
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();
	//gluOrtho2D(0, windoww, windowh, 0);
	gluOrtho2D(-outputw/outputh, outputw/outputh , -1.0 , 1.0);
	

	// Switch to Model View Matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Draw a textured quad
	CreateGridNoColor();
	
	
	// this has been moved to generatemovie function
	glReadPixels(0, 0, outputw, outputh, GL_BGRA, GL_UNSIGNED_BYTE, dst.data);
	//glReadPixels(0,0,lpbih->biWidth,lpbih->biHeight,GL_BGR_EXT,GL_UNSIGNED_BYTE,bmBits);
	// GL_BGRA makes it much faster.
	cvtColor(dst, dstbgr, CV_BGRA2BGR);
	flip(dstbgr, flipped, 0);
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
	  case 'x':
	  case 'X':
		  
		  onExitCleanup();
		  exit(0);
		  break;
	}
	glutPostRedisplay();
}


GLvoid OnIdle()
{
	// Capture next frame
	inputVideo >> src; // gets the next frame into image
	if (src.empty()) // end of video;
	{
		onExitCleanup();
		exit(0);
	}

	// Create Texture
	flip(src, flipped, 0);	// flip up down
	resize(flipped, srcres, Size(2048,2048), 0, 0, INTER_CUBIC);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, srcres.cols, srcres.rows, GL_BGR, GL_UNSIGNED_BYTE, srcres.data);
	
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
			infile >> windoww;
			infile >> tempstring;
			infile >> windowh;
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
    nFrames = inputVideo.get(CAP_PROP_FRAME_COUNT);
    std::cout << "Input frame resolution: Width=" << S.width << "  Height=" << S.height
         << " of nr#: " << nFrames << std::endl;
    std::cout << "Input codec type: " << EXT << std::endl;
        
    t_start = time(NULL);
	fps = 0;
	
	ReadMesh(strpathtowarpfile);
	
	dst.create(outputh, outputw, CV_8UC4);	// (rows, columns, type)
	// https://stackoverflow.com/questions/9097756/converting-data-from-glreadpixels-to-opencvmat/9098883
	//use fast 4-byte alignment (default anyway) if possible
	glPixelStorei(GL_PACK_ALIGNMENT, (dst.step & 3) ? 1 : 4);
	//set length of one complete row in destination data (doesn't need to equal img.cols)
	glPixelStorei(GL_PACK_ROW_LENGTH, dst.step/dst.elemSize());
	
	
	// Create GLUT Window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_ALPHA);
	glutInitWindowSize(windoww, windowh);
	g_hWindow = glutCreateWindow("Display");

    // Initialize OpenGL
	InitGL();
	
	// get OpenGL extensions
	
    glExtension& ext = glExtension::getInstance();
    pboSupported = ext.isSupported("GL_ARB_pixel_buffer_object");
    if(pboSupported)
    {
        std::cout << "Video card supports GL_ARB_pixel_buffer_object." << std::endl;
        pboMode = 2;
    }
    else
    {
        std::cout << "Video card does not support GL_ARB_pixel_buffer_object. Output may be slow." << std::endl;
        pboMode = 0;
    }
    
    DATA_SIZE        = S.width * S.height * CHANNEL_COUNT;
     
    
    // init texture objects
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    //~ glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, outputw, outputh, 0, GL_BGRA, GL_UNSIGNED_BYTE, (GLvoid*)imageData);
    // use gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, src.cols, src.rows, GL_BGR, GL_UNSIGNED_BYTE, src.data);
    // instead.
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    if(pboSupported)
    {
        // create 2 pixel buffer objects, you need to delete them when program exits.
        // glBufferData() with NULL pointer reserves only memory space.
        glGenBuffers(2, pboIds);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[0]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, DATA_SIZE, 0, GL_STREAM_DRAW);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[1]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, DATA_SIZE, 0, GL_STREAM_DRAW);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }
    
    
	//GenerateMovie();
	
	glutMainLoop();
    
    return 0;
	   
	   
} // end main

