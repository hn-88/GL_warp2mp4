// GL_warp2mp4.h
// moved declarations and globals here,
// from GL_warp2mp4.cpp

#define CV_PI   3.1415926535897932384626433832795

using namespace cv;

// some declarations and global variables
GLint g_hWindow;

GLvoid InitGL();
GLvoid OnDisplay();
GLvoid OnReshape(GLint w, GLint h);
GLvoid OnKeyPress (unsigned char key, GLint x, GLint y);
GLvoid OnIdle();

bool DrawGL();

// from fbo

int screenWidth, screenHeight;
bool mouseLeftDown;
bool mouseRightDown;
float mouseX, mouseY;
float cameraAngleX;
float cameraAngleY;
float cameraDistance;
const float CAMERA_DISTANCE = 6.0f;



void GenerateMovie();
void onExitCleanup();
void CreateGrid();
void CreateGridNoColor();
bool ReadMesh(std::string strpathtowarpfile);
void PreCreateGrid();
void getNextFrame();

VideoCapture inputVideo; 
VideoWriter outputVideo;
Mat src, srcres, dst, dstbgr, flipped;
int  fps, key;
int t_start, t_end;
unsigned long long framenum = 0;
int outputw = 1920;
int outputh = 1080;
int windoww = 800;
int windowh = 600;
int texturew = 2048;
int textureh = 2048;
char *pixels;
int returncode;

// PBO, FBO
GLuint pboIds[2];                   // IDs of PBO
GLuint fbotextureId, srctextureId;  // IDs of textures
GLubyte* imageData = 0;             // pointer to texture buffer
GLuint fboId;                       // ID of FBO

GLuint rboColorId, rboDepthId;      // IDs of Renderbuffer objects
bool pboSupported;
int pboMode = 2;
int drawMode = 0;
bool fboSupported;
bool fboUsed;

int    CHANNEL_COUNT   = 3;
int    DATA_SIZE;    //    = outputw * outputh * CHANNEL_COUNT;
int TEXTURE_WIDTH = 2048, TEXTURE_HEIGHT = 2048;
// Hack, make this a variable.


// from GL_warp2Avi
uint nFrames;

typedef struct {
   GLfloat x,y,u,v,i;
} meshpoint;

meshpoint *mesh;

// adding this for adapting vlc-warp code 
GLfloat *coords;
GLfloat *uv;
GLfloat *intensity;

int meshrows, meshcolumns;
 
///////////////////////
// the following is from
// Paul Bourke's lens.c and lens.h

typedef struct {
   double x,y,z;
} XYZ;
typedef struct {
   double r,g,b;
} COLOUR;
typedef struct {
   unsigned char r,g,b,a;
} PIXELA;

typedef struct {
   XYZ vp;              /* View position           */
   XYZ vd;              /* View direction vector   */
   XYZ vu;              /* View up direction       */
   XYZ pr;              /* Point to rotate about   */
   double focallength;  /* Focal Length along vd   */
   double aperture;     /* Camera aperture         */
   double eyesep;       /* Eye separation          */
	int screenheight,screenwidth;
} CAMERA;

GLUquadricObj *quadratic;	
