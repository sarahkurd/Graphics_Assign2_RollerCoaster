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

float global_u = 0.0;
int control_point_index = 0;
struct point prev_binorm;

/* vertices of cube about the origin */
GLfloat vertices[8][3] =
    {{-1.0, -1.0, -1.0}, {1.0, -1.0, -1.0},
    {1.0, 1.0, -1.0}, {-1.0, 1.0, -1.0}, {-1.0, -1.0, 1.0},
    {1.0, -1.0, 1.0}, {1.0, 1.0, 1.0}, {-1.0, 1.0, 1.0}};

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

/* Need to Compute T, N, and B */
struct point moveCamera(float u, struct point p1, struct point p2, struct point p3, struct point p4, struct point binorm_previous, struct point current_point) {
  struct point norm;
  struct point binormal;

  // compute tangent vector
  struct point tangent = computeTangentVector(u, p1, p2, p3, p4);
  printf("Tangent: [%f %f %f]\n", tangent.x, tangent.y, tangent.z);

  if (u == 0.0) { // if we are at the first point then pick arbitrary Vector
    binorm_previous.x = tangent.y;
    binorm_previous.y = tangent.x;
    binorm_previous.z = tangent.z;
  }
  // Normal = Binorm_previous x T
  norm.x = (binorm_previous.y * tangent.z) - (binorm_previous.z * tangent.y);
  norm.y = (binorm_previous.z * tangent.x) - (binorm_previous.x * tangent.z);
  norm.z = (binorm_previous.x * tangent.y) - (binorm_previous.y * tangent.x);
  printf("Normal: [%f %f %f]\n", norm.x, norm.y, norm.z);


  // Binormal = T x N
  binormal.x = (tangent.y * norm.z) - (tangent.z * norm.y);
  binormal.y = (tangent.z * norm.x) - (tangent.x * norm.z);
  binormal.z = (tangent.x * norm.y) - (tangent.y * norm.x);
  printf("binorm: [%f %f %f]\n", binormal.x, binormal.y, binormal.z);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // update the camera (viewing transformation) 
  // parameters: eyex, eyey, eyez, focusx, focusy, focusz, upx, upy, upz 
  gluLookAt(current_point.x, current_point.y, current_point.z, tangent.x, tangent.y, tangent.z, norm.x, norm.y, norm.z);

  return binormal;
}

/* Get each point on the spline by varying the parameter, "u", by 0.001 */
/* Draw a new vertex for each point generated by Catmull-Rom calculation */
/* Then, update the camera position */
void constructSpline(struct point p1, struct point p2, struct point p3, struct point p4) 
{
  struct point binormal; // update binormal on each iteration
  binormal.x = 0;
  binormal.y = 0;
  binormal.z = 0;

  glColor3f(1.0, 1.0, 0.0);
  glLineWidth((GLfloat)10.0);
  glBegin(GL_POINTS);

    for (float u = 0; u < 1.0; u += 0.01) {
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
      glVertex3d(new_p.x, new_p.y, new_p.z);
    }

  glEnd();
}

/* Iterate over control points in the spline files */
/* Return 4 control points in an array */
void iterateOverControlPoints() {
  for (int i = 0; i < g_iNumOfSplines; i++) { //iterate over number of spline files
    //printf("File %d:\n", i);
    struct spline *the_spline;
    the_spline = &g_Splines[i]; // the_spline is a pointer to structure g_splines[i]
    //printf("the_spline->numControlPoints: %d\n", the_spline->numControlPoints);

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
  printf("global u: %f\n", global_u);
  if (global_u >= 1.0) {
    global_u = 0.0;
    control_point_index += 1;
  }
  struct point c1 = the_spline->points[control_point_index];
  struct point c2 = the_spline->points[control_point_index+1];
  struct point c3 = the_spline->points[control_point_index+2];
  struct point c4 = the_spline->points[control_point_index+3];

  struct point new_p;
  new_p = catmullRomCalc(global_u, c1, c2, c3, c4);
  double max = maxValue(new_p.x, new_p.y, new_p.z);
  if (max > 1) {
    new_p.x = new_p.x/max;
    new_p.y = new_p.y/max;
    new_p.z = new_p.z/max;
  } 

  prev_binorm = moveCamera(global_u, c1, c2, c3, c4, prev_binorm, new_p);

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
  if (global_u <= 1.0) {
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
