#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdbool.h>
#include <pthread.h>

#include "mandelCore.h"

#define WinW 300
#define WinH 300
#define ZoomStepFactor 0.5
#define ZoomIterationFactor 2

static Display *dsp = NULL;
static unsigned long curC;
static Window win;
static GC gc;

volatile bool *worker_activate;

struct job_parameters{
  mandel_Pars *pars;
  int maxIterations;
  int *res;
  int result_available;
};

volatile struct job_parameters *worker_parameters;
/* basic win management rountines */

static void openDisplay() {
  if (dsp == NULL) {
    dsp = XOpenDisplay(NULL);
  }
}

static void closeDisplay() {
  if (dsp != NULL) {
    XCloseDisplay(dsp);
    dsp=NULL;
  }
}

void openWin(const char *title, int width, int height) {
  unsigned long blackC,whiteC;
  XSizeHints sh;
  XEvent evt;
  long evtmsk;

  whiteC = WhitePixel(dsp, DefaultScreen(dsp));
  blackC = BlackPixel(dsp, DefaultScreen(dsp));
  curC = blackC;

  win = XCreateSimpleWindow(dsp, DefaultRootWindow(dsp), 0, 0, WinW, WinH, 0, blackC, whiteC);

  sh.flags=PSize|PMinSize|PMaxSize;
  sh.width=sh.min_width=sh.max_width=WinW;
  sh.height=sh.min_height=sh.max_height=WinH;
  XSetStandardProperties(dsp, win, title, title, None, NULL, 0, &sh);

  XSelectInput(dsp, win, StructureNotifyMask|KeyPressMask);
  XMapWindow(dsp, win);
  do {
    XWindowEvent(dsp, win, StructureNotifyMask, &evt);
  } while (evt.type != MapNotify);

  gc = XCreateGC(dsp, win, 0, NULL);

}

void closeWin() {
  XFreeGC(dsp, gc);
  XUnmapWindow(dsp, win);
  XDestroyWindow(dsp, win);
}

void flushDrawOps() {
  XFlush(dsp);
}

void clearWin() {
  XSetForeground(dsp, gc, WhitePixel(dsp, DefaultScreen(dsp)));
  XFillRectangle(dsp, win, gc, 0, 0, WinW, WinH);
  flushDrawOps();
  XSetForeground(dsp, gc, curC);
}

void drawPoint(int x, int y) {
  XDrawPoint(dsp, win, gc, x, WinH-y);
  flushDrawOps();
}

void getMouseCoords(int *x, int *y) {
  XEvent evt;

  XSelectInput(dsp, win, ButtonPressMask);
  do {
    XNextEvent(dsp, &evt);
  } while (evt.type != ButtonPress);
  *x=evt.xbutton.x; *y=evt.xbutton.y;
}

/* color stuff */

void setColor(char *name) {
  XColor clr1,clr2;

  if (!XAllocNamedColor(dsp, DefaultColormap(dsp, DefaultScreen(dsp)), name, &clr1, &clr2)) {
    printf("failed\n"); return;
  }
  XSetForeground(dsp, gc, clr1.pixel);
  curC = clr1.pixel;
}

char *pickColor(int v, int maxIterations) {
  static char cname[128];

  if (v == maxIterations) {
    return("black");
  }
  else {
    sprintf(cname,"rgb:%x/%x/%x",v%64,v%128,v%256);
    return(cname);
  }
}

void* worker_thread(void* args);

int main(int argc, char *argv[]) {
  mandel_Pars pars,*slices;
  int i,j,x,y,nofslices,maxIterations,level,*res;
  int xoff,yoff;
  long double reEnd,imEnd,reCenter,imCenter;
  pthread_t *threads;
  int iret;
  int workers_done=0;

  printf("\n");
  printf("This program starts by drawing the default Mandelbrot region\n");
  printf("When done, you can click with the mouse on an area of interest\n");
  printf("and the program will automatically zoom around this point\n");
  printf("\n");
  printf("Press enter to continue\n");
  getchar();

  pars.reSteps = WinW; /* never changes */
  pars.imSteps = WinH; /* never changes */

  /* default mandelbrot region */

  pars.reBeg = (long double) -2.0;
  reEnd = (long double) 1.0;
  pars.imBeg = (long double) -1.5;
  imEnd = (long double) 1.5;
  pars.reInc = (reEnd - pars.reBeg) / pars.reSteps;
  pars.imInc = (imEnd - pars.imBeg) / pars.imSteps;

  printf("enter max iterations (50): ");
  scanf("%d",&maxIterations);
  printf("enter no of slices: ");
  scanf("%d",&nofslices);

  /* adjust slices to divide win height */

  while (WinH % nofslices != 0) { nofslices++;}

  /* allocate slice parameter and result arrays */

  slices = (mandel_Pars *) malloc(sizeof(mandel_Pars)*nofslices);
  res = (int *) malloc(sizeof(int)*pars.reSteps*pars.imSteps);

  /* open window for drawing results */

  openDisplay();
  openWin(argv[0], WinW, WinH);

  level = 1;

  //Allocate memory for workers
  worker_activate = (bool*) calloc(nofslices, sizeof(bool*));
  if (worker_activate==NULL){
      printf("Error,dynamic memory %d line\n",__LINE__);
      return(EXIT_FAILURE);
  }

  worker_parameters = (struct job_parameters*) malloc(nofslices*sizeof(struct job_parameters));
  if (worker_parameters==NULL){
      printf("Error, dynamic memory %d line\n",__LINE__);
      return(EXIT_FAILURE);
  }

  threads = (pthread_t*) malloc(nofslices * sizeof(pthread_t));
  if (threads==NULL){
      printf("Error, dynamic memory %d line\n",__LINE__);
      return(EXIT_FAILURE);
  }

    //Create worker threads
    for (i=0; i<nofslices; i++){
      int *arg = malloc(sizeof(*arg));
      if (arg==NULL){
        printf("Error, dynamic memory %d line\n",__LINE__);
        return(EXIT_FAILURE);
      }
      *arg=i;
      iret = pthread_create(threads+i, NULL, worker_thread, (void*) arg);
      if (iret){
        fprintf(stderr, "ERROR pthread create return: %d\n", iret);
        return(EXIT_FAILURE);
      }
    }

  while (1) {

    clearWin();

    mandel_Slice(&pars,nofslices,slices);

    y=0;
    for (i=0; i<nofslices; i++) {
      worker_parameters[i].pars = &slices[i];
      worker_parameters[i].result_available=0;
      worker_parameters[i].maxIterations = maxIterations;
      worker_parameters[i].res = &res[i*slices[i].imSteps*slices[i].reSteps];
    }

    for (i=0; i<nofslices; i++){
        worker_activate[i]=true;
    }
    
    while(workers_done<nofslices){
        for (i=0; i<nofslices; i++){
            if(worker_parameters[i].result_available){
                y = i*slices[i].imSteps;
                for (j=0; j<slices[i].imSteps; j++) {
	                for (x=0; x<slices[i].reSteps; x++) {
                        setColor(pickColor(res[y*slices[i].reSteps+x],maxIterations));
                        drawPoint(x,y);
                    }
                    y++;
                }
                worker_parameters[i].result_available=0;
                workers_done++;
            }
        }
    }
    workers_done=0;

    /* get next focus/zoom point */

    getMouseCoords(&x,&y);
    xoff = x;
    yoff = WinH-y;

    /* adjust region and zoom factor  */

    reCenter = pars.reBeg + xoff*pars.reInc;
    imCenter = pars.imBeg + yoff*pars.imInc;
    pars.reInc = pars.reInc*ZoomStepFactor;
    pars.imInc = pars.imInc*ZoomStepFactor;
    pars.reBeg = reCenter - (WinW/2)*pars.reInc;
    pars.imBeg = imCenter - (WinH/2)*pars.imInc;

    maxIterations = maxIterations*ZoomIterationFactor;
    level++;

  }

  /* never reach this point; for cosmetic reasons */

  free(slices);
  free(res);

  closeWin();
  closeDisplay();

}

void* worker_thread(void* args){
  int *id = (int*) args;
  
  while (1){
    while(!worker_activate[*id]){};
    mandel_Calc(worker_parameters[*id].pars,worker_parameters[*id].maxIterations,worker_parameters[*id].res);
    worker_activate[*id]=false;
    worker_parameters[*id].result_available=1;
  }
}
