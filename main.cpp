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
float playerRadius = 0.35f; 

// Camera rotation
float cameraYaw = 0.0f;
float cameraPitch = 0.0f;
int lastMouseX = 400;
int lastMouseY = 300;
bool mouseLocked = false;
bool mouseDown = false;

// Animation
float animationTime = 0.0f;
bool isMoving = false;
float playerLookAngle = 0.0f; 


void drawText(float x, float y, string text) {
    glRasterPos2f(x, y);
    for (char c : text) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
}


bool checkCollision(float newX, float newZ) {
    // Room bounds: -5 to 5 in X, -5 to 5 in Z (except for door opening)
    // Door opening: X from -1 to 1, Z at 5
    
    float minX = -5.0f + playerRadius;
    float maxX = 5.0f - playerRadius;
    float minZ = -5.0f + playerRadius;
    float maxZ = 5.0f - playerRadius;
    
    // Check collision with walls
    if (newX < minX || newX > maxX || newZ < minZ || newZ > maxZ) {
        if (newZ > maxZ && newX >= -1.0f && newX <= 1.0f) {
            return false; 
        }
        return true;
    }
    
    return false; 
}


void drawPlayer() {
    glPushMatrix();
    glTranslatef(playerX, 0.0f, playerZ);
    glRotatef(playerLookAngle, 0.0f, 1.0f, 0.0f); 

    // Legs
    glColor3f(0.2f, 0.2f, 0.8f); // Blue 
    // Left leg
    glPushMatrix();
    glTranslatef(-0.15f, 0.4f, 0.0f);
    if (isMoving) {
        float legRotation = sin(animationTime * 15.0f) * 15.0f; 
        glRotatef(legRotation, 1.0f, 0.0f, 0.0f);
    }
    glScalef(0.15f, 0.8f, 0.15f);
    glutSolidCube(1.0);
    glPopMatrix();
    
    // Right leg
    glPushMatrix();
    glTranslatef(0.15f, 0.4f, 0.0f);
    if (isMoving) {
        float legRotation = sin(animationTime * 15.0f + 3.14159f) * 15.0f; 
        glRotatef(legRotation, 1.0f, 0.0f, 0.0f);
    }
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
    if (isMoving) {
        float armRotation = sin(animationTime * 15.0f + 3.14159f) * 20.0f; 
        glRotatef(armRotation, 1.0f, 0.0f, 0.0f);
    }
    glScalef(0.12f, 0.6f, 0.12f);
    glutSolidCube(1.0);
    glPopMatrix();
    
    // Right arm
    glPushMatrix();
    glTranslatef(0.35f, 1.1f, 0.0f);
    if (isMoving) {
        float armRotation = sin(animationTime * 15.0f) * 20.0f; 
        glRotatef(armRotation, 1.0f, 0.0f, 0.0f);
    }
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

    float camX = playerX + 5.0f * sin(cameraYaw * 3.14159f / 180.0f);
    float camY = 3.0f + 5.0f * sin(cameraPitch * 3.14159f / 180.0f);
    float camZ = playerZ + 5.0f * cos(cameraYaw * 3.14159f / 180.0f);
    
    if (camX < -5.0f) camX = -5.0f;
    if (camX > 5.0f) camX = 5.0f;
    if (camZ < -5.0f) camZ = -5.0f;
    if (camZ > 5.0f) camZ = 5.0f;
    if (camY < 0.5f) camY = 0.5f;
    if (camY > 3.8f) camY = 3.8f;

    gluLookAt(camX, camY, camZ,  playerX, 1, playerZ,  0, 1, 0);

    
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
            cameraYaw = 0.0f;
            cameraPitch = 0.0f;
            mouseLocked = true;
            glutSetCursor(GLUT_CURSOR_NONE);
            glutWarpPointer(400, 300);
        }
        if (key == '2') exit(0);
    }

    if (currentState == GAME) {
        float newX = playerX;
        float newZ = playerZ;
        bool moved = false;
        
        // Calculate movement direction based on camera yaw
        float moveX = sin(cameraYaw * 3.14159f / 180.0f);
        float moveZ = cos(cameraYaw * 3.14159f / 180.0f);
        float strafeX = cos(cameraYaw * 3.14159f / 180.0f);
        float strafeZ = -sin(cameraYaw * 3.14159f / 180.0f);
        
        if (key == 's') {
            newX = playerX + moveX * speed;
            newZ = playerZ + moveZ * speed;
            playerLookAngle = cameraYaw;
            moved = true;
        }
        if (key == 'w') {
            newX = playerX - moveX * speed;
            newZ = playerZ - moveZ * speed;
            playerLookAngle = cameraYaw + 180.0f;
            moved = true;
        }
        if (key == 'a') {
            newX = playerX - strafeX * speed;
            newZ = playerZ - strafeZ * speed;
            playerLookAngle = cameraYaw - 90.0f;
            moved = true;
        }
        if (key == 'd') {
            newX = playerX + strafeX * speed;
            newZ = playerZ + strafeZ * speed;
            playerLookAngle = cameraYaw + 90.0f;
            moved = true;
        }
        
        if (moved) {
            if (!checkCollision(newX, newZ)) {
                playerX = newX;
                playerZ = newZ;
                isMoving = true;
                animationTime += 0.016f; 
            }
        } else {
            isMoving = false;
            animationTime = 0.0f;
        }
        
        if (key == 'm') {
            currentState = MENU;
            mouseLocked = false;
            glutSetCursor(GLUT_CURSOR_INHERIT);
        }
        if (key == 27) exit(0);
    }
}


void mouseMotion(int x, int y) {
    if (currentState == GAME && mouseLocked && mouseDown) {
        int deltaX = x - lastMouseX;
        int deltaY = y - lastMouseY;
        

        cameraYaw += deltaX * 0.25f;
        cameraPitch -= deltaY * 0.25f;
        

        if (cameraPitch > 89.0f) cameraPitch = 89.0f;
        if (cameraPitch < -89.0f) cameraPitch = -89.0f;
        
        if (cameraYaw > 360.0f) cameraYaw -= 360.0f;
        if (cameraYaw < 0.0f) cameraYaw += 360.0f;
        
        lastMouseX = x;
        lastMouseY = y;
    }
}


void mouse(int button, int state, int x, int y) {
    if (currentState == GAME && mouseLocked) {
        if (button == GLUT_LEFT_BUTTON) {
            if (state == GLUT_DOWN) {
                mouseDown = true;
                lastMouseX = x;
                lastMouseY = y;
            } else if (state == GLUT_UP) {
                mouseDown = false;
            }
        }
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
    glutMotionFunc(mouseMotion);
    glutMouseFunc(mouse);

    glutMainLoop();
    return 0;
}
