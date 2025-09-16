#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <iostream>
using namespace std;

const int WINW = 800;
const int WINH = 600;

// Estructura de color
struct Color {
    float r, g, b;
};

// Estructura para una linea
struct Shape {
    int x1, y1, x2, y2;
    Color color;
    int thickness;
};

vector<Shape> shapes;   // lista de lineas dibujadas

// Estado actual
Color currentColor = {0, 0, 0};  // negro
int currentThickness = 1;
bool waitingSecondPoint = false;
int firstX = 0;
int firstY = 0;

inline int roundi(float v) {
    return (int) floor(v + 0.5f);
}

// Dibuja un pixel
void putPixel(int x, int y) {
    glVertex2i(x, y);
}

// Algoritmo de linea directa
void lineDirect(int x0, int y0, int x1, int y1, int t) {
    int dx = x1 - x0;
    int dy = y1 - y0;

    glPointSize(t);
    glBegin(GL_POINTS);

    if (dx == 0) {
        int sy = (y1 > y0) ? 1 : -1;
        for (int y = y0; y != y1 + sy; y += sy) {
            putPixel(x0, y);
        }
    }
    else if (dy == 0) {
        int sx = (x1 > x0) ? 1 : -1;
        for (int x = x0; x != x1 + sx; x += sx) {
            putPixel(x, y0);
        }
    }
    else {
        float m = (float) dy / dx;
        if (fabs(m) <= 1) {
            int sx = (x1 > x0) ? 1 : -1;
            for (int x = x0; x != x1 + sx; x += sx) {
                putPixel(x, roundi(m * (x - x0) + y0));
            }
        }
        else {
            int sy = (y1 > y0) ? 1 : -1;
            float invm = (float) dx / dy;
            for (int y = y0; y != y1 + sy; y += sy) {
                putPixel(roundi(invm * (y - y0) + x0), y);
            }
        }
    }

    glEnd();
}

// Dibuja todas las lineas guardadas
void redrawAll() {
    glClear(GL_COLOR_BUFFER_BIT);

    for (auto &s : shapes) {
        glColor3f(s.color.r, s.color.g, s.color.b);
        lineDirect(s.x1, s.y1, s.x2, s.y2, s.thickness);
    }

    glutSwapBuffers();
}

// ---------------- Callbacks ----------------
void display() {
    redrawAll();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void mouse(int b, int s, int x, int y) {
    int ox = x;
    int oy = WINH - y;

    if (b == GLUT_LEFT_BUTTON && s == GLUT_DOWN) {
        if (!waitingSecondPoint) {
            // Primer clic: guardar coordenadas
            firstX = ox;
            firstY = oy;
            waitingSecondPoint = true;
        }
        else {
            // Segundo clic: crear linea y guardarla
            Shape sh;
            sh.color = currentColor;
            sh.thickness = currentThickness;
            sh.x1 = firstX;
            sh.y1 = firstY;
            sh.x2 = ox;
            sh.y2 = oy;

            shapes.push_back(sh);
            waitingSecondPoint = false;

            glutPostRedisplay();
        }
    }
}

// ---------------- Main ----------------
void initGL() {
    glClearColor(1, 1, 1, 1);  // fondo blanco
    glPointSize(1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINW, 0, WINH);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINW, WINH);
    glutCreateWindow("Version 1 - Linea Directa");

    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);

    glutMainLoop();
    return 0;
}
