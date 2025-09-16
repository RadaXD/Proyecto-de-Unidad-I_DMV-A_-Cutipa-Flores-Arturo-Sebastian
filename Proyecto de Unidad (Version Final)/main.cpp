#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <stack>
#include <string>
#include <iostream>
#include <fstream>
using namespace std;

const int WINW = 800;
const int WINH = 600;

// Herramientas disponibles
enum Tool {
    LINE_DIRECT,
    LINE_DDA,
    CIRCLE_PM,
    ELLIPSE_PM,
    NONE
};

// Estructura de color
struct Color {
    float r, g, b;
};

// Estructura para una figura
struct Shape {
    Tool type;
    int x1, y1, x2, y2;   // para lineas
    int xc, yc, r;        // para circulo
    int rx, ry;           // para elipse
    Color color;
    int thickness;
};

vector<Shape> shapes;
stack<vector<Shape>> undo_stack;
stack<vector<Shape>> redo_stack;

// Estado actual
Tool currentTool = LINE_DIRECT;
Color currentColor = {0, 0, 0};
int currentThickness = 1;
bool showGrid = true;
bool showAxes = true;
bool waitingSecondPoint = false;
int firstX = 0;
int firstY = 0;
int viewportW = WINW;
int viewportH = WINH;

inline int roundi(float v) {
    return (int) floor(v + 0.5f);
}

// Manejo de pilas undo/redo
void pushUndo() {
    undo_stack.push(shapes);
    while (!redo_stack.empty()) redo_stack.pop();
}

void doUndo() {
    if (!undo_stack.empty()) {
        redo_stack.push(shapes);
        shapes = undo_stack.top();
        undo_stack.pop();
        glutPostRedisplay();
    }
}

void doRedo() {
    if (!redo_stack.empty()) {
        undo_stack.push(shapes);
        shapes = redo_stack.top();
        redo_stack.pop();
        glutPostRedisplay();
    }
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

// Circulo Punto Medio
void circ8(int xc, int yc, int x, int y) {
    putPixel(xc + x, yc + y);
    putPixel(xc - x, yc + y);
    putPixel(xc + x, yc - y);
    putPixel(xc - x, yc - y);
    putPixel(xc + y, yc + x);
    putPixel(xc - y, yc + x);
    putPixel(xc + y, yc - x);
    putPixel(xc - y, yc - x);
}

void circlePM(int xc, int yc, int r, int t) {
    int x = 0;
    int y = r;
    int p = 1 - r;

    glPointSize(t);
    glBegin(GL_POINTS);

    circ8(xc, yc, x, y);

    while (x < y) {
        x++;
        if (p < 0) {
            p += 2 * x + 1;
        }
        else {
            y--;
            p += 2 * (x - y) + 1;
        }
        circ8(xc, yc, x, y);
    }

    glEnd();
}

// Elipse Punto Medio
void ellipse4(int xc, int yc, int x, int y) {
    putPixel(xc + x, yc + y);
    putPixel(xc - x, yc + y);
    putPixel(xc + x, yc - y);
    putPixel(xc - x, yc - y);
}

void ellipsePM(int xc, int yc, int rx, int ry, int t) {
    int x = 0;
    int y = ry;

    long rx2 = rx * rx;
    long ry2 = ry * ry;
    long two_rx2 = 2 * rx2;
    long two_ry2 = 2 * ry2;

    double p1 = ry2 - rx2 * ry + 0.25 * rx2;

    glPointSize(t);
    glBegin(GL_POINTS);

    while ((two_ry2 * x) <= (two_rx2 * y)) {
        ellipse4(xc, yc, x, y);
        if (p1 < 0) {
            x++;
            p1 += two_ry2 * x + ry2;
        }
        else {
            x++;
            y--;
            p1 += two_ry2 * x - two_rx2 * y + ry2;
        }
    }

    double p2 = ry2 * (x + 0.5) * (x + 0.5)
               + rx2 * (y - 1) * (y - 1)
               - rx2 * ry2;

    while (y >= 0) {
        ellipse4(xc, yc, x, y);
        if (p2 > 0) {
            y--;
            p2 -= two_rx2 * y + rx2;
        }
        else {
            y--;
            x++;
            p2 += two_ry2 * x - two_rx2 * y + rx2;
        }
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
    else if (s.type == CIRCLE_PM) {
        circlePM(s.xc, s.yc, s.r, s.thickness);
    }
    else if (s.type == ELLIPSE_PM) {
        ellipsePM(s.xc, s.yc, s.rx, s.ry, s.thickness);
    }
}

void redrawAll() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Dibujar grid
    if (showGrid) {
        glColor3f(0.85, 0.85, 0.85);
        glBegin(GL_LINES);
        for (int x = 0; x <= viewportW; x += 20) {
            glVertex2i(x, 0);
            glVertex2i(x, viewportH);
        }
        for (int y = 0; y <= viewportH; y += 20) {
            glVertex2i(0, y);
            glVertex2i(viewportW, y);
        }
        glEnd();
    }

    // Dibujar ejes
    if (showAxes) {
        glColor3f(0.6, 0.6, 0.6);
        glBegin(GL_LINES);
        glVertex2i(0, viewportH / 2);
        glVertex2i(viewportW, viewportH / 2);
        glVertex2i(viewportW / 2, 0);
        glVertex2i(viewportW / 2, viewportH);
        glEnd();
    }

    // Dibujar figuras guardadas
    for (auto &s : shapes) {
        drawShape(s);
    }

    glutSwapBuffers();
}

// Guardar imagen en formato PPM
void exportPPM(const string &filename) {
    int w = viewportW;
    int h = viewportH;

    vector<unsigned char> pixels(3 * w * h);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    ofstream ofs(filename, ios::binary);
    ofs << "P6\n" << w << " " << h << "\n255\n";
    for (int y = h - 1; y >= 0; y--) {
        ofs.write((char*)&pixels[y * w * 3], 3 * w);
    }
    ofs.close();

    cout << "Exportado " << filename << endl;
}

// ---------------- Acciones ----------------
void display() {
    redrawAll();
}

void reshape(int w, int h) {
    viewportW = w;
    viewportH = h;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void mouse(int b, int s, int x, int y) {
    int ox = x;
    int oy = viewportH - y;

    if (b == GLUT_LEFT_BUTTON && s == GLUT_DOWN) {
        if (!waitingSecondPoint) {
            firstX = ox;
            firstY = oy;
            waitingSecondPoint = true;
        }
        else {
            pushUndo();

            Shape sh;
            sh.color = currentColor;
            sh.thickness = currentThickness;

            if (currentTool == LINE_DIRECT || currentTool == LINE_DDA) {
                sh.type = currentTool;
                sh.x1 = firstX;
                sh.y1 = firstY;
                sh.x2 = ox;
                sh.y2 = oy;
            }
            else if (currentTool == CIRCLE_PM) {
                sh.type = CIRCLE_PM;
                sh.xc = firstX;
                sh.yc = firstY;
                int dx = ox - firstX;
                int dy = oy - firstY;
                sh.r = round(sqrt(dx * dx + dy * dy));
            }
            else if (currentTool == ELLIPSE_PM) {
                sh.type = ELLIPSE_PM;
                sh.xc = firstX;
                sh.yc = firstY;
                sh.rx = abs(ox - firstX);
                sh.ry = abs(oy - firstY);
            }

            shapes.push_back(sh);
            waitingSecondPoint = false;
            glutPostRedisplay();
        }
    }
}

void keyboard(unsigned char k, int, int) {
    if (k == 'g' || k == 'G') { showGrid = !showGrid; }
    if (k == 'e' || k == 'E') { showAxes = !showAxes; }
    if (k == 'c' || k == 'C') { pushUndo(); shapes.clear(); }
    if (k == 'z' || k == 'Z') doUndo();
    if (k == 'y' || k == 'Y') doRedo();
    if (k == 'p' || k == 'P' || k == 's' || k == 'S') exportPPM("canvas.ppm");
    if (k == 27) exit(0);

    glutPostRedisplay();
}

// Menu contextual
void menuSelect(int op) {
    switch (op) {
        case 1: currentTool = LINE_DIRECT; break;
        case 2: currentTool = LINE_DDA; break;
        case 3: currentTool = CIRCLE_PM; break;
        case 4: currentTool = ELLIPSE_PM; break;

        case 10: currentColor = {0,0,0}; break;
        case 11: currentColor = {1,0,0}; break;
        case 12: currentColor = {0,1,0}; break;
        case 13: currentColor = {0,0,1}; break;

        case 20: currentThickness = 1; break;
        case 21: currentThickness = 2; break;
        case 22: currentThickness = 3; break;
        case 23: currentThickness = 5; break;

        case 30: showGrid = !showGrid; break;
        case 31: showAxes = !showAxes; break;

        case 40: pushUndo(); shapes.clear(); break;
        case 41: doUndo(); break;
        case 42: doRedo(); break;
        case 43: exportPPM("canvas.ppm"); break;
    }

    glutPostRedisplay();
}

void createMenus() {
    int draw = glutCreateMenu(menuSelect);
    glutAddMenuEntry("Linea Directa", 1);
    glutAddMenuEntry("Linea DDA", 2);
    glutAddMenuEntry("Circulo PM", 3);
    glutAddMenuEntry("Elipse PM", 4);

    int color = glutCreateMenu(menuSelect);
    glutAddMenuEntry("Negro", 10);
    glutAddMenuEntry("Rojo", 11);
    glutAddMenuEntry("Verde", 12);
    glutAddMenuEntry("Azul", 13);

    int thick = glutCreateMenu(menuSelect);
    glutAddMenuEntry("1px", 20);
    glutAddMenuEntry("2px", 21);
    glutAddMenuEntry("3px", 22);
    glutAddMenuEntry("5px", 23);

    int view = glutCreateMenu(menuSelect);
    glutAddMenuEntry("Toggle Grid", 30);
    glutAddMenuEntry("Toggle Axes", 31);

    int tools = glutCreateMenu(menuSelect);
    glutAddMenuEntry("Clear", 40);
    glutAddMenuEntry("Undo", 41);
    glutAddMenuEntry("Redo", 42);
    glutAddMenuEntry("Export PPM", 43);

    int mainM = glutCreateMenu(menuSelect);
    glutAddSubMenu("Dibujo", draw);
    glutAddSubMenu("Color", color);
    glutAddSubMenu("Grosor", thick);
    glutAddSubMenu("Vista", view);
    glutAddSubMenu("Herramientas", tools);

    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

// ---------------- Ventana ----------------
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
    glutCreateWindow("Mini CAD Raster");

    initGL();
    createMenus();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}
