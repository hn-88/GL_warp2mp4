// GL_warp2mp4.h
// moved declarations and globals here,
// from GL_warp2mp4.cpp

#define CV_PI   3.1415926535897932384626433832795

using namespace cv;


VideoCapture inputVideo; 
VideoWriter outputVideo;
Mat src, srcres, dst, dstbgr, flipped;
int  fps, key;
int t_start, t_end;
unsigned long long framenum = 0;
int outputw = 2056;
int outputh = 2056;
int windoww = 800;
int windowh = 600;
int texturew = 2056;
int textureh = 2056;
char *pixels;
int returncode;

void getNextFrame();
void CreateGrid();
void CreateGridNoColor();
bool ReadMesh(std::string strpathtowarpfile);

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
