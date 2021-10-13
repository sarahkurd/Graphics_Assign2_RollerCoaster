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

void iterateAndDrawSplines() {
  for (int i = 0; i < g_iNumOfSplines; i++) { //iterate over number of spline files
    printf("File %d:", i);
    struct spline *the_spline;
    the_spline = &g_Splines[i]; // the_spline is a pointer to structure g_splines[i]
    printf("the_spline->numControlPoints: %d", the_spline->numControlPoints);
    for (int j = 0; j < the_spline->numControlPoints; j++) {
      printf("Control point %d is %lf %lf %lf", j, the_spline->points[j].x, the_spline->points[j].y, the_spline->points[j].z);
    }
  }
}

/* Each point's coordinate system is a functionnof the previous one.
Compute the tangent, normal, and binormal for given position u on the curve */
void computeLocalCoordinates(float u, point binorm_previous)
{
  // compute tan_new, norm_new, binorm_new
  // make camera up vector to be n_new
}

/* Calculate and return a point on the spline using the Catmull-Rom spline formula */
struct point catmullRomCalc(float u, struct point p1, struct point p2, struct point p3, struct point p4) 
{
  float u_cubed = u * u * u;
  float u_squared = u * u;
  struct point control_matrix[4] = {p1, p2, p3, p4};
  float u_matrix[4] = {u_cubed, u_squared, u, 1};

  struct point new_p;

  float c11 = (basis_catmull[0][0] * control_matrix[0].x) + (basis_catmull[0][1] * control_matrix[1].x) + (basis_catmull[0][2] * control_matrix[2].x) + (basis_catmull[0][1] * control_matrix[3].x)
  float c12 = (basis_catmull[0][0] * control_matrix[0].y) + (basis_catmull[0][1] * control_matrix[1].y) + (basis_catmull[0][2] * control_matrix[2].y) + (basis_catmull[0][1] * control_matrix[3].y)
  float c13 = (basis_catmull[0][0] * control_matrix[0].z) + (basis_catmull[0][1] * control_matrix[1].z) + (basis_catmull[0][2] * control_matrix[2].z) + (basis_catmull[0][1] * control_matrix[3].z)

  return new_p;
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

  glBegin(GL_POLYGON);
    glColor3f(1.0, 0.0, 0.0);
    glVertex3f(0.0, 0.0, -0.2);
    glVertex3f(0.0, 1.0, -0.2);
    glVertex3f(1.0, 0.0, -0.2);
  glEnd();

  // Construct Spline 
  // vary the parameter, "u", by 0.001
  for (float u = 0; u < 1.0; u += 0.001) {
    // insert each value of "u" into the Catmull-Rom equation and obtain the point at p(u)
  }

  // double buffer, so swap buffers when done drawing
  glutSwapBuffers();
}


int main (int argc, char ** argv)
{
  if (argc<2)
  {  
  printf ("usage: %s <trackfile>\n", argv[0]);
  exit(0);
  }

  loadSplines(argv[1]);

  iterateAndDrawSplines();

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

  /* callback for reshaping the window */
  glutReshapeFunc(reshape);

  glutMainLoop();

  return 0;
}
