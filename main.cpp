#include <GL/glut.h>
#include <iostream>
#include <string>
#include <cmath>
using namespace std;

enum GameState { MENU, INTRO, LASER_CORRIDOR };
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

// Laser Corridor variables
float laserTime = 0.0f;
bool laserCorridorLoaded = false;
bool levelStarted = false;
float instructionDisplayTime = 0.0f;
float levelStartTime = 0.0f;
const float LEVEL_TIME_LIMIT = 90.0f; 


void drawText(float x, float y, string text) {
    glRasterPos2f(x, y);
    for (char c : text) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
}

bool isMouseOverButton(int mouseX, int mouseY, float buttonX, float buttonY, float buttonW, float buttonH) {
    return (mouseX >= buttonX && mouseX <= buttonX + buttonW &&
            mouseY >= buttonY && mouseY <= buttonY + buttonH);
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

bool checkCollisionLaserCorridor(float newX, float newZ) {
    // Laser Corridor: -4 to 4 in X, 0 to 20 in Z
    float minX = -4.0f + playerRadius;
    float maxX = 4.0f - playerRadius;
    float minZ = 0.0f + playerRadius;
    float maxZ = 20.0f - playerRadius;
    
    // Exit door opening at Z=20, X from -1 to 1
    if (newZ > maxZ) {
        if (newX >= -1.0f && newX <= 1.0f) {
            return false; // Can exit through door
        }
        return true; // Blocked by walls
    }
    
    if (newX < minX || newX > maxX || newZ < minZ) {
        return true;
    }
    return false;
}

void resetLaserLevel() {
    playerX = 0.0f;
    playerZ = 0.5f;
}

bool checkLaserCollision(float x, float y, float z) {
    // Check if player touches any laser
    laserTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    
    // Horizontal sweeping lasers (moving side to side) - RED
    // 3 red lasers distributed at Z=4, Z=10, Z=16
    for (int i = 0; i < 3; i++) {
        float redLaserZ = 4.0f + (i * 6.0f);
        float redLaserX = sin(laserTime * 0.5f) * 3.5f;
        
        // Very tight collision box matching laser thickness (1.5pt core + glow margin)
        if (fabs(z - redLaserZ) < 0.3f && fabs(x - redLaserX) < 0.25f && y < 3.8f && y > 0.2f) {
            return true; // Hit red laser
        }
    }
    
    // Horizontal pulsing lasers (fixed positions, on/off) - GREEN
    // 3 green lasers distributed at Z=7, Z=13, Z=19
    for (int i = 0; i < 3; i++) {
        float greenLaserZ = 7.0f + (i * 6.0f);
        float cycle = fmod(laserTime, 2.0f);
        if (cycle < 1.0f) { // Laser on for first second
            // Wide horizontal beam collision (spans full width -3.5 to 3.5)
            if (fabs(z - greenLaserZ) < 0.3f && y < 2.5f && y > 0.5f && fabs(x) < 3.7f) {
                return true; // Hit green laser
            }
        }
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
    glDisable(GL_FOG);
    glDisable(GL_LIGHTING);
    
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
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(10, 580, "Level: 0");
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawRoom1() {
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
    if (camZ < 0.0f) camZ = 0.0f;
    if (camZ > 10.0f) camZ = 10.0f;
    if (camY < 0.5f) camY = 0.5f;
    if (camY > 3.8f) camY = 3.8f;

    gluLookAt(camX, camY, camZ, playerX, 1, playerZ, 0, 1, 0);

    // --- FOG MECHANIC ---
    float generatorZ = 0.0f;
    float maxDistance = 8.0f;
    float distanceToGenerator = fmax(0.0f, playerZ - generatorZ);
    float normalizedDistance = fmin(1.0f, distanceToGenerator / maxDistance);
    float minDensity = 0.05f;
    float maxDensity = 0.5f;
    float currentFogDensity = maxDensity - (maxDensity - minDensity) * normalizedDistance;

    GLfloat fogColor[] = {0.1f, 0.1f, 0.2f, 1.0f};
    glEnable(GL_FOG);
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, currentFogDensity);

    // Floor
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex3f(-5, 0, 0);
    glVertex3f( 5, 0, 0);
    glVertex3f( 5, 0, 10);
    glVertex3f(-5, 0, 10);
    glEnd();

    // Ceiling
    glColor3f(0.4f, 0.4f, 0.4f);
    glBegin(GL_QUADS);
    glVertex3f(-5, 4, 0);
    glVertex3f( 5, 4, 0);
    glVertex3f( 5, 4, 10);
    glVertex3f(-5, 4, 10);
    glEnd();

    // Back Wall (Entrance)
    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_QUADS);
    glVertex3f(-5, 0, 0);
    glVertex3f( 5, 0, 0);
    glVertex3f( 5, 4, 0);
    glVertex3f(-5, 4, 0);
    glEnd();

    // Front Wall (Exit)
    glBegin(GL_QUADS);
    glVertex3f(-5, 0, 10);
    glVertex3f( 5, 0, 10);
    glVertex3f( 5, 4, 10);
    glVertex3f(-5, 4, 10);
    glEnd();

    // Left Wall
    glColor3f(0.45f, 0.45f, 0.45f);
    glBegin(GL_QUADS);
    glVertex3f(-5, 0, 0);
    glVertex3f(-5, 0, 10);
    glVertex3f(-5, 4, 10);
    glVertex3f(-5, 4, 0);
    glEnd();

    // Right Wall
    glBegin(GL_QUADS);
    glVertex3f( 5, 0, 0);
    glVertex3f( 5, 0, 10);
    glVertex3f( 5, 4, 10);
    glVertex3f( 5, 4, 0);
    glEnd();

    // Generator Light (clue)
    glPushMatrix();
    glTranslatef(4.0f, 1.0f, 1.0f);
    float lightIntensity = 1.0f - normalizedDistance;
    float pulse = (sin(glutGet(GLUT_ELAPSED_TIME) / 500.0f) * 0.2f) + 0.8f;
    glColor3f(lightIntensity * pulse, 0.0f, 0.0f);
    glutSolidSphere(0.5, 20, 20);
    glPopMatrix();

    drawPlayer();
    glDisable(GL_FOG);
    
    // Draw HUD text (level display)
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(10, 580, "Level: 1 - Fog Room");
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}


void drawLaserCorridor() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, 800/600.0, 0.1, 100);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float camX = playerX + 3.0f * sin(cameraYaw * 3.14159f / 180.0f);
    float camY = 3.0f + 3.0f * sin(cameraPitch * 3.14159f / 180.0f);
    float camZ = playerZ + 3.0f * cos(cameraYaw * 3.14159f / 180.0f);
    
    if (camX < -4.0f) camX = -4.0f;
    if (camX > 4.0f) camX = 4.0f;
    if (camZ < 0.0f) camZ = 0.0f;
    if (camZ > 20.0f) camZ = 20.0f;
    if (camY < 0.5f) camY = 0.5f;
    if (camY > 3.8f) camY = 3.8f;

    gluLookAt(camX, camY, camZ, playerX, 1, playerZ, 0, 1, 0);

    // Metallic Lab walls
    glColor3f(0.3f, 0.3f, 0.35f);
    
    // Floor with grid pattern
    glColor3f(0.2f, 0.2f, 0.25f);
    glBegin(GL_QUADS);
    glVertex3f(-4, 0, 0);
    glVertex3f( 4, 0, 0);
    glVertex3f( 4, 0, 20);
    glVertex3f(-4, 0, 20);
    glEnd();


    glColor3f(0.0f, 0.5f, 1.0f);
    for (int i = 0; i <= 20; i += 2) {
        glBegin(GL_LINES);
        glVertex3f(-4, 0.01f, i);
        glVertex3f( 4, 0.01f, i);
        glEnd();
    }
    for (float x = -4; x <= 4; x += 2) {
        glBegin(GL_LINES);
        glVertex3f(x, 0.01f, 0);
        glVertex3f(x, 0.01f, 20);
        glEnd();
    }

    // Ceiling
    glColor3f(0.4f, 0.4f, 0.45f);
    glBegin(GL_QUADS);
    glVertex3f(-4, 4, 0);
    glVertex3f( 4, 4, 0);
    glVertex3f( 4, 4, 20);
    glVertex3f(-4, 4, 20);
    glEnd();

    // Back wall (entrance)
    glColor3f(0.35f, 0.35f, 0.4f);
    glBegin(GL_QUADS);
    glVertex3f(-4, 0, 0);
    glVertex3f( 4, 0, 0);
    glVertex3f( 4, 4, 0);
    glVertex3f(-4, 4, 0);
    glEnd();

    // Front wall (exit)
    // Left section
    glColor3f(0.35f, 0.35f, 0.4f);
    glBegin(GL_QUADS);
    glVertex3f(-4, 0, 20);
    glVertex3f(-1, 0, 20);
    glVertex3f(-1, 4, 20);
    glVertex3f(-4, 4, 20);
    glEnd();
    
    // Right section
    glBegin(GL_QUADS);
    glVertex3f(1, 0, 20);
    glVertex3f(4, 0, 20);
    glVertex3f(4, 4, 20);
    glVertex3f(1, 4, 20);
    glEnd();
    
    // Top section above door
    glBegin(GL_QUADS);
    glVertex3f(-1, 2.5f, 20);
    glVertex3f(1, 2.5f, 20);
    glVertex3f(1, 4, 20);
    glVertex3f(-1, 4, 20);
    glEnd();
    
    // Exit door
    glColor3f(0.0f, 1.0f, 0.5f);
    glBegin(GL_QUADS);
    glVertex3f(-1, 0, 20);
    glVertex3f(1, 0, 20);
    glVertex3f(1, 2.5f, 20);
    glVertex3f(-1, 2.5f, 20);
    glEnd();

    // Left wall
    glColor3f(0.35f, 0.35f, 0.4f);
    glBegin(GL_QUADS);
    glVertex3f(-4, 0, 0);
    glVertex3f(-4, 0, 20);
    glVertex3f(-4, 4, 20);
    glVertex3f(-4, 4, 0);
    glEnd();

    // Right wall
    glBegin(GL_QUADS);
    glVertex3f( 4, 0, 0);
    glVertex3f( 4, 0, 20);
    glVertex3f( 4, 4, 20);
    glVertex3f( 4, 4, 0);
    glEnd();


    laserTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    // Horizontal sweeping lasers (Red)
    for (int i = 0; i < 3; i++) {
        float laserZ = 4.0f + (i * 6.0f);
        float laserX = sin(laserTime * 0.5f) * 3.5f;
        

        glColor3f(1.0f, 0.3f, 0.3f);
        glLineWidth(3.0f);
        glBegin(GL_LINES);
        glVertex3f(laserX, 0.5f, laserZ);
        glVertex3f(laserX, 3.5f, laserZ);
        glEnd();
        
        glColor3f(1.0f, 0.0f, 0.0f);
        glLineWidth(1.5f);
        glBegin(GL_LINES);
        glVertex3f(laserX, 0.5f, laserZ);
        glVertex3f(laserX, 3.5f, laserZ);
        glEnd();
    }
    glLineWidth(1.0f);

    // Horizontal pulsing lasers (Green)
    for (int i = 0; i < 3; i++) {
        float laserZ = 7.0f + (i * 6.0f);
        float cycle = fmod(laserTime, 2.0f);
        if (cycle < 1.0f) { 

            glColor3f(0.3f, 1.0f, 0.3f);
            glLineWidth(3.0f);
            glBegin(GL_LINES);
            glVertex3f(-3.5f, 1.5f, laserZ);
            glVertex3f(3.5f, 1.5f, laserZ);
            glEnd();
            
            glColor3f(0.0f, 1.0f, 0.0f);
            glLineWidth(1.5f);
            glBegin(GL_LINES);
            glVertex3f(-3.5f, 1.5f, laserZ);
            glVertex3f(3.5f, 1.5f, laserZ);
            glEnd();
        }
    }
    glLineWidth(1.0f);

    drawPlayer();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(10, 580, "Level: 1 - Laser Corridor");
    
    if (!laserCorridorLoaded) {
        laserCorridorLoaded = true;
        instructionDisplayTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
        levelStarted = false;
    }
    

    if (!levelStarted) {
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
        glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(800, 0);
        glVertex2f(800, 600);
        glVertex2f(0, 600);
        glEnd();
        
        // Main dialog box 
        glColor4f(0.1f, 0.1f, 0.15f, 0.75f);
        glBegin(GL_QUADS);
        glVertex2f(100, 150);
        glVertex2f(700, 150);
        glVertex2f(700, 480);
        glVertex2f(100, 480);
        glEnd();
        
        // Outer glow border
        glColor3f(0.0f, 1.0f, 1.0f);
        glLineWidth(3.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(100, 150);
        glVertex2f(700, 150);
        glVertex2f(700, 480);
        glVertex2f(100, 480);
        glEnd();
        glLineWidth(1.0f);
        
        // Title bar 
        glColor4f(0.05f, 0.05f, 0.1f, 0.8f);
        glBegin(GL_QUADS);
        glVertex2f(100, 430);
        glVertex2f(700, 430);
        glVertex2f(700, 480);
        glVertex2f(100, 480);
        glEnd();
        
        glDisable(GL_BLEND);
        
        // Title 
        glColor3f(0.0f, 0.5f, 0.6f);
        drawText(261, 449, "L1-LASER CORRIDOR");
        glColor3f(0.0f, 1.0f, 1.0f);
        drawText(260, 450, "L1-LASER CORRIDOR");
        
        // Instructions
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText(140, 390, "OBJECTIVE:");
        glColor3f(0.9f, 0.9f, 1.0f);
        drawText(160, 360, "Reach the green exit door within 90 seconds");
        
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText(140, 320, "HAZARDS:");
        glColor3f(1.0f, 0.4f, 0.4f);
        drawText(160, 290, "RED LASERS - Vertical sweeping beams");
        glColor3f(0.4f, 1.0f, 0.4f);
        drawText(160, 260, "GREEN LASERS - Horizontal pulsing barriers");
        
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText(140, 220, "WARNING:");
        glColor3f(1.0f, 1.0f, 0.5f);
        drawText(160, 190, "Touching any laser resets your position!");
        
        // Start button 
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.0f, 0.7f, 0.4f, 0.8f);
        glBegin(GL_QUADS);
        glVertex2f(250, 120);
        glVertex2f(550, 120);
        glVertex2f(550, 80);
        glVertex2f(250, 80);
        glEnd();
        glDisable(GL_BLEND);
        
        // Button border
        glColor3f(0.0f, 1.0f, 0.6f);
        glLineWidth(2.5f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(250, 120);
        glVertex2f(550, 120);
        glVertex2f(550, 80);
        glVertex2f(250, 80);
        glEnd();
        glLineWidth(1.0f);
        
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText(295, 95, "PRESS SPACE TO START");
    } else {

        float elapsedTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f - levelStartTime;
        float remainingTime = LEVEL_TIME_LIMIT - elapsedTime;
        
        if (remainingTime <= 0.0f) {
            currentState = LASER_CORRIDOR;
            levelStarted = false;
        } else {
            if (remainingTime < 10.0f) {
                glColor3f(1.0f, 0.0f, 0.0f); 
            } else {
                glColor3f(0.0f, 1.0f, 1.0f);
            }
            drawText(650, 580, "Time: " + to_string((int)remainingTime) + "s");
        }
    }
    
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (currentState == MENU) {
        drawMenu();
    } else if (currentState == INTRO) {
        if (playerZ >= 4.5f && playerX > -1.0f && playerX < 1.0f) {
            currentState = LASER_CORRIDOR;
            playerX = 0.0f;
            playerZ = 0.5f;
            laserCorridorLoaded = false;
            levelStarted = false;
        }
        drawGame();
    } else if (currentState == LASER_CORRIDOR) {
        drawLaserCorridor();
    }

    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
    if (currentState == MENU) {
        if (key == '1') {
            currentState = INTRO;
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

    if (currentState == INTRO || currentState == LASER_CORRIDOR) {
        float newX = playerX;
        float newZ = playerZ;
        bool moved = false;

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
            bool collision = false;
            if (currentState == INTRO) {
                collision = checkCollision(newX, newZ);
            } else if (currentState == LASER_CORRIDOR) {
                collision = checkCollisionLaserCorridor(newX, newZ);
                
                if (levelStarted && !collision) {
                    collision = checkLaserCollision(newX, 1.0f, newZ); 
                    if (collision) {
                        resetLaserLevel();
                        return; 
                    }
                }
                
                if (levelStarted && newZ >= 19.5f && newX >= -1.0f && newX <= 1.0f) {
                    currentState = MENU;
                    levelStarted = false;
                }
            }
            
            if (!collision) {
                playerX = newX;
                playerZ = newZ;
                isMoving = true;
                animationTime += 0.016f; 
            }
        } else {
            isMoving = false;
            animationTime = 0.0f;
        }

        

        if (key == ' ' && currentState == LASER_CORRIDOR && !levelStarted) {
            levelStarted = true;
            levelStartTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
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
    if ((currentState == INTRO || currentState == LASER_CORRIDOR) && mouseLocked && mouseDown) {
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
    if ((currentState == INTRO || currentState == LASER_CORRIDOR) && mouseLocked) {
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
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    GLfloat ambientLight[] = {0.3f, 0.3f, 0.3f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientLight);

    GLfloat light_position[] = {1.0f, 1.0f, 1.0f, 0.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glutDisplayFunc(display);
    glutIdleFunc(display);
    glutKeyboardFunc(keyboard);
    glutMotionFunc(mouseMotion);
    glutMouseFunc(mouse);

    glutMainLoop();
    return 0;
}
