#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <iostream>
using namespace std;

const int WINW = 800;
const int WINH = 600;

// Herramientas disponibles
enum Tool {
    LINE_DIRECT,
    LINE_DDA,
    NONE
};

// Estructura de color
struct Color {
    float r, g, b;
};

// Estructura para una linea
struct Shape {
    Tool type;
    int x1, y1, x2, y2;
    Color color;
    int thickness;
};

vector<Shape> shapes;   // lista de lineas dibujadas

// Estado actual
Tool currentTool = LINE_DIRECT;
Color currentColor = {0, 0, 0};   // negro
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

// ---------------- Algoritmos ----------------

// Linea directa
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

// Linea DDA
void lineDDA(int x0, int y0, int x1, int y1, int t) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int steps = max(abs(dx), abs(dy));

    float x = x0;
    float y = y0;
    float incx = dx / (float) steps;
    float incy = dy / (float) steps;

    glPointSize(t);
    glBegin(GL_POINTS);

    for (int i = 0; i <= steps; i++) {
        putPixel(roundi(x), roundi(y));
        x += incx;
        y += incy;
    }

    glEnd();
}

// ---------------- Dibujo ----------------
void drawShape(const Shape &s) {
    glColor3f(s.color.r, s.color.g, s.color.b);

    if (s.type == LINE_DIRECT) {
        lineDirect(s.x1, s.y1, s.x2, s.y2, s.thickness);
    }
    else if (s.type == LINE_DDA) {
        lineDDA(s.x1, s.y1, s.x2, s.y2, s.thickness);
    }
}

void redrawAll() {
    glClear(GL_COLOR_BUFFER_BIT);

    for (auto &s : shapes) {
        drawShape(s);
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
            // Primer clic
            firstX = ox;
            firstY = oy;
            waitingSecondPoint = true;
        }
        else {
            // Segundo clic: crear linea
            Shape sh;
            sh.type = currentTool;
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

// ---------------- Menu ----------------
void menuSelect(int option) {
    switch (option) {
        case 1: currentTool = LINE_DIRECT; break;
        case 2: currentTool = LINE_DDA; break;
        case 10: currentColor = {0, 0, 0}; break;   // negro
        case 11: currentColor = {1, 0, 0}; break;   // rojo
        case 12: currentColor = {0, 0, 1}; break;   // azul
    }
    glutPostRedisplay();
}

void createMenus() {
    int drawMenu = glutCreateMenu(menuSelect);
    glutAddMenuEntry("Linea Directa", 1);
    glutAddMenuEntry("Linea DDA", 2);

    int colorMenu = glutCreateMenu(menuSelect);
    glutAddMenuEntry("Negro", 10);
    glutAddMenuEntry("Rojo", 11);
    glutAddMenuEntry("Azul", 12);

    int mainMenu = glutCreateMenu(menuSelect);
    glutAddSubMenu("Algoritmo", drawMenu);
    glutAddSubMenu("Color", colorMenu);

    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

// ---------------- Main ----------------
void initGL() {
    glClearColor(1,1,1,1);
    glPointSize(1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINW, 0, WINH);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINW, WINH);
    glutCreateWindow("Version 2 - Directa y DDA");

    initGL();
    createMenus();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);

    glutMainLoop();
    return 0;
}
