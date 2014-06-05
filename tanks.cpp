#include <GL/gl.h>
#include <GL/glu.h>
#if defined (__APPLE__) || defined(MACOSX)
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct Window {
    int width;
    int height;
    int id;
    std::string title;
};
typedef struct Window Window;
Window window;

struct Vertex {
    float x, y, z;
};
typedef struct Vertex Vertex;

#define MOUNTAIN_POINTS 25
Vertex mtns[MOUNTAIN_POINTS];
Vertex tank;
Vertex enemy_tank;
Vertex* proj;
Vertex* proj_vel;
std::vector<Vertex> proj_path;
float power;
float angle;
float wind;

GLfloat enemy_color[3];

std::string center_message;
unsigned int center_message_countdown;

#define FPS 30
#define MILLIS (1000/FPS)

#define RAD_TO_DEG(a) (a * 180 / M_PI)
#define DEG_TO_RAD(a) (a * M_PI / 180)
#define POWER_STEP 10
#define ANGLE_STEP DEG_TO_RAD(5)
#define POWER_SCALE 0.0001
#define GRAV (9.8 * POWER_SCALE)
#define WIND_SCALE 0.00001

// prototypes
void draw_string(float x, float y, float z, std::string string);
void set_center_message(std::string msg, unsigned int time);
void reshape(int, int);
void draw_sky();
void draw_mountains();
void draw_tanks();
void draw_projectile();
void draw_center_message();
void display();
void fire();
void init_window();
void init_mountain();
void init_tanks();
void keyboard(unsigned char key, int x, int y);
void specialKeyboard(int key, int x, int y);
void init_gl();
void init_glut();
void timer(int value);
int main(int, char**);

void
draw_string(float x, float y, float z, std::string string) {
    //std::cerr << "Move: (" << x << ", " << y << ", " << z << "); ";
    glRasterPos3f(x, y, z);
    for (unsigned int i = 0; i < string.length(); i++) {
        //std::cerr << string[i];
        //glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, string[i]);
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, string[i]);
    }
    //std::cerr << std::endl;
}

void
set_center_message(std::string msg, unsigned int time) {
    center_message = msg;
    center_message_countdown = time;
}

void
draw_center_message() {
    if (center_message_countdown > 0) {
        draw_string(0, 0, 1, center_message);
        center_message_countdown--;
    }
}

void
reshape(int x, int y) {
    //std::cerr << "reshape(" << x << ", " << y << ")" << std::endl;
    glViewport(0, 0, x, y);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 10);
    gluLookAt(0, 0, 5, 0, 0, 0, 0, 1, 0);
}

void
draw_sky() {
    //std::cerr << "draw_sky()" << std::endl;
    // gradient box
    glBegin(GL_QUADS);
    glColor3f(0.0, 0.0, 0.2);
    glVertex3f(-1, -1, 0);
    glVertex3f(1, -1, 0);
    glColor3f(0.1, 0.1, 1.0);
    glVertex3f(1, 1, 0);
    glVertex3f(-1, 1, 0);
    glEnd();
}

void
draw_mountains() {
    //std::cerr << "draw_mountains()" << std::endl;
    glColor3f(0.2, 0.8, 0.2);
    glBegin(GL_POLYGON);
    glVertex3f(-1, -1, 0.1);
    for (int i = 0; i < MOUNTAIN_POINTS; i++) {
        Vertex p = mtns[i];
        glVertex3f(p.x, p.y, p.z);
    }
    glVertex3f(1, -1, 0.1);
    glEnd();
}

void
draw_tanks() {
    //std::cerr << "draw_tanks()" << std::endl;
    glColor3f(1, 1, 1);
    std::ostringstream tank_pos;
    tank_pos << "Tank @ (" << tank.x << ", " << tank.y << ", " << tank.z << "); Power: " << power << "; Angle: " << RAD_TO_DEG(angle);
    draw_string(-1, 0.95, 2, tank_pos.str());

    glPushMatrix();
    // tank body
    glTranslatef(tank.x, tank.y, tank.z);
    glColor3f(0.8, 0, 0);
    glutSolidCube(0.05);
    // turret
    glPushMatrix();
    glRotatef(RAD_TO_DEG(angle), 0, 0, 1);
    glBegin(GL_LINE);
    glVertex3f(0, 0, 0);
    glVertex3f(0.1, 0, 0);
    glEnd();
    glPopMatrix();
    glPopMatrix();

    glColor3f(1, 1, 1);
    std::ostringstream enemy_tank_pos;
    enemy_tank_pos << "Bad Tank @ (" << enemy_tank.x << ", " << enemy_tank.y << ", " << enemy_tank.z << "); Wind: " << wind;
    draw_string(-1, 0.85, 2, enemy_tank_pos.str());

    glPushMatrix();
    glTranslatef(enemy_tank.x, enemy_tank.y, enemy_tank.z);
    glColor3fv(enemy_color);
    glutSolidCube(0.05);
    glPopMatrix();
}

void
display() {
    //std::cerr << "display()" << std::endl;
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    draw_sky();
    draw_mountains();
    draw_tanks();
    draw_projectile();
    draw_center_message();

    glutSwapBuffers();
}

void
fire() {
    proj = new Vertex();
    proj->x = tank.x;
    proj->y = tank.y;
    proj->z = tank.z;
    proj_path.push_back(*proj);
    proj_vel = new Vertex();
    proj_vel->x = cos(angle) * power * POWER_SCALE;
    proj_vel->y = sin(angle) * power * POWER_SCALE;
    //std::cerr << "fire(): proj->y " << proj->y << "; proj_vel->y: " << proj_vel->y << std::endl;
    proj_vel->z = 0;
    enemy_color[1] = 0;
}

void
draw_projectile() {
    if (! proj) {
        return;
    }
    glColor3f(1, 1, 1);
    std::ostringstream pos_strstr;
    pos_strstr << "Projectile: (" << proj->x << ", " << proj->y << ", " << proj->z << ") @ (" << proj_vel->x << ", " << proj_vel->y << ", " << proj_vel->z << ")";
    draw_string(-1, 0.75, 2, pos_strstr.str());

    // apply acceleration from the wind
    proj_vel->x += wind * WIND_SCALE;
    proj->x += proj_vel->x;
    // gravity changes our y velocity with every step
    proj_vel->y -= GRAV;
    proj->y += proj_vel->y;
    proj_path.push_back(*proj);

    glPushMatrix();
    glTranslatef(proj->x, proj->y, proj->z);
    glutSolidCube(0.01);
    glPopMatrix();

    bool done = false;
    if (proj->y < -1 || proj->x < -1 || proj->x > 1) {
        std::cerr << "Projectile out of bounds, so we're done." << std::endl;
        done = true;
    } else if ((proj->y > (enemy_tank.y - 0.025)) && proj->y < (enemy_tank.y + 0.025)
            && (proj->x > (enemy_tank.x - 0.025)) && (proj->x < (enemy_tank.x + 0.025))) {
        std::cerr << "HIT! Yay!" << std::endl;
        set_center_message("HIT!", 60);
        enemy_color[1] = 0.8;
        done = true;
    } else if (proj->y <= (sin(proj->x*M_PI)/3)) {
        std::cerr << "Projectile hit the dirt!" << std::endl;
        done = true;
    }
    if (done) {
        //std::cerr << "draw_projectile(): proj->y: " << proj->y<< "; proj_vel->y: " << proj_vel->y << std::endl;
        delete proj;
        proj = NULL;
        delete proj_vel;
        proj_vel = NULL;
        proj_path.clear();
    }

    glColor3f(1, 0.2, 0.2);
    glBegin(GL_LINE_STRIP);
    for (std::vector<Vertex>::iterator i = proj_path.begin(); i != proj_path.end(); i++) {
        glVertex3f(i->x, i->y, i->z);
    }
    glEnd();
}

void
keyboard(unsigned char key, int x, int y) {
    switch(key) {
        case 27:
            exit(0);
            break;
        case ' ':
            fire();
            break;
    }
}

void
specialKeyboard(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_RIGHT:
            angle -= ANGLE_STEP;
            break;
        case GLUT_KEY_LEFT:
            angle += ANGLE_STEP;
            break;
        case GLUT_KEY_UP:
            power += POWER_STEP;
            break;
        case GLUT_KEY_DOWN:
            power -= POWER_STEP;
            break;
    }
}

void
init_gl() {
    //std::cerr << "init_gl()" << std::endl;
    glClearColor(0, 0, 0, 1);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_CULL_FACE);
    //glEnable(GL_LIGHTING);
    //glEnable(GL_LIGHT0);
    //float light_pos[4] = {0, 1, 10, 0};
    //glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
}

void
init_mountain() {
    for (int i = 0; i < MOUNTAIN_POINTS; i++) {
        float r = (float)i / (MOUNTAIN_POINTS/2) - 1;
        float a = r * M_PI;
        float s = sin(a);
        mtns[i].x = r;
        mtns[i].y = s / 3;
        mtns[i].z = 0.1;
        //std::cerr << "Peak " << i << ": (" << r << ", " << s << ", 0.0)" << std::endl;
    }
}

void
init_tanks() {
    //std::cerr << "init_tanks()" << std::endl;
    tank.x = mtns[5].x;
    tank.y = mtns[5].y;
    tank.z = mtns[5].z;
    enemy_tank.x = mtns[15].x;
    enemy_tank.y = mtns[15].y;
    enemy_tank.z = mtns[15].z;
    enemy_color[0] = 0.5;
    enemy_color[1] = 0;
    enemy_color[2] = 0.5;
    power = 250;
    angle = DEG_TO_RAD(45);
    wind = -25;
}

void
init_window() {
    //std::cerr << "init_window()" << std::endl;
    window.width = 800;
    window.height= 600;
    window.title = "Tanks!";
    center_message_countdown = 0;
}

void
init_glut(int* argc, char** argv) {
    //std::cerr << "init_glut()" << std::endl;
    glutInit(argc, argv);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_DOUBLE);
    glutInitWindowSize(window.width, window.height);
    window.id = glutCreateWindow(window.title.c_str());
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    init_gl();
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard);
    glutTimerFunc(MILLIS, timer, 0);
}

void
timer(int value) {
    glutTimerFunc(MILLIS, timer, ++value);
    glutPostRedisplay();
}

int
main(int argc, char** argv) {
    init_window();
    init_glut(&argc, argv);
    init_mountain();
    init_tanks();

    glutReportErrors();
    //std::cerr << "glutMainLoop()" << std::endl;
    glutMainLoop();

    return 0;
}

// vim: set fdm=syntax :
