/*
  CSCI 420
  Assignment 2: Roller Coasters
  Sarah Kurdoghlian
 */

#include <stdio.h>
#include <stdlib.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include <math.h>
#include <pic.h>

//raunaqra@usc.edu

/* represents one control point along the spline */
struct point {
   double x;
   double y;
   double z;
};

/* spline struct which contains how many control points, and an array of control points */
struct spline {
   int numControlPoints;
   struct point *points;
};

/* the spline array */
struct spline *g_Splines;

/* total number of splines */
int g_iNumOfSplines;

float tension_factor = 0.5;
float basis_catmull[4][4] = { 
    {-tension_factor, 2 - tension_factor, tension_factor - 2, tension_factor},
    {2 * tension_factor, tension_factor - 3, 3 - (2 * tension_factor), -tension_factor},
    {-tension_factor, 0, tension_factor, 0},
    {0, 1, 0, 0}
  };

float camera_up_vector[3] = {0.0, 1.0, 0.0};

int g_iMenuId;
int menuSelection;

int g_vMousePos[2] = {0, 0};
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iRightMouseButton = 0;
int g_iMiddleMouseButton = 0;

/* state of the world */
float g_vLandRotate[3] = {0.0, 0.0, 0.0};
float g_vLandTranslate[3] = {0.0, 0.0, 0.0};
float g_vLandScale[3] = {1.0, 1.0, 1.0};

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROLSTATE;

typedef enum {LEFT, BACK, RIGHT, FRONT, TOP, FLOOR} FACES;

/* default states of transformation */
CONTROLSTATE g_ControlState = ROTATE;

/* texture of floor */
Pic * floorImage;
unsigned char floor_texture[256][256][3];
GLuint texName[2];

/* texture of sky */
Pic * skyImage;
unsigned char sky_texture[256][256][3];
GLuint texNameSky;

/* globals for keeping track of the u parameter, previous point, and previous binormal */
/* Used for moving the camera and drawing the spline */
float global_u = 0.0;
int control_point_index = 0;
bool isFirstPoint = true;
bool shouldCalcInitialNormal = true;
int crossBarCounter = 4;

/* globals to keep track of local coordinate system */
struct point current_tangent;
struct point current_norm;
struct point current_binorm;

struct point current_point;
struct point prev_point;

struct point previous_tangent;
struct point previous_norm;
struct point previous_binorm;

/* vertices of cube about the origin */
GLfloat vertices[8][3] =
    {{-2.0, -2.0, -2.0}, {2.0, -2.0, -2.0},
    {2.0, 2.0, -2.0}, {-2.0, 2.0, -2.0}, {-2.0, -2.0, 2.0},
    {2.0, -2.0, 2.0}, {2.0, 2.0, 2.0}, {-2.0, 2.0, 2.0}};

int loadSplines(char *argv) {
  char *cName = (char *)malloc(128 * sizeof(char));
  FILE *fileList;
  FILE *fileSpline;
  int iType, i = 0, j, iLength;

  /* load the track file */
  fileList = fopen(argv, "r");
  if (fileList == NULL) {
    printf ("can't open file\n");
    exit(1);
  }
  
  /* stores the number of splines in a global variable */
  fscanf(fileList, "%d", &g_iNumOfSplines);

  g_Splines = (struct spline *)malloc(g_iNumOfSplines * sizeof(struct spline));

  /* reads through the spline files */
  for (j = 0; j < g_iNumOfSplines; j++) {
    i = 0;
    fscanf(fileList, "%s", cName);
    fileSpline = fopen(cName, "r");

    if (fileSpline == NULL) {
      printf ("can't open file\n");
      exit(1);
    }

    /* gets length for spline file */
    fscanf(fileSpline, "%d %d", &iLength, &iType);

    /* allocate memory for all the points */
    g_Splines[j].points = (struct point *)malloc(iLength * sizeof(struct point));
    g_Splines[j].numControlPoints = iLength;

    /* saves the data to the struct */
    while (fscanf(fileSpline, "%lf %lf %lf", 
	   &g_Splines[j].points[i].x, 
	   &g_Splines[j].points[i].y, 
	   &g_Splines[j].points[i].z) != EOF) {
      i++;
    }
  }

  free(cName);

  return 0;
}

void myInit() 
{
  /* setup gl view here */
  glClearColor(0.0, 0.0, 0.0, 0.0);
  // enable depth buffering
  glEnable(GL_DEPTH_TEST); 
  // interpolate colors during rasterization
  glShadeModel(GL_SMOOTH); 
}

void initTextures() {
  floorImage = jpeg_read("floor3.jpg", NULL);
  if (!floorImage)
  {
    printf ("error reading %s.\n", "floor.jpg");
    exit(1);
  }

  // Load pixels into array
  int height = floorImage->ny;
  int width = floorImage->nx;
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      unsigned char pixel[3] = {
                      PIC_PIXEL(floorImage, j, i, 0), 
                      PIC_PIXEL(floorImage, j, i, 1),
                      PIC_PIXEL(floorImage, j, i, 2)
                    };
      floor_texture[i][j][0] = pixel[0];
      floor_texture[i][j][1] = pixel[1];
      floor_texture[i][j][2] = pixel[2];
    }
  }

  // create placeholder for texture
  glGenTextures(2, texName);

  // make texture, "texName", the currently active texture on a CUBE_MAP
  glBindTexture(GL_TEXTURE_2D, texName[0]);

  // repeat texture pattern in s and t
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // use linear filter for both magnification and minification
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  // load image data stored at pointer (floor_texture) into currently active texture (texName)
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, floor_texture);

  // ------ SKY TEXTURE (TEXTURE NUMBER 2) -------
  skyImage = jpeg_read("galaxy.jpg", NULL);
  if (!floorImage)
  {
    printf ("error reading %s.\n", "galaxy.jpg");
    exit(1);
  }

  // Load pixels into array
  height = skyImage->ny;
  width = skyImage->nx;
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      unsigned char pixel[3] = {
                      PIC_PIXEL(skyImage, j, i, 0), 
                      PIC_PIXEL(skyImage, j, i, 1),
                      PIC_PIXEL(skyImage, j, i, 2)
                    };
      sky_texture[i][j][0] = pixel[0];
      sky_texture[i][j][1] = pixel[1];
      sky_texture[i][j][2] = pixel[2];
    }
  }

  // make texture, "texName", the currently active texture on a CUBE_MAP
  glBindTexture(GL_TEXTURE_2D, texName[1]);

  // repeat texture pattern in s and t
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // use linear filter for both magnification and minification
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  // load image data stored at pointer (floor_texture) into currently active texture (texName)
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, sky_texture);
}

/* Calculate and return a point on the spline using the Catmull-Rom spline formula */
struct point catmullRomCalc(float u, struct point p1, struct point p2, struct point p3, struct point p4) 
{
  float u_cubed = u * u * u;
  float u_squared = u * u;
  struct point control_matrix[4] = {p1, p2, p3, p4};
  float u_matrix[4] = {u_cubed, u_squared, u, 1};

  struct point new_p;

  // Calculate values for Catmull Matrix X Control Matrix
  float c11 = (basis_catmull[0][0] * control_matrix[0].x) + (basis_catmull[0][1] * control_matrix[1].x) + (basis_catmull[0][2] * control_matrix[2].x) + (basis_catmull[0][3] * control_matrix[3].x);
  float c12 = (basis_catmull[0][0] * control_matrix[0].y) + (basis_catmull[0][1] * control_matrix[1].y) + (basis_catmull[0][2] * control_matrix[2].y) + (basis_catmull[0][3] * control_matrix[3].y);
  float c13 = (basis_catmull[0][0] * control_matrix[0].z) + (basis_catmull[0][1] * control_matrix[1].z) + (basis_catmull[0][2] * control_matrix[2].z) + (basis_catmull[0][3] * control_matrix[3].z);

  float c21 = (basis_catmull[1][0] * control_matrix[0].x) + (basis_catmull[1][1] * control_matrix[1].x) + (basis_catmull[1][2] * control_matrix[2].x) + (basis_catmull[1][3] * control_matrix[3].x);
  float c22 = (basis_catmull[1][0] * control_matrix[0].y) + (basis_catmull[1][1] * control_matrix[1].y) + (basis_catmull[1][2] * control_matrix[2].y) + (basis_catmull[1][3] * control_matrix[3].y);
  float c23 = (basis_catmull[1][0] * control_matrix[0].z) + (basis_catmull[1][1] * control_matrix[1].z) + (basis_catmull[1][2] * control_matrix[2].z) + (basis_catmull[1][3] * control_matrix[3].z);

  float c31 = (basis_catmull[2][0] * control_matrix[0].x) + (basis_catmull[2][1] * control_matrix[1].x) + (basis_catmull[2][2] * control_matrix[2].x) + (basis_catmull[2][3] * control_matrix[3].x);
  float c32 = (basis_catmull[2][0] * control_matrix[0].y) + (basis_catmull[2][1] * control_matrix[1].y) + (basis_catmull[2][2] * control_matrix[2].y) + (basis_catmull[2][3] * control_matrix[3].y);
  float c33 = (basis_catmull[2][0] * control_matrix[0].z) + (basis_catmull[2][1] * control_matrix[1].z) + (basis_catmull[2][2] * control_matrix[2].z) + (basis_catmull[2][3] * control_matrix[3].z);

  float c41 = (basis_catmull[3][0] * control_matrix[0].x) + (basis_catmull[3][1] * control_matrix[1].x) + (basis_catmull[3][2] * control_matrix[2].x) + (basis_catmull[3][3] * control_matrix[3].x);
  float c42 = (basis_catmull[3][0] * control_matrix[0].y) + (basis_catmull[3][1] * control_matrix[1].y) + (basis_catmull[3][2] * control_matrix[2].y) + (basis_catmull[3][3] * control_matrix[3].y);
  float c43 = (basis_catmull[3][0] * control_matrix[0].z) + (basis_catmull[3][1] * control_matrix[1].z) + (basis_catmull[3][2] * control_matrix[2].z) + (basis_catmull[3][3] * control_matrix[3].z);

  // Calculate point
  float intermediate_matrix[4][3] = {
    {c11, c12, c13},
    {c21, c22, c23},
    {c31, c32, c33},
    {c41, c42, c43}
  };

  new_p.x = (u_matrix[0] * intermediate_matrix[0][0]) + (u_matrix[1] * intermediate_matrix[1][0]) + (u_matrix[2] * intermediate_matrix[2][0]) + (u_matrix[3] * intermediate_matrix[3][0]);
  new_p.y = (u_matrix[0] * intermediate_matrix[0][1]) + (u_matrix[1] * intermediate_matrix[1][1]) + (u_matrix[2] * intermediate_matrix[2][1]) + (u_matrix[3] * intermediate_matrix[3][1]);
  new_p.z = (u_matrix[0] * intermediate_matrix[0][2]) + (u_matrix[1] * intermediate_matrix[1][2]) + (u_matrix[2] * intermediate_matrix[2][2]) + (u_matrix[3] * intermediate_matrix[3][2]);

  return new_p;
}

double maxValue(double x, double y, double z) {
  if (x < 0) { x = -1 * x; }
  if (y < 0) { y = -1 * y; }
  if (z < 0) { z = -1 * z; }
  if (x > y) {
    return (x > z) ? x : z;
  } else {
    return (y > z) ? y : z;
  }
}

double magnitude(double x, double y, double z)
{
  return sqrt((x * x) + (y * y) + (z * z));
}

struct point computeTangentVector(float u, struct point p1, struct point p2, struct point p3, struct point p4)
{
  float three_u_squared = 3 * (u * u);
  float two_u = 2 * u;
  struct point control_matrix[4] = {p1, p2, p3, p4};
  float u_matrix[4] = {three_u_squared, two_u, 1, 0};

  struct point new_p;

  // Calculate values for Catmull Matrix X Control Matrix
  float c11 = (basis_catmull[0][0] * control_matrix[0].x) + (basis_catmull[0][1] * control_matrix[1].x) + (basis_catmull[0][2] * control_matrix[2].x) + (basis_catmull[0][3] * control_matrix[3].x);
  float c12 = (basis_catmull[0][0] * control_matrix[0].y) + (basis_catmull[0][1] * control_matrix[1].y) + (basis_catmull[0][2] * control_matrix[2].y) + (basis_catmull[0][3] * control_matrix[3].y);
  float c13 = (basis_catmull[0][0] * control_matrix[0].z) + (basis_catmull[0][1] * control_matrix[1].z) + (basis_catmull[0][2] * control_matrix[2].z) + (basis_catmull[0][3] * control_matrix[3].z);

  float c21 = (basis_catmull[1][0] * control_matrix[0].x) + (basis_catmull[1][1] * control_matrix[1].x) + (basis_catmull[1][2] * control_matrix[2].x) + (basis_catmull[1][3] * control_matrix[3].x);
  float c22 = (basis_catmull[1][0] * control_matrix[0].y) + (basis_catmull[1][1] * control_matrix[1].y) + (basis_catmull[1][2] * control_matrix[2].y) + (basis_catmull[1][3] * control_matrix[3].y);
  float c23 = (basis_catmull[1][0] * control_matrix[0].z) + (basis_catmull[1][1] * control_matrix[1].z) + (basis_catmull[1][2] * control_matrix[2].z) + (basis_catmull[1][3] * control_matrix[3].z);

  float c31 = (basis_catmull[2][0] * control_matrix[0].x) + (basis_catmull[2][1] * control_matrix[1].x) + (basis_catmull[2][2] * control_matrix[2].x) + (basis_catmull[2][3] * control_matrix[3].x);
  float c32 = (basis_catmull[2][0] * control_matrix[0].y) + (basis_catmull[2][1] * control_matrix[1].y) + (basis_catmull[2][2] * control_matrix[2].y) + (basis_catmull[2][3] * control_matrix[3].y);
  float c33 = (basis_catmull[2][0] * control_matrix[0].z) + (basis_catmull[2][1] * control_matrix[1].z) + (basis_catmull[2][2] * control_matrix[2].z) + (basis_catmull[2][3] * control_matrix[3].z);

  float c41 = (basis_catmull[3][0] * control_matrix[0].x) + (basis_catmull[3][1] * control_matrix[1].x) + (basis_catmull[3][2] * control_matrix[2].x) + (basis_catmull[3][3] * control_matrix[3].x);
  float c42 = (basis_catmull[3][0] * control_matrix[0].y) + (basis_catmull[3][1] * control_matrix[1].y) + (basis_catmull[3][2] * control_matrix[2].y) + (basis_catmull[3][3] * control_matrix[3].y);
  float c43 = (basis_catmull[3][0] * control_matrix[0].z) + (basis_catmull[3][1] * control_matrix[1].z) + (basis_catmull[3][2] * control_matrix[2].z) + (basis_catmull[3][3] * control_matrix[3].z);

  // Calculate point
  float intermediate_matrix[4][3] = {
    {c11, c12, c13},
    {c21, c22, c23},
    {c31, c32, c33},
    {c41, c42, c43}
  };

  new_p.x = (u_matrix[0] * intermediate_matrix[0][0]) + (u_matrix[1] * intermediate_matrix[1][0]) + (u_matrix[2] * intermediate_matrix[2][0]) + (u_matrix[3] * intermediate_matrix[3][0]);
  new_p.y = (u_matrix[0] * intermediate_matrix[0][1]) + (u_matrix[1] * intermediate_matrix[1][1]) + (u_matrix[2] * intermediate_matrix[2][1]) + (u_matrix[3] * intermediate_matrix[3][1]);
  new_p.z = (u_matrix[0] * intermediate_matrix[0][2]) + (u_matrix[1] * intermediate_matrix[1][2]) + (u_matrix[2] * intermediate_matrix[2][2]) + (u_matrix[3] * intermediate_matrix[3][2]);

  double vec_mag = magnitude(new_p.x, new_p.y, new_p.z);

  struct point normalized_tangent;
  normalized_tangent.x = new_p.x/vec_mag;
  normalized_tangent.y = new_p.y/vec_mag;
  normalized_tangent.z = new_p.z/vec_mag;

  return normalized_tangent;
}

/* assume binormal x and y = 1 and solve for binormal.z so that the dot product is 0 */
float solveForFirstBinormal(struct point tangent) {
  return (-(tangent.y + tangent.z))/tangent.x;
}

/* Need to Compute T, N, and B */
void moveCamera(float u, struct point p1, struct point p2, struct point p3, struct point p4) {
  // compute tangent vector
  current_tangent = computeTangentVector(u, p1, p2, p3, p4);
  //printf("Tangent: [%f %f %f]\n", current_tangent.x, current_tangent.y, current_tangent.z);

  // if we are at the first point then pick arbitrary Vector
  if (shouldCalcInitialNormal == true) { 
    shouldCalcInitialNormal = false;
    current_binorm.x = 0.0;
    current_binorm.y = 1.0;
    current_binorm.z = 0.0;
  }

  // Normal = Binorm_previous x T
  current_norm.x = (current_binorm.y * current_tangent.z) - (current_binorm.z * current_tangent.y);
  current_norm.y = (current_binorm.z * current_tangent.x) - (current_binorm.x * current_tangent.z);
  current_norm.z = (current_binorm.x * current_tangent.y) - (current_binorm.y * current_tangent.x);

  // normalize
  double norm_mag = magnitude(current_norm.x, current_norm.y, current_norm.z); //normalize
  current_norm.x = current_norm.x/norm_mag;
  current_norm.y = current_norm.y/norm_mag;
  current_norm.z = current_norm.z/norm_mag;
  //printf("Normal: [%f %f %f]\n", current_norm.x, current_norm.y, current_norm.z);

  // Binormal = T x N
  current_binorm.x = (current_tangent.y * current_norm.z) - (current_tangent.z * current_norm.y);
  current_binorm.y = (current_tangent.z * current_norm.x) - (current_tangent.x * current_norm.z);
  current_binorm.z = (current_tangent.x * current_norm.y) - (current_tangent.y * current_norm.x);

  // normalize
  double binormal_mag = magnitude(current_binorm.x, current_binorm.y, current_binorm.z); //normalize
  current_binorm.x = current_binorm.x/binormal_mag;
  current_binorm.y = current_binorm.y/binormal_mag;
  current_binorm.z = current_binorm.z/binormal_mag;
  //printf("binorm: [%f %f %f]\n", current_binorm.x, current_binorm.y, current_binorm.z);

  // update the camera (viewing transformation) 
  // parameters: eyex, eyey, eyez, focusx, focusy, focusz, upx, upy, upz 
  printf("current point: %f %f %f\n", current_point.x, current_point.y, current_point.z);
  gluLookAt(current_point.x + (0.1*current_norm.x) - (0.1 * current_binorm.x), current_point.y + (0.1*current_norm.y) - (0.1 * current_binorm.y), current_point.z + (current_norm.z * 0.1)  - (0.1 * current_binorm.z), 
            current_point.x + current_tangent.x, current_point.y + current_tangent.y, current_point.z + current_tangent.z, 
            current_norm.x, current_norm.y, current_norm.z);
//  gluLookAt(current_point.x + (0.1*current_norm.x), current_point.y + (0.1*current_norm.y), current_point.z + (0.1*current_norm.z), 
//             0, 0, 1.0, 
//             0, 1.0, 0);
}

void calcLocalCoordinates(float u, struct point p1, struct point p2, struct point p3, struct point p4) {
  // compute tangent vector
  previous_tangent = computeTangentVector(u, p1, p2, p3, p4);
  //printf("Tangent: [%f %f %f]\n", current_tangent.x, current_tangent.y, current_tangent.z);

  // if we are at the first point then pick arbitrary Vector
  if (shouldCalcInitialNormal == true) { 
    //shouldCalcInitialNormal = false;
    previous_binorm.x = 0.0;
    previous_binorm.y = 1.0;
    previous_binorm.z = 0.0;
  }

  // Normal = Binorm_previous x T
  previous_norm.x = (previous_binorm.y * previous_tangent.z) - (previous_binorm.z * previous_tangent.y);
  previous_norm.y = (previous_binorm.z * previous_tangent.x) - (previous_binorm.x * previous_tangent.z);
  previous_norm.z = (previous_binorm.x * previous_tangent.y) - (previous_binorm.y * previous_tangent.x);

  // normalize
  double norm_mag = magnitude(previous_norm.x, previous_norm.y, previous_norm.z); //normalize
  previous_norm.x = previous_norm.x/norm_mag;
  previous_norm.y = previous_norm.y/norm_mag;
  previous_norm.z = previous_norm.z/norm_mag;
  //printf("Normal: [%f %f %f]\n", current_norm.x, current_norm.y, current_norm.z);

  // Binormal = T x N
  previous_binorm.x = (previous_tangent.y * previous_norm.z) - (previous_tangent.z * previous_norm.y);
  previous_binorm.y = (previous_tangent.z * previous_norm.x) - (previous_tangent.x * previous_norm.z);
  previous_binorm.z = (previous_tangent.x * previous_norm.y) - (previous_tangent.y * previous_norm.x);

  // normalize
  double binormal_mag = magnitude(previous_binorm.x, previous_binorm.y, previous_binorm.z); //normalize
  previous_binorm.x = previous_binorm.x/binormal_mag;
  previous_binorm.y = previous_binorm.y/binormal_mag;
  previous_binorm.z = previous_binorm.z/binormal_mag;
}

void drawQuadPoints(float u, struct point p1, struct point p2, struct point p3 , struct point p4, struct point the_point, struct point previous_point)
{
  current_tangent = computeTangentVector(u, p1, p2, p3, p4);
  //printf("Tangent: [%f %f %f]\n", current_tangent.x, current_tangent.y, current_tangent.z);

  // if we are at the first point then pick arbitrary Vector
  if (shouldCalcInitialNormal == true) { 
    shouldCalcInitialNormal = false;
    current_binorm.x = 0.0;
    current_binorm.y = 1.0;
    current_binorm.z = 0.0;
  }

  // Normal = Binorm_previous x T
  current_norm.x = (current_binorm.y * current_tangent.z) - (current_binorm.z * current_tangent.y);
  current_norm.y = (current_binorm.z * current_tangent.x) - (current_binorm.x * current_tangent.z);
  current_norm.z = (current_binorm.x * current_tangent.y) - (current_binorm.y * current_tangent.x);

  // normalize
  double norm_mag = magnitude(current_norm.x, current_norm.y, current_norm.z); //normalize
  current_norm.x = current_norm.x/norm_mag;
  current_norm.y = current_norm.y/norm_mag;
  current_norm.z = current_norm.z/norm_mag;
  //printf("Normal: [%f %f %f]\n", current_norm.x, current_norm.y, current_norm.z);

  // Binormal = T x N
  current_binorm.x = (current_tangent.y * current_norm.z) - (current_tangent.z * current_norm.y);
  current_binorm.y = (current_tangent.z * current_norm.x) - (current_tangent.x * current_norm.z);
  current_binorm.z = (current_tangent.x * current_norm.y) - (current_tangent.y * current_norm.x);

  // normalize
  double binormal_mag = magnitude(current_binorm.x, current_binorm.y, current_binorm.z); //normalize
  current_binorm.x = current_binorm.x/binormal_mag;
  current_binorm.y = current_binorm.y/binormal_mag;
  current_binorm.z = current_binorm.z/binormal_mag;
  //printf("binorm: [%f %f %f]\n", current_binorm.x, current_binorm.y, current_binorm.z);

  float bottomRight_previous[3] = {previous_point.x + 0.005*(-previous_norm.x + previous_binorm.x), previous_point.y + 0.005*(-previous_norm.y + previous_binorm.y), previous_point.z + 0.005*(-previous_norm.z + previous_binorm.z)};
  float topRight_previous[3] = {previous_point.x + 0.005*(previous_norm.x + previous_binorm.x), previous_point.y + 0.005*(previous_norm.y + previous_binorm.y), previous_point.z + 0.005*(previous_norm.z + previous_binorm.z)};
  float topLeft_previous[3] = {previous_point.x + 0.005*(previous_norm.x - previous_binorm.x), previous_point.y + 0.005*(previous_norm.y - previous_binorm.y), previous_point.z + 0.005*(previous_norm.z - previous_binorm.z)};
  float bottomLeft_previous[3] = {previous_point.x + 0.005*(-previous_norm.x - previous_binorm.x), previous_point.y + 0.005*(-previous_norm.y - previous_binorm.y), previous_point.z + 0.005*(-previous_norm.z - previous_binorm.z)};

  float bottomRight_current[3] = {the_point.x + 0.005*(-current_norm.x + current_binorm.x), the_point.y + 0.005*(-current_norm.y + current_binorm.y), the_point.z + 0.005*(-current_norm.z + current_binorm.z)};
  float topRight_current[3] = {the_point.x + 0.005*(current_norm.x + current_binorm.x), the_point.y + 0.005*(current_norm.y + current_binorm.y), the_point.z + 0.005*(current_norm.z + current_binorm.z)};
  float topLeft_current[3] = {the_point.x + 0.005*(current_norm.x - current_binorm.x), the_point.y + 0.005*(current_norm.y - current_binorm.y), the_point.z + 0.005*(current_norm.z - current_binorm.z)};
  float bottomLeft_current[3] = {the_point.x + 0.005*(-current_norm.x - current_binorm.x), the_point.y + 0.005*(-current_norm.y - current_binorm.y), the_point.z + 0.005*(-current_norm.z - current_binorm.z)};
  glBegin(GL_QUADS);

  // left
  glVertex3f(topLeft_previous[0], topLeft_previous[1], topLeft_previous[2]); // v0
  glVertex3f(bottomLeft_previous[0], bottomLeft_previous[1], bottomLeft_previous[2]); //v1
  glVertex3f(bottomLeft_current[0], bottomLeft_current[1], bottomLeft_current[2]); //v2
  glVertex3f(topLeft_current[0], topLeft_current[1], topLeft_current[2]); //v3

  // right
  glVertex3f(topRight_previous[0], topRight_previous[1], topRight_previous[2]); // v0
  glVertex3f(bottomRight_previous[0], bottomRight_previous[1], bottomRight_previous[2]); //v1
  glVertex3f(topRight_current[0], topRight_current[1], topRight_current[2]); //v2
  glVertex3f(bottomRight_current[0], bottomRight_current[1], bottomRight_current[2]); //v3

  // top
  glVertex3f(topRight_previous[0], topRight_previous[1], topRight_previous[2]); // v0
  glVertex3f(topLeft_previous[0], topLeft_previous[1], topLeft_previous[2]); //v1
  glVertex3f(topLeft_current[0], topLeft_current[1], topLeft_current[2]); //v2
  glVertex3f(topRight_current[0], topRight_current[1], topRight_current[2]); //v3

  // bottom
  glVertex3f(bottomRight_previous[0], bottomRight_previous[1], bottomRight_previous[2]); // v0
  glVertex3f(bottomLeft_previous[0], bottomLeft_previous[1], bottomLeft_previous[2]); //v1
  glVertex3f(bottomRight_current[0], bottomRight_current[1], bottomRight_current[2]); //v2
  glVertex3f(bottomLeft_current[0], bottomLeft_current[1], bottomLeft_current[2]); //v3

  glEnd();

  // PARALLEL LINE COORDINATES
  float parallel_bottomRight_current[3] = {(the_point.x - (0.1 * current_binorm.x)) + 0.005*(-current_norm.x + current_binorm.x), (the_point.y - (0.1 * current_binorm.y)) + 0.005*(-current_norm.y + current_binorm.y), (the_point.z - (0.1 * current_binorm.z)) + 0.005*(-current_norm.z + current_binorm.z)};
  float parallel_topRight_current[3] = {(the_point.x -  (0.1 * current_binorm.x)) + 0.005*(current_norm.x + current_binorm.x), (the_point.y - (0.1 * current_binorm.y)) + 0.005*(current_norm.y + current_binorm.y), (the_point.z - (0.1 * current_binorm.z)) + 0.005*(current_norm.z + current_binorm.z)};
  float parallel_topLeft_current[3] = {(the_point.x - (0.1 * current_binorm.x)) + 0.005*(current_norm.x - current_binorm.x), (the_point.y - (0.1 * current_binorm.y)) + 0.005*(current_norm.y - current_binorm.y), (the_point.z - (0.1 * current_binorm.z)) + 0.005*(current_norm.z - current_binorm.z)};
  float parallel_bottomLeft_current[3] = {(the_point.x - (0.1 * current_binorm.x)) + 0.005*(-current_norm.x - current_binorm.x), (the_point.y - (0.1 * current_binorm.y)) + 0.005*(-current_norm.y - current_binorm.y), (the_point.z - (0.1 * current_binorm.z)) + 0.005*(-current_norm.z - current_binorm.z)};

  float parallel_bottomRight_previous[3] = {(previous_point.x - (0.1 * previous_binorm.x)) + 0.005*(-previous_norm.x + previous_binorm.x), (previous_point.y - (0.1 * previous_binorm.y)) + 0.005*(-previous_norm.y + previous_binorm.y), (previous_point.z - (0.1 * previous_binorm.z)) + 0.005*(-previous_norm.z + previous_binorm.z)};
  float parallel_topRight_previous[3] = {(previous_point.x -  (0.1 * previous_binorm.x)) + 0.005*(previous_norm.x + previous_binorm.x), (previous_point.y - (0.1 * previous_binorm.y)) + 0.005*(previous_norm.y + previous_binorm.y), (previous_point.z - (0.1 * previous_binorm.z)) + 0.005*(previous_norm.z + previous_binorm.z)};
  float parallel_topLeft_previous[3] = {(previous_point.x - (0.1 * previous_binorm.x)) + 0.005*(previous_norm.x - previous_binorm.x), (previous_point.y - (0.1 * previous_binorm.y)) + 0.005*(previous_norm.y - previous_binorm.y), (previous_point.z - (0.1 * previous_binorm.z)) + 0.005*(previous_norm.z - previous_binorm.z)};
  float parallel_bottomLeft_previous[3] = {(previous_point.x - (0.1 * previous_binorm.x)) + 0.005*(-previous_norm.x - previous_binorm.x), (previous_point.y - (0.1 * previous_binorm.y)) + 0.005*(-previous_norm.y - previous_binorm.y), (previous_point.z - (0.1 * previous_binorm.z)) + 0.005*(-previous_norm.z - previous_binorm.z)};

  // Draw Parallel spline
  glBegin(GL_QUADS);

  // left
  glVertex3f(parallel_topLeft_previous[0], parallel_topLeft_previous[1], parallel_topLeft_previous[2]); // v0
  glVertex3f(parallel_bottomLeft_previous[0], parallel_bottomLeft_previous[1], parallel_bottomLeft_previous[2]); //v1
  glVertex3f(parallel_topLeft_current[0], parallel_topLeft_current[1], parallel_topLeft_current[2]); //v2
  glVertex3f(parallel_bottomLeft_current[0], parallel_bottomLeft_current[1], parallel_bottomLeft_current[2]); //v3

  // right
  glVertex3f(parallel_topRight_previous[0], parallel_topRight_previous[1], parallel_topRight_previous[2]); // v0
  glVertex3f(parallel_bottomRight_previous[0], parallel_bottomRight_previous[1], parallel_bottomRight_previous[2]); //v1
  glVertex3f(parallel_topRight_current[0], parallel_topRight_current[1], parallel_topRight_current[2]); //v2
  glVertex3f(parallel_bottomRight_current[0], parallel_bottomRight_current[1], parallel_bottomRight_current[2]); //v3

  // top
  glVertex3f(parallel_topLeft_previous[0], parallel_topLeft_previous[1], parallel_topLeft_previous[2]); // v0
  glVertex3f(parallel_topRight_previous[0], parallel_topRight_previous[1], parallel_topRight_previous[2]); //v1
  glVertex3f(parallel_topLeft_current[0], parallel_topLeft_current[1], parallel_topLeft_current[2]); //v2
  glVertex3f(parallel_topRight_current[0], parallel_topRight_current[1], parallel_topRight_current[2]); //v3

  // bottom
  glVertex3f(parallel_bottomRight_previous[0], parallel_bottomRight_previous[1], parallel_bottomRight_previous[2]); // v0
  glVertex3f(parallel_bottomLeft_previous[0], parallel_bottomLeft_previous[1], parallel_bottomLeft_previous[2]); //v1
  glVertex3f(parallel_bottomRight_current[0], parallel_bottomRight_current[1], parallel_bottomRight_current[2]); //v2
  glVertex3f(parallel_bottomLeft_current[0], parallel_bottomLeft_current[1], parallel_bottomLeft_current[2]); //v3

  glEnd();

  crossBarCounter += 1;
  // cross bar
  if (crossBarCounter == 5) {
    // glBegin();

    // glEnd();
    crossBarCounter = 0;
  }
}

/* Get each point on the spline by varying the parameter, "u", by 0.001 */
/* Draw a new vertex for each point generated by Catmull-Rom calculation */
/* Then, update the camera position */
void constructSpline(struct point p1, struct point p2, struct point p3, struct point p4) 
{
  float prev_u = 0.0;
  glColor3f(1.0, 1.0, 0.0);
  glLineWidth((GLfloat)5.0);

  // calc initial point
  prev_point = catmullRomCalc(0.0, p1, p2, p3, p4);
  double max = maxValue(prev_point.x, prev_point.y, prev_point.z);
  if (max > 1) {
    prev_point.x = prev_point.x/max;
    prev_point.y = prev_point.y/max;
    prev_point.z = prev_point.z/max;
  } 

  for (float u = 0.01; u < 1.0; u += 0.01) {
    // insert each value of "u" into the Catmull-Rom equation and obtain the point at p(u)
    struct point new_p;
    new_p = catmullRomCalc(u, p1, p2, p3, p4);
    double max = maxValue(new_p.x, new_p.y, new_p.z);
    if (max > 1) {
      new_p.x = new_p.x/max;
      new_p.y = new_p.y/max;
      new_p.z = new_p.z/max;
    } 
    //printf("new_p.x: %lf new_p.y: %lf new_p.z: %lf\n", new_p.x, new_p.y, new_p.z);

    drawQuadPoints(prev_u, p1, p2, p3, p4, new_p, prev_point);
    //drawQuadPoints(u, p1, p2, p3, p4, new_p);

    // keep track of previous point in order to draw smooth lines
    // glBegin(GL_LINES);
    //   glVertex3d(prev_point.x, prev_point.y, prev_point.z);
    //   glVertex3d(new_p.x, new_p.y, new_p.z);
    // glEnd();

    // update globals for previous point
    prev_point = new_p;
    previous_norm = current_norm;
    previous_binorm = current_binorm;
    previous_tangent = current_tangent;
  }
}

/* Iterate over control points in the spline files */
/* Return 4 control points in an array */
void iterateOverControlPoints() {
  for (int i = 0; i < g_iNumOfSplines; i++) { //iterate over number of spline files
    struct spline *the_spline;
    the_spline = &g_Splines[i]; // the_spline is a pointer to structure g_splines[i]

    for (int j = 0; j < the_spline->numControlPoints - 3; j++) {
      //printf("Control point %d is %lf %lf %lf\n", j, the_spline->points[j].x, the_spline->points[j].y, the_spline->points[j].z);
      struct point p1;
      struct point p2;
      struct point p3;
      struct point p4;
 
      p1 = the_spline->points[j];
      p2 = the_spline->points[j+1];
      p3 = the_spline->points[j+2];
      p4 = the_spline->points[j+3];
      constructSpline(p1, p2, p3, p4);
    }
  }
}
 
 /* Eye coordinates are transformed to Clip Coordinates */
void reshape(int w, int h) 
{
  glViewport(0, 0, w, h);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  // setup projection
  gluPerspective(45.0, w/h, 0.01, 10);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void positionCamera()
{
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  /* Modeling transformations */
  // translation
  glTranslatef(g_vLandTranslate[0], g_vLandTranslate[1], g_vLandTranslate[2]);

  // rotation
  glRotatef(g_vLandRotate[0], 1.0, 0.0, 0.0);
  glRotatef(g_vLandRotate[1], 0.0, 1.0, 0.0);
  glRotatef(g_vLandRotate[2], 0.0, 0.0, 1.0);

  //scaling
  glScalef(g_vLandScale[0], g_vLandScale[1], g_vLandScale[2]);
}

void face(int a, int b, int c, int d, FACES face)
{   
  if (face == BACK) {
    glBegin(GL_POLYGON);
      glTexCoord2f(0.0, 0.50);
        glVertex3fv(vertices[a]);
      glTexCoord2f(0.0, 0.0);
        glVertex3fv(vertices[b]);
      glTexCoord2f(0.25, 0.0);
        glVertex3fv(vertices[c]);
      glTexCoord2f(0.25, 0.50);
        glVertex3fv(vertices[d]);
    glEnd(); 
  } else if (face == LEFT) {
    glBegin(GL_POLYGON);
      glTexCoord2f(0.25, 0.50);
        glVertex3fv(vertices[a]);
      glTexCoord2f(0.25, 0.0);
        glVertex3fv(vertices[b]);
      glTexCoord2f(0.50, 0.0);
        glVertex3fv(vertices[c]);
      glTexCoord2f(0.50, 0.50);
        glVertex3fv(vertices[d]);
    glEnd();
  } else if (face == FRONT) {
    glBegin(GL_POLYGON);
      glTexCoord2f(0.50, 0.50);
        glVertex3fv(vertices[a]);
      glTexCoord2f(0.50, 0.0);
        glVertex3fv(vertices[b]);
      glTexCoord2f(0.75, 0.0);
        glVertex3fv(vertices[c]);
      glTexCoord2f(0.75, 0.50);
        glVertex3fv(vertices[d]);
    glEnd();
  } else if (face == RIGHT) {
    glBegin(GL_POLYGON);
      glTexCoord2f(0.75, 0.50);
        glVertex3fv(vertices[a]);
      glTexCoord2f(0.75, 0.0);
        glVertex3fv(vertices[b]);
      glTexCoord2f(1.0, 0.0);
        glVertex3fv(vertices[c]);
      glTexCoord2f(1.0, 0.50);
        glVertex3fv(vertices[d]);
    glEnd();
  } else if (face == TOP) {
    glBegin(GL_POLYGON);
      glTexCoord2f(0.50, 1.0);
        glVertex3fv(vertices[a]);
      glTexCoord2f(0.50, 0.50);
        glVertex3fv(vertices[b]);
      glTexCoord2f(0.75, 0.50);
        glVertex3fv(vertices[c]);
      glTexCoord2f(0.75, 1.0);
        glVertex3fv(vertices[d]);
    glEnd();
  } else if (face == FLOOR) {
    glBegin(GL_POLYGON);
      glTexCoord2f(0.0, 1.0);
        glVertex3fv(vertices[a]);
      glTexCoord2f(0.0, 0.0);
        glVertex3fv(vertices[b]);
      glTexCoord2f(1.0, 0.0);
        glVertex3fv(vertices[c]);
      glTexCoord2f(1.0, 1.0);
        glVertex3fv(vertices[d]);
    glEnd();
  }
}

void cube()
{
  // use texture color directly
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE); 

  glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texName[1]);
      face(0,3,2,1, FRONT);
      face(2,3,7,6, TOP);
      face(0,4,7,3, LEFT);
      face(1,2,6,5, RIGHT);
      face(4,5,6,7, BACK);

    glBindTexture(GL_TEXTURE_2D, texName[0]);
      face(0,1,5,4, FLOOR);
  glDisable(GL_TEXTURE_2D);
}

void display()
{
  // clear buffers
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // update camera
  positionCamera();

  // move camera
  struct spline *the_spline;
  the_spline = &g_Splines[0];

  // reset u and get the next 4 control points
  if (global_u >= 1.0) {
    global_u = 0.0;
    control_point_index += 1;
  }
  if (control_point_index+3 < the_spline->numControlPoints) {
    struct point c1 = the_spline->points[control_point_index];
    struct point c2 = the_spline->points[control_point_index+1];
    struct point c3 = the_spline->points[control_point_index+2];
    struct point c4 = the_spline->points[control_point_index+3];

    current_point = catmullRomCalc(global_u, c1, c2, c3, c4);
    double max = maxValue(current_point.x, current_point.y, current_point.z);
    if (max > 1) {
      current_point.x = current_point.x/max;
      current_point.y = current_point.y/max;
      current_point.z = current_point.z/max;
    } 

    //printf("global_u: %f\n", global_u);
    //printf("Move camera points: [%f %f %f]\n", current_point.x, current_point.y, current_point.z);
    moveCamera(global_u, c1, c2, c3, c4);
    
  } else {
    // reset
    shouldCalcInitialNormal = true;
    control_point_index = 0;
    global_u = 0.0;
  }

  // enclose scene in cube that is texture mapped
  cube();

  // draw spline within the cube
  iterateOverControlPoints();

  // double buffer, so swap buffers when done drawing
  glutSwapBuffers();
}

void menufunc(int value)
{
  switch (value)
  {
    case 0:
      exit(0);
      break;
  }
}

void doIdle()
{
  /* Capture a screenshot of the window */
  // char fileName[2048];

  /* create filenames up to 1000 filenames */
  // sprintf(fileName, "anim.%04d.jpg", screenshotNumber);
  // saveScreenshot(fileName);

  // screenshotNumber++;

  /* make the screen update */
  if (global_u < 1.0) {
    global_u = global_u + 0.01;
  }
  glutPostRedisplay();
}

/* converts mouse drags into information about 
rotation/translation/scaling */
void mousedrag(int x, int y)
{
  int vMouseDelta[2] = {x-g_vMousePos[0], y-g_vMousePos[1]};
  
  switch (g_ControlState)
  {
    case TRANSLATE:  
      if (g_iLeftMouseButton)
      {
        g_vLandTranslate[0] += vMouseDelta[0]*0.01;
        g_vLandTranslate[2] += vMouseDelta[1]*0.01;
        //g_vLandTranslate[1] += vMouseDelta[1]*0.01;
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandTranslate[1] += vMouseDelta[1]*0.01;
        //g_vLandTranslate[2] += vMouseDelta[1]*0.01;
      }
      break;
    case ROTATE:
      if (g_iLeftMouseButton)
      {
        g_vLandRotate[0] += vMouseDelta[1];
        g_vLandRotate[1] -= vMouseDelta[0];
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandRotate[2] += vMouseDelta[1];
      }
      break;
    case SCALE:
      if (g_iLeftMouseButton)
      {
        g_vLandScale[0] *= 1.0+vMouseDelta[0]*0.01;
        g_vLandScale[1] *= 1.0-vMouseDelta[1]*0.01;
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandScale[2] *= 1.0-vMouseDelta[1]*0.01;
      }
      break;
  }
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mouseidle(int x, int y)
{
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

/* Control the transformations and color states with keyboard input */
void keyboardButton(unsigned char key, int x, int y) 
{
  switch (key) {
    case 't':
      g_ControlState = TRANSLATE;
      break;
    case 's':
      g_ControlState = SCALE;
      break;
    case 'r':
      g_ControlState = ROTATE;
      break;
  } 

  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mousebutton(int button, int state, int x, int y)
{
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      g_iLeftMouseButton = (state==GLUT_DOWN);
      break;
    case GLUT_MIDDLE_BUTTON:
      g_iMiddleMouseButton = (state==GLUT_DOWN);
      break;
    case GLUT_RIGHT_BUTTON:
      g_iRightMouseButton = (state==GLUT_DOWN);
      break;
  }

  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

int main (int argc, char ** argv)
{
  if (argc<2)
  {  
  printf ("usage: %s <trackfile>\n", argv[0]);
  exit(0);
  }

  loadSplines(argv[1]);

  glutInit(&argc, argv);

  /* request double buffer */
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);

  /* set window size */
  glutInitWindowSize(640, 480);
  
  /* set window position */
  glutInitWindowPosition(0, 0);
  
  /* creates a window */
  glutCreateWindow("Welcome to the Coaster");

  /* do initialization */
  myInit();

  /* load the floor and sky images into textures */
  initTextures();

  /* tells glut to use a particular display function */
  glutDisplayFunc(display);

  /* allow the user to quit using the right mouse button menu */
  g_iMenuId = glutCreateMenu(menufunc);
  glutSetMenu(g_iMenuId);
  glutAddMenuEntry("Quit", 0);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  /* replace with any animate code */
  glutIdleFunc(doIdle);

  /* callback for reshaping the window */
  glutReshapeFunc(reshape);

  /* callback for mouse drags */
  glutMotionFunc(mousedrag);

  /* callback for idle mouse movement */
  glutPassiveMotionFunc(mouseidle);

  /* callback for mouse button changes */
  glutMouseFunc(mousebutton);

  /* callback to bind the keyboard for translation, scaling */
  glutKeyboardFunc(keyboardButton);

  glutMainLoop();

  return 0;
}
