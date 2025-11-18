#include <GL/glut.h>
#include <iostream>
#include <string>
#include <cmath>
using namespace std;

enum GameState { MENU, GAME };
GameState currentState = MENU;


float playerX = 0.0f;
float playerZ = 0.0f;
float speed = 0.1f;


void drawText(float x, float y, string text) {
    glRasterPos2f(x, y);
    for (char c : text) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
}


void drawPlayer() {
    glPushMatrix();
    glTranslatef(playerX, 0.0f, playerZ);

    // Legs
    glColor3f(0.2f, 0.2f, 0.8f); // Blue 
    // Left leg
    glPushMatrix();
    glTranslatef(-0.15f, 0.4f, 0.0f);
    glScalef(0.15f, 0.8f, 0.15f);
    glutSolidCube(1.0);
    glPopMatrix();
    
    // Right leg
    glPushMatrix();
    glTranslatef(0.15f, 0.4f, 0.0f);
    glScalef(0.15f, 0.8f, 0.15f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Torso
    glColor3f(0.8f, 0.1f, 0.1f); // Red 
    glPushMatrix();
    glTranslatef(0.0f, 1.1f, 0.0f);
    glScalef(0.5f, 0.7f, 0.25f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Arms
    glColor3f(1.0f, 0.8f, 0.6f); // Skin 
    // Left arm
    glPushMatrix();
    glTranslatef(-0.35f, 1.1f, 0.0f);
    glScalef(0.12f, 0.6f, 0.12f);
    glutSolidCube(1.0);
    glPopMatrix();
    
    // Right arm
    glPushMatrix();
    glTranslatef(0.35f, 1.1f, 0.0f);
    glScalef(0.12f, 0.6f, 0.12f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Neck
    glColor3f(1.0f, 0.8f, 0.6f);
    glPushMatrix();
    glTranslatef(0.0f, 1.55f, 0.0f);
    glScalef(0.1f, 0.15f, 0.1f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Head
    glColor3f(1.0f, 0.8f, 0.6f);
    glPushMatrix();
    glTranslatef(0.0f, 1.8f, 0.0f);
    glutSolidSphere(0.25, 20, 20);
    glPopMatrix();

    // Eyes
    glColor3f(0.0f, 0.0f, 0.0f);
    // Left eye
    glPushMatrix();
    glTranslatef(-0.08f, 1.85f, 0.22f);
    glutSolidSphere(0.03, 10, 10);
    glPopMatrix();
    
    // Right eye
    glPushMatrix();
    glTranslatef(0.08f, 1.85f, 0.22f);
    glutSolidSphere(0.03, 10, 10);
    glPopMatrix();

    glPopMatrix();
}


void drawMenu() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(1, 1, 1);
    drawText(350, 400, "ESCAPE THE LAB");
    drawText(360, 300, "1 - START GAME");
    drawText(360, 250, "2 - EXIT");
}




void drawGame() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, 800/600.0, 0.1, 100);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();


    gluLookAt(playerX, 3, playerZ - 5,  playerX, 1, playerZ,  0, 1, 0);

    
    glColor3f(0.3f, 0.3f, 0.3f);
    // Floor
    glBegin(GL_QUADS);
    glVertex3f(-5, 0, -5);
    glVertex3f( 5, 0, -5);
    glVertex3f( 5, 0,  5);
    glVertex3f(-5, 0,  5);
    glEnd();

    // Ceiling
    glColor3f(0.4f, 0.4f, 0.4f);
    glBegin(GL_QUADS);
    glVertex3f(-5, 4, -5);
    glVertex3f( 5, 4, -5);
    glVertex3f( 5, 4,  5);
    glVertex3f(-5, 4,  5);
    glEnd();

    // Back wall
    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_QUADS);
    glVertex3f(-5, 0, -5);
    glVertex3f( 5, 0, -5);
    glVertex3f( 5, 4, -5);
    glVertex3f(-5, 4, -5);
    glEnd();

    // Front wall with door opening
    // Left section
    glBegin(GL_QUADS);
    glVertex3f(-5, 0, 5);
    glVertex3f(-1, 0, 5);
    glVertex3f(-1, 4, 5);
    glVertex3f(-5, 4, 5);
    glEnd();
    
    // Right section
    glBegin(GL_QUADS);
    glVertex3f(1, 0, 5);
    glVertex3f(5, 0, 5);
    glVertex3f(5, 4, 5);
    glVertex3f(1, 4, 5);
    glEnd();
    
    // Top section above door
    glBegin(GL_QUADS);
    glVertex3f(-1, 2, 5);
    glVertex3f(1, 2, 5);
    glVertex3f(1, 4, 5);
    glVertex3f(-1, 4, 5);
    glEnd();

    //door
    glColor3f(0.6f, 0.3f, 0.0f);
    glBegin(GL_QUADS);
    glVertex3f(-1, 2, 5);
    glVertex3f(1, 2, 5);
    glVertex3f(1, 0, 5);
    glVertex3f(-1, 0, 5);
    glEnd();
    

    // Left wall
    glColor3f(0.45f, 0.45f, 0.45f);
    glBegin(GL_QUADS);
    glVertex3f(-5, 0, -5);
    glVertex3f(-5, 0,  5);
    glVertex3f(-5, 4,  5);
    glVertex3f(-5, 4, -5);
    glEnd();

    // Right wall
    glBegin(GL_QUADS);
    glVertex3f(5, 0, -5);
    glVertex3f(5, 0,  5);
    glVertex3f(5, 4,  5);
    glVertex3f(5, 4, -5);
    glEnd();

    drawPlayer();
}


void opendoor(){
    
}
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (currentState == MENU) drawMenu();
    else if (currentState == GAME) drawGame();

    glutSwapBuffers();
}


void keyboard(unsigned char key, int x, int y) {
    if (currentState == MENU) {
        if (key == '1') {
            currentState = GAME;
            playerX = 0.0f;
            playerZ = 0.0f;
        }
        if (key == '2') exit(0);
    }

    if (currentState == GAME) {
        if (key == 'w') playerZ += speed;
        if (key == 's') playerZ -= speed;
        if (key == 'a') playerX += speed;
        if (key == 'd') playerX -= speed;
        if (key == 'm') currentState = MENU;
    }
}


int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Escape The Lab");

    glEnable(GL_DEPTH_TEST);

    glutDisplayFunc(display);
    glutIdleFunc(display);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}
