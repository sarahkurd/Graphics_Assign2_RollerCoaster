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
#include "pic.h"

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

/* Get each point on the spline by varying the parameter, "u", by 0.001 */
/* Draw a new vertex for each point generated by Catmull-Rom calculation */
void constructSpline(struct point p1, struct point p2, struct point p3, struct point p4) 
{
  glColor3f(1.0, 0.0, 0.0);
  glBegin(GL_LINES);
    for (float u = 0; u < 1.0; u += 0.001) {
      // insert each value of "u" into the Catmull-Rom equation and obtain the point at p(u)
      struct point new_p;
      new_p = catmullRomCalc(u, p1, p2, p3, p4);
      glVertex3d(new_p.x, new_p.y, new_p.z);
    }
  glEnd();
}

/* This function is needed to position the camera normal and tangent correctly */
/* Each point's coordinate system is a function of the previous one. */
/* Compute the tangent, normal, and binormal for given position u on the curve */
void computeLocalCoordinates(float u, point binorm_previous)
{
  // compute tan_new, norm_new, binorm_new
  // make camera up vector to be n_new
}

/* Iterate over control points in the spline files */
/* Return 4 control points in an array */
void iterateOverControlPoints() {
  for (int i = 0; i < g_iNumOfSplines; i++) { //iterate over number of spline files
    printf("File %d:\n", i);
    struct spline *the_spline;
    the_spline = &g_Splines[i]; // the_spline is a pointer to structure g_splines[i]
    printf("the_spline->numControlPoints: %d\n", the_spline->numControlPoints);

    for (int j = 0; j < the_spline->numControlPoints; j++) {
      printf("Control point %d is %lf %lf %lf\n", j, the_spline->points[j].x, the_spline->points[j].y, the_spline->points[j].z);
    }
  }
}
 
void reshape(int w, int h) 
{
  glViewport(0, 0, w, h);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  // setup projection
  gluPerspective(60.0, w/h, 0.1, 1000.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void positionCamera()
{
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // place the camera (viewing transformation) 
  // parameters: eyex, eyey, eyez, focusx, focusy, focusz, upx, upy, upz 
  gluLookAt(0.0, 0.0, 0.0, 0.0, 0.0, -10.0, 0.0, 1.0, 0.0);
}

void display()
{
  // clear buffers
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // update camera
  positionCamera();

  iterateOverControlPoints();

  glBegin(GL_POLYGON);
    glColor3f(1.0, 0.0, 0.0);
    glVertex3f(0.0, 0.0, -0.2);
    glVertex3f(0.0, 1.0, -0.2);
    glVertex3f(1.0, 0.0, -0.2);
  glEnd();

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

  /* tells glut to use a particular display function */
  glutDisplayFunc(display);

  /* allow the user to quit using the right mouse button menu */
  g_iMenuId = glutCreateMenu(menufunc);
  glutSetMenu(g_iMenuId);
  glutAddMenuEntry("Quit", 0);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  /* callback for reshaping the window */
  glutReshapeFunc(reshape);

  glutMainLoop();

  return 0;
}
