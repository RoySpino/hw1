//cs335 Spring 2015 Lab-1
//This program demonstrates the use of OpenGL and XWindows
//
//Assignment is to modify this program.
//You will follow along with your instructor.
//
//Elements to be learned in this lab...
//
//. general animation framework
//. animation loop
//. object definition and movement
//. collision detection
//. mouse/keyboard interaction
//. object constructor
//. coding style
//. defined constants
//. use of static variables
//. dynamic memory allocation
//. simple opengl components
//. git
//
//elements we will add to program...
//. Game constructor
//. multiple particles
//. gravity
//. collision detection
//. more objects
//
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include <string>
#include "ppm.h"
extern "C" {
	#include "fonts.h"
}

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600

#define MAX_PARTICLES 20000
#define GRAVITY 0.1

//X Windows variables
Display *dpy;
Window win;
GLXContext glc;
GC gc;

using namespace std;

//Structures

struct Vec {
	float x, y, z;
};

struct Shape {
	float width, height;
	float radius;
	Vec center;
};

struct Particle {
	Shape s;
	Vec velocity;
};
/*
struct Rect {
	int left;
	int top;
	int right;
	int bot;
	int width;
	int height;
	int center;
	int centerx;
	int centery;
};*/

// linklist class
class list{
	struct lst{
		Particle data;
		lst *next;
		lst *prev;
	};
	lst *root, *end;
	int cnt;

	public:
	list(){
		root = end = NULL;
		cnt =0;
	}
	int count(){
		return cnt;
	}
	void add(Particle x){
		if(cnt > 1500)
			return;
		lst *node = new lst;

		node->next = NULL;
		node->prev = NULL;
		node->data = x;

		if(root==NULL) 
			end = root = node;
		else{
			end->next = node;
			node->prev = end;
			end = node;
		}
		cnt++;
	}
	void remove(Particle *p){
		lst *trav = root;

		for(int i=0; i<cnt; i++)
			if(p == &trav->data){
				if(trav == root){
					root = root->next;
					root->prev = NULL;
					delete trav;
					break;
				}else{
					if (trav == end){
						end = end->prev;
						end->next = NULL;
						delete trav;
						break;
					}else{
						trav->next->prev = trav->prev;
						trav->prev->next = trav->next;
						delete trav;
						break;
					}
				}
			}else{
				trav = trav->next;
			}
		cnt--;
	}
	Particle * operator [](int index){
		lst *trav = root;
		for(int i=0; i<cnt; i++){
			if(i == index)
				return &trav->data;
			trav = trav->next;
		}
		return &end->data;
	}
	~list(){
		lst *killnode, *trav;
		trav = root;
		while(trav->next != NULL){
			killnode = trav;
			trav = trav->next;
			delete killnode;
		}
		delete trav;
	}
};


struct Game {
	Shape box[7];
	//Particle particle[MAX_PARTICLES];
	list particle;
	int n, nbox;
};
int seedval = -1;
double maxvel = 10.5, 
       cx = 750,
       cy = 50, 
       ehx = 0,
       ehy =0,
       ehy2 =0,
       ehx2 =0,
       incr=0;
bool noTiles = false,
     blackhole = false,
     rotat = false;
string strarr[5]={
        "Requirements",
        "                       Design",
        "                                       Implementation",
        "                                                              Verification",
        "                                                                                     Maintenance"};
//Function prototypes
void initXWindows(void);
void init_opengl(void);
void cleanupXWindows(void);
void check_mouse(XEvent *e, Game *game);
int check_keys(XEvent *e, Game *game);
void movement(Game *game);
void render(Game *game);
void makeParticle(Game *game, int x, int y); 
void perpVec(Particle *p);
double random_();
void grav(Particle *p);

int main(void)
{
	int done=0;
	srand(time(NULL));
	initXWindows();
	init_opengl();
	//declare game object
	Game game;
	game.n=0;

	game.nbox = 5;
	//declare a box shape
	for(int i=0; i< game.nbox; i++){
		game.box[i].width = 100;
		game.box[i].height = 10;
		game.box[i].center.x = (-180+(80 * i)) + 5*65;
		game.box[i].center.y = (800 - (50 * i)) - 5*60;
	}

	//start animation
	while(!done) {
		while(XPending(dpy)) {
			XEvent e;
			XNextEvent(dpy, &e);
			check_mouse(&e, &game);
			done = check_keys(&e, &game);
		}

		movement(&game);
		render(&game);
		glXSwapBuffers(dpy, win);
	}
	cleanupXWindows();

	return 0;
}

void set_title(void)
{
	//Set the window title bar.
	XMapWindow(dpy, win);
	XStoreName(dpy, win, "335 Lab1   LMB for particle");
}


void cleanupXWindows(void) {
	//do not change
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
}

void initXWindows(void) {
	//do not change
	GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	int w=WINDOW_WIDTH, h=WINDOW_HEIGHT;
	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		cout << "\n\tcannot connect to X server\n" << endl;
		exit(EXIT_FAILURE);
	}
	Window root = DefaultRootWindow(dpy);
	XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
	if(vi == NULL) {
		cout << "\n\tno appropriate visual found\n" << endl;
		exit(EXIT_FAILURE);
	} 


	Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
	XSetWindowAttributes swa;
	swa.colormap = cmap;

	swa.event_mask = ExposureMask | 
		KeyPressMask | KeyReleaseMask |
		ButtonPress | ButtonReleaseMask |
		PointerMotionMask | StructureNotifyMask | 
		SubstructureNotifyMask;
	win = XCreateWindow(dpy, root, 0, 0, w, h, 0, vi->depth,
			InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
	set_title();
	glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
	glXMakeCurrent(dpy, win, glc);
}


void init_opengl(void)
{
	//OpenGL initialization
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	//Initialize matrices
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	//Set 2D mode (no perspective)
	glOrtho(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT, -1, 1);
	//Set the screen background color
	glClearColor(0.1, 0.1, 0.1, 1.0);
	
	glEnable(GL_TEXTURE_2D);
	initialize_fonts();
}

void makeParticle(Game *game, int x, int y) {

	if (game->n >= MAX_PARTICLES)
		return;
	//cout << "makeParticle() " << x << " " << y << endl;
	//position of particle
	//Particle *p = &game->particle[game->n];
	Particle *p = new Particle;
	p->s.center.x = x;
	p->s.center.y = y;
	p->velocity.y = ((-random_() > .8)? 1 : -1) * random_();
	p->velocity.x = random_();

	game->particle.add(*p);
	game->n +=1
		;
	//	cout << game->particle[game->n-1] << endl;

	//cout << p << endl;
}

void check_mouse(XEvent *e, Game *game)
{
	static int savex = 0;
	static int savey = 0;
	static int n = 0;

	if (e->type == ButtonRelease) {
		return;
	}
	if (e->type == ButtonPress) {
		if (e->xbutton.button==1) {
			//Left button was pressed
			//int y = WINDOW_HEIGHT - e->xbutton.y;
			return;
		}
		if (e->xbutton.button==3) {
			//Right button was pressed
			return;
		}
	}
	//Did the mouse move?
	if (savex != e->xbutton.x || savey != e->xbutton.y) {
		savex = e->xbutton.x;
		savey = e->xbutton.y;
		if (++n < 10)
			return;
	}
}

int check_keys(XEvent *e, Game *game)
{
	//Was there input from the keyboard?
	if (e->type == KeyPress) {
		int key = XLookupKeysym(&e->xkey, 0);
		if (key == XK_Escape) {
			return 1;
		}
		// disable tiles
		if (key == XK_Up) {
			noTiles = (noTiles == false)?true:false;
		}
		// enable black hole
		if (key == XK_Down) {
			blackhole = (blackhole == false)?true:false;
		}
		if (key == XK_Left) {
			rotat = (rotat == false)?true:false;
		}
		if (key == XK_Right) {
		}
		//You may check other keys here.

	}
	return 0;
}

void movement(Game *game)
{
	Particle *p;
	float hx,hy,evHor;
	bool flip;

	for(int i=0; i< 5; i++)
		makeParticle(game, 80, WINDOW_HEIGHT-50);

	if (game->n <= 0)
		return;

	for(int i=0; i<game->particle.count(); i++){
		p = game->particle[i];
		p->s.center.x += p->velocity.x;
		p->s.center.y += p->velocity.y;

		//check for collision with rectangles...
		if(noTiles == false){
			Shape s;
			for(int x=0; x< game->nbox; x++){
				s = game->box[x];
				if(p->s.center.y < (s.center.y + s.height) and
						p->s.center.y > (s.center.y - s.height) and
						p->s.center.x < (s.center.x + s.width) and
						p->s.center.x > (s.center.x - s.width)){
					p->velocity.y =0.0;
					p->velocity.x *=1.013;
					break;
				}
				else{
					grav(p);
				}
			}
		}else{
			grav(p);
		}

		// rotate the black hole
		if(rotat == true){
			incr += 0.00002;

			ehx = (WINDOW_WIDTH/2.0) + 150 * cos(incr);
			ehy = (WINDOW_HEIGHT/2.0) + 150 * sin(incr);
			ehy2 = (WINDOW_HEIGHT/2.0) - 170 *sin(incr);
			ehx2 = (WINDOW_WIDTH/2.0) - 170 *cos(incr);
		}else{
			ehx = WINDOW_WIDTH/2.0;
			ehy = WINDOW_HEIGHT/2.0;
		}

		// check for collision with circle 
		hx = cx - p->s.center.x;
		hy = cy - p->s.center.y;
		evHor = sqrt(hx*hx + hy*hy);

		if (evHor <= 135){
			flip = (random_() >.6);
			p->velocity.x = ((flip?-1:1)*
					((cy) - p->s.center.y)/evHor)*(flip?5:10);
			p->velocity.y = ((flip?1:-1)*
					((cx) - p->s.center.x)/evHor)*(flip?5:10);
		}
		



		//check for off-screen
		if(blackhole == false){
			if (p->s.center.y < 0.0 or p->s.center.y > WINDOW_HEIGHT or
					p->s.center.x <0 or p->s.center.x > WINDOW_WIDTH) {
				//cout << "off screen" << endl;
				game->particle.remove(p);
				game->n -=1;
			}
		}else{
			// check to see if the particle passes the event horizon
			hx = ehx - p->s.center.x;
			hy = ehy - p->s.center.y;
			evHor = (hx*hx + hy*hy);

			if (evHor <= 800){
				game->particle.remove(p);
				game->n -=1;
			}
			if(rotat == true){
				hx = ehx2 - p->s.center.x;
				hy = ehy2 - p->s.center.y;
				evHor = (hx*hx + hy*hy);

				if (evHor <= 800){
					game->particle.remove(p);
					game->n -=1;
				}
			}
		}
	}
}
void grav(Particle *p){
	if(blackhole == false)
		p->velocity.y -= 0.05;
	else{
		float hx,hy,d;
		hx = ehx - p->s.center.x;
		hy = ehy - p->s.center.y;
		d = sqrt(hx*hx + hy*hy);
		hx = (hx /d) * 1.2;
		hy = (hy /d) * 2;
		p->velocity.y += hy;
		p->velocity.x += hx;
			
		
		if(rotat == true){
			hx = ehx2 - p->s.center.x;
			hy = ehy2 - p->s.center.y;
			d = sqrt(hx*hx + hy*hy);
			hx = (hx /d) * 1.2;
			hy = (hy /d) * 2;
			p->velocity.y += hy;
			p->velocity.x += hx;
		}
		
	}
}
double random_(){
	if(seedval == -1)
		srand(time(0));
	else
		srand(seedval);
	seedval = rand();

	return (rand()%100)/100.0;
}

void perpVec(Particle *p){

}
void render(Game *game)
{
	float w, h;
	glClear(GL_COLOR_BUFFER_BIT);
	Rect r;
	r.bot = WINDOW_HEIGHT - 20;
	r.left = 100;
	r.center = 0;
	r.top = WINDOW_HEIGHT-100;
	//Draw shapes...
	
	//draw boxes
	ggprint8b(&r, 85, 0x00ffffff, "");
	if(noTiles == false){
		Shape *s;
		for(int x=0; x<game->nbox; x++){
			glColor3ub(90,140,90);
			s = &game->box[x];
			glPushMatrix();
			glTranslatef(s->center.x, s->center.y, s->center.z);
			w = s->width;
			h = s->height;
			glBegin(GL_QUADS);
			glVertex2i(-w,-h);
			glVertex2i(-w, h);
			glVertex2i( w, h);
			glVertex2i( w,-h);
			glEnd();
			glPopMatrix();
			ggprint8b(&r, 50, 0x00ffffff, strarr[x].data());
		}

	}
	// draw circle
	float th,thx,thy;
	glPushMatrix();
	glColor3ub(90,140,90);
	glTranslatef(cx, cy, 0);
	glBegin(GL_POLYGON);
	for(int z=0; z < 40; z++){
		th = 2 * 3.14159265 *z / 40;
		thx = 130 * cos(th);
		thy = 130 * sin(th);
		glVertex2f(thx,thy);
	}
	glEnd();
	glPopMatrix();

	if(blackhole == true or rotat == true){
		if(rotat == true){
			// draw second EVENT Horizon
			th =0;
			glPushMatrix();
			glColor3ub(0,0,0);
			glTranslatef(ehx2, ehy2, 0);
			glBegin(GL_POLYGON);
			for(int z=0; z < 40; z++){
				th = 2 * 3.14159265 *z / 40;
				thx = 30 * cos(th);
				thy = 30 * sin(th);
				glVertex2f(thx,thy);
			}
			glEnd();
			glPopMatrix();
	 	}
		// draw EVENT Horizon
		th =0;
		glPushMatrix();
		glColor3ub(0,0,0);
		glTranslatef(ehx, ehy, 0);
		glBegin(GL_POLYGON);
		for(int z=0; z < 40; z++){
			th = 2 * 3.14159265 *z / 40;
			thx = 30 * cos(th);
			thy = 30 * sin(th);
			glVertex2f(thx,thy);
		}
		glEnd();
		glPopMatrix();
	}

	//draw all particles here
	Particle *p;
	Vec *c;
	float xt;
	for(int i=0; i<game->particle.count(); i++){
		p = game->particle[i];
		c = &p->s.center;
		xt = int(sqrt((p->velocity.x * p->velocity.x)+(p->velocity.y+ p->velocity.y)))%100;
		glPushMatrix();
		glColor3ub(220*(xt),0,220*abs(100-xt));
		w = 2;
		h = 2;
		glBegin(GL_QUADS);
		glVertex2i(c->x-w, c->y-h);
		glVertex2i(c->x-w, c->y+h);
		glVertex2i(c->x+w, c->y+h);
		glVertex2i(c->x+w, c->y-h);
		glEnd();
		glPopMatrix();
	}
}
