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

/* Each point's coordinate system is a functionnof the previous one.
Compute the tangent, normal, and binormal for given position u on the curve */
void computeLocalCoordinates(float u, point binorm_previous)
{
  // compute tan_new, norm_new, binorm_new
  // make camera up vector to be n_new
  return 0;
}
 
void reshape(int w, int h) 
{
  glViewport(0, 0, w, h);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  // setup projection
  gluPerspective(45.0, w/h, 0.1, 1000.0);

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

  glBegin(GL_TRIANGLES);
    glColor3f(1.0, 0.0, 0.0);
    glVertex3f(0.0, 0.0, -0.2);
    glVertex3f(0.0, 1.0, -0.2);
    glVertex3f(1.0, 0.0, -0.2);
  glEnd();

  // double buffer, so swap buffers when done drawing
  glutSwapBuffers();
}


int main (int argc, char ** argv)
{
  // if (argc<2)
  // {  
  // printf ("usage: %s <trackfile>\n", argv[0]);
  // exit(0);
  // }

  // loadSplines(argv[1]);

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
