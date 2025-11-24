#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <iostream>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstdio>
using namespace std;

enum GameState { MENU, DARK_HUNT, LASER_CORRIDOR };
GameState currentState = MENU;


float playerX = 0.0f;
float playerZ = 0.0f;
float speed = 0.1f;
float playerRadius = 0.35f; 

float cameraYaw = 0.0f;
float cameraPitch = 0.0f;
int lastMouseX = 400;
int lastMouseY = 300;
bool mouseLocked = false;
bool mouseDown = false;

float animationTime = 0.0f;
bool isMoving = false;
float playerLookAngle = 0.0f;

float laserTime = 0.0f;
bool laserCorridorLoaded = false;
bool levelStarted = false;
float levelStartTime = 0.0f;
const float LEVEL_TIME_LIMIT = 60.0f;
int playerScore = 0;
int highestScore = 0;
float lastScoreTime = 0.0f;
const float SCORE_COOLDOWN = 2.0f;

bool mazeRoomLoaded = false;
bool mazeStarted = false;

struct Wall {
    float x1, z1, x2, z2;
};

const int NUM_MAZE_WALLS = 17;
Wall mazeWalls[NUM_MAZE_WALLS];

struct Marker {
    float x, z;
    bool active;
};

const int NUM_MARKERS = 3;
Marker markers[NUM_MARKERS] = {
    { 0.0f,  5.0f, false },
    { -3.0f, 10.0f, false },
    { 2.0f, 15.0f, false }
};

float markerOriginalPositions[NUM_MARKERS][2] = {
    { 0.0f,  5.0f },
    { -3.0f, 10.0f },
    { 2.0f, 15.0f }
};

int currentMarker = 0;

struct HeatZone {
    float x, z;
    float width, depth;
    float cycleTime;
    float hotDuration;
    float timeOffset;
    bool isHot;
    bool touched;
};

HeatZone heatZones[6];

void initializeHeatZones() {
    for (int i = 0; i < 6; i++) {
        heatZones[i].x = (rand() % 7 - 3) * 1.0f;
        float baseZ = 3.0f + (i * 3.0f);
        heatZones[i].z = baseZ + (rand() % 3 - 1) * 0.5f;
        heatZones[i].width = 2.0f;
        heatZones[i].depth = 2.0f;
        heatZones[i].cycleTime = 2.0f + (rand() % 40) * 0.1f;
        heatZones[i].hotDuration = 1.0f + (rand() % 20) * 0.1f;
        heatZones[i].timeOffset = (rand() % 60) * 0.1f;
        heatZones[i].isHot = false;
        heatZones[i].touched = false;
    }
}

void updateHeatZones(float currentTime) {
    for (int i = 0; i < 6; i++) {
        float adjustedTime = currentTime + heatZones[i].timeOffset;
        float cyclePosition = fmod(adjustedTime, heatZones[i].cycleTime);
        heatZones[i].isHot = (cyclePosition < heatZones[i].hotDuration);
    }
}

int checkHeatZoneCollision(float x, float z) {
    for (int i = 0; i < 6; i++) {
        if (heatZones[i].isHot) {
            float halfWidth = heatZones[i].width / 2.0f;
            float halfDepth = heatZones[i].depth / 2.0f;
            
            if (x >= heatZones[i].x - halfWidth && x <= heatZones[i].x + halfWidth &&
                z >= heatZones[i].z - halfDepth && z <= heatZones[i].z + halfDepth) {
                return i + 1;
            }
        }
    }
    return 0;
}

void drawHeatZones() {
    for (int i = 0; i < 6; i++) {
        glPushMatrix();
        glTranslatef(heatZones[i].x, 0.02f, heatZones[i].z);
        
        if (heatZones[i].isHot) {
            glColor3f(0.0f, 0.2f, 0.4f);
            glBegin(GL_QUADS);
            glVertex3f(-heatZones[i].width/2.0f, 0.0f, -heatZones[i].depth/2.0f);
            glVertex3f( heatZones[i].width/2.0f, 0.0f, -heatZones[i].depth/2.0f);
            glVertex3f( heatZones[i].width/2.0f, 0.0f,  heatZones[i].depth/2.0f);
            glVertex3f(-heatZones[i].width/2.0f, 0.0f,  heatZones[i].depth/2.0f);
            glEnd();
            
            glColor3f(0.0f, 0.4f, 0.8f);
            glBegin(GL_QUADS);
            glVertex3f(-heatZones[i].width/2.0f - 0.1f, 0.01f, -heatZones[i].depth/2.0f - 0.1f);
            glVertex3f( heatZones[i].width/2.0f + 0.1f, 0.01f, -heatZones[i].depth/2.0f - 0.1f);
            glVertex3f( heatZones[i].width/2.0f + 0.1f, 0.01f,  heatZones[i].depth/2.0f + 0.1f);
            glVertex3f(-heatZones[i].width/2.0f - 0.1f, 0.01f,  heatZones[i].depth/2.0f + 0.1f);
            glEnd();
        }
        
        glPopMatrix();
    }
}


void drawText(float x, float y, string text) {
    glRasterPos2f(x, y);
    for (char c : text) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
}

bool checkMazeWallCollision(float newX, float newZ) {
    for (int i = 0; i < NUM_MAZE_WALLS; i++) {
        Wall w = mazeWalls[i];
        
        if (fabs(w.x1 - w.x2) > fabs(w.z1 - w.z2)) {
            float minX = fmin(w.x1, w.x2);
            float maxX = fmax(w.x1, w.x2);
            float wallZ = w.z1;
            
            if (newX >= minX - playerRadius && newX <= maxX + playerRadius) {
                if (fabs(newZ - wallZ) < playerRadius + 0.2f) {
                    return true;
                }
            }
        } else {
            float minZ = fmin(w.z1, w.z2);
            float maxZ = fmax(w.z1, w.z2);
            float wallX = w.x1;
            
            if (newZ >= minZ - playerRadius && newZ <= maxZ + playerRadius) {
                if (fabs(newX - wallX) < playerRadius + 0.2f) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool checkCollisionMazeRoom(float newX, float newZ) {
    float minX = -10.0f + playerRadius;
    float maxX = 10.0f - playerRadius;
    float minZ = 0.0f + playerRadius;
    float maxZ = 20.0f - playerRadius;
    
    if (newZ > maxZ) {
        if (newX >= -1.0f && newX <= 1.0f) {
            if (currentMarker < NUM_MARKERS) {
                return true;
            }
            return false;
        }
        return true;
    }
    
    if (newX < minX || newX > maxX || newZ < minZ) {
        return true;
    }
    
    if (mazeStarted && checkMazeWallCollision(newX, newZ)) {
        return true;
    }
    
    return false;
}

bool checkCollisionLaserCorridor(float newX, float newZ) {
    float minX = -4.0f + playerRadius;
    float maxX = 4.0f - playerRadius;
    float minZ = 0.0f + playerRadius;
    float maxZ = 20.0f - playerRadius;
    
    if (newZ > maxZ) {
        if (newX >= -1.0f && newX <= 1.0f) {
            return false;
        }
        return true;
    }
    
    if (newX < minX || newX > maxX || newZ < minZ) {
        return true;
    }
    return false;
}

bool checkLaserCollision(float x, float y, float z) {
    laserTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    
    // Red sweeping lasers
    for (int i = 0; i < 3; i++) {
        float redLaserZ = 4.0f + (i * 6.0f);
        float redLaserX = sin(laserTime * 0.5f) * 3.5f;
        
        if (fabs(z - redLaserZ) < 0.3f && fabs(x - redLaserX) < 0.25f && y < 3.8f && y > 0.2f) {
            return true;
        }
    }
    
    // Green pulsing lasers
    for (int i = 0; i < 3; i++) {
        float greenLaserZ = 7.0f + (i * 6.0f);
        float cycle = fmod(laserTime, 2.0f);
        if (cycle < 1.0f) {
            if (fabs(z - greenLaserZ) < 0.3f && y < 2.5f && y > 0.5f && fabs(x) < 3.7f) {
                return true;
            }
        }
    }
    
    if (levelStarted && checkHeatZoneCollision(x, z)) {
        return true;
    }
    
    return false;
}

void drawPlayer() {
    glPushMatrix();
    glTranslatef(playerX, 0.0f, playerZ);
    glRotatef(playerLookAngle, 0.0f, 1.0f, 0.0f);

    glColor3f(0.2f, 0.2f, 0.8f);
    glPushMatrix();
    glTranslatef(-0.15f, 0.4f, 0.0f);
    if (isMoving) {
        float legRotation = sin(animationTime * 15.0f) * 15.0f; 
        glRotatef(legRotation, 1.0f, 0.0f, 0.0f);
    }
    glScalef(0.15f, 0.8f, 0.15f);
    glutSolidCube(1.0);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(0.15f, 0.4f, 0.0f);
    if (isMoving) {
        float legRotation = sin(animationTime * 15.0f + 3.14159f) * 15.0f; 
        glRotatef(legRotation, 1.0f, 0.0f, 0.0f);
    }
    glScalef(0.15f, 0.8f, 0.15f);
    glutSolidCube(1.0);
    glPopMatrix();

    glColor3f(0.8f, 0.1f, 0.1f);
    glPushMatrix();
    glTranslatef(0.0f, 1.1f, 0.0f);
    glScalef(0.5f, 0.7f, 0.25f);
    glutSolidCube(1.0);
    glPopMatrix();

    glColor3f(1.0f, 0.8f, 0.6f);
    glPushMatrix();
    glTranslatef(-0.35f, 1.1f, 0.0f);
    if (isMoving) {
        float armRotation = sin(animationTime * 15.0f + 3.14159f) * 20.0f; 
        glRotatef(armRotation, 1.0f, 0.0f, 0.0f);
    }
    glScalef(0.12f, 0.6f, 0.12f);
    glutSolidCube(1.0);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(0.35f, 1.1f, 0.0f);
    if (isMoving) {
        float armRotation = sin(animationTime * 15.0f) * 20.0f; 
        glRotatef(armRotation, 1.0f, 0.0f, 0.0f);
    }
    glScalef(0.12f, 0.6f, 0.12f);
    glutSolidCube(1.0);
    glPopMatrix();

    glColor3f(1.0f, 0.8f, 0.6f);
    glPushMatrix();
    glTranslatef(0.0f, 1.55f, 0.0f);
    glScalef(0.1f, 0.15f, 0.1f);
    glutSolidCube(1.0);
    glPopMatrix();

    glColor3f(1.0f, 0.8f, 0.6f);
    glPushMatrix();
    glTranslatef(0.0f, 1.8f, 0.0f);
    glutSolidSphere(0.25, 20, 20);
    glPopMatrix();

    glColor3f(0.0f, 0.0f, 0.0f);
    glPushMatrix();
    glTranslatef(-0.08f, 1.85f, 0.22f);
    glutSolidSphere(0.03, 10, 10);
    glPopMatrix();
    
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

void drawMinimap() {
    float mmSize = 150.0f;
    float mmX = 800 - mmSize - 20; // Top right X
    float mmY = 600 - mmSize - 20; // Top right Y
    
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(mmX, mmY);
    glVertex2f(mmX + mmSize, mmY);
    glVertex2f(mmX + mmSize, mmY + mmSize);
    glVertex2f(mmX, mmY + mmSize);
    glEnd();
    glDisable(GL_BLEND);
    
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(mmX, mmY);
    glVertex2f(mmX + mmSize, mmY);
    glVertex2f(mmX + mmSize, mmY + mmSize);
    glVertex2f(mmX, mmY + mmSize);
    glEnd();
    glLineWidth(1.0f);
    
    float scaleX = mmSize / 20.0f;
    float scaleZ = mmSize / 20.0f;
    
    float px = mmX + (mmSize - (playerX + 10.0f) * scaleX);
    float py = mmY + (mmSize - (playerZ) * scaleZ);
    
    if(px < mmX) px = mmX; if(px > mmX + mmSize) px = mmX + mmSize;
    if(py < mmY) py = mmY; if(py > mmY + mmSize) py = mmY + mmSize;
    
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_POLYGON);
    for(int i=0; i<360; i+=45) {
        float rad = i * 3.14159f / 180.0f;
        glVertex2f(px + cos(rad)*3, py + sin(rad)*3);
    }
    glEnd();
    
    for(int i=0; i<NUM_MARKERS; i++) {
        if(!markers[i].active) {
            float mx = mmX + (mmSize - (markerOriginalPositions[i][0] + 10.0f) * scaleX);
            float my = mmY + (mmSize - (markerOriginalPositions[i][1]) * scaleZ);
            
            glColor3f(1.0f, 1.0f, 0.0f);
            glBegin(GL_POLYGON);
            for(int j=0; j<360; j+=90) { 
                float rad = j * 3.14159f / 180.0f;
                glVertex2f(mx + cos(rad)*3, my + sin(rad)*3);
            }
            glEnd();
        }
    }
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void drawMazeRoom() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, 800/600.0, 0.1, 100);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float camX = playerX + 3.0f * sin(cameraYaw * 3.14159f / 180.0f);
    float camY = 3.0f + 3.0f * sin(cameraPitch * 3.14159f / 180.0f);
    float camZ = playerZ + 3.0f * cos(cameraYaw * 3.14159f / 180.0f);
    
    if (camX < -10.0f) camX = -10.0f;
    if (camX > 10.0f) camX = 10.0f;
    if (camZ < 0.0f) camZ = 0.0f;
    if (camZ > 20.0f) camZ = 20.0f;
    if (camY < 0.5f) camY = 0.5f;
    if (camY > 3.8f) camY = 3.8f;

    gluLookAt(camX, camY, camZ, playerX, 1, playerZ, 0, 1, 0);

    if (mazeStarted) {
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT1);
        
        GLfloat spotlightPosition[] = {playerX, 3.5f, playerZ, 1.0f};
        GLfloat spotlightDirection[] = {0.0f, -1.0f, 0.0f};
        GLfloat spotlightAmbient[] = {0.01f, 0.01f, 0.01f, 1.0f};
        GLfloat spotlightDiffuse[] = {0.9f, 0.9f, 0.7f, 1.0f};
        GLfloat spotlightSpecular[] = {0.3f, 0.3f, 0.3f, 1.0f};

        glLightfv(GL_LIGHT1, GL_POSITION, spotlightPosition);
        glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, spotlightDirection);
        glLightfv(GL_LIGHT1, GL_AMBIENT, spotlightAmbient);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, spotlightDiffuse);
        glLightfv(GL_LIGHT1, GL_SPECULAR, spotlightSpecular);
        
        glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 25.0f);
        glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 20.0f);
        glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 1.0f);
        glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.15f);
        glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.08f);

        GLfloat darkAmbient[] = {0.02f, 0.02f, 0.02f, 1.0f};
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, darkAmbient);
    } else {
        glEnable(GL_LIGHTING);
        GLfloat normalAmbient[] = {0.3f, 0.3f, 0.3f, 1.0f};
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, normalAmbient);
    }

    glColor3f(0.15f, 0.15f, 0.15f);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-10, 0, 0);
    glVertex3f( 10, 0, 0);
    glVertex3f( 10, 0, 20);
    glVertex3f(-10, 0, 20);
    glEnd();

    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-10, 4, 0);
    glVertex3f( 10, 4, 0);
    glVertex3f( 10, 4, 20);
    glVertex3f(-10, 4, 20);
    glEnd();

    glColor3f(0.25f, 0.25f, 0.3f);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-10, 0, 0);
    glVertex3f( 10, 0, 0);
    glVertex3f( 10, 4, 0);
    glVertex3f(-10, 4, 0);
    glEnd();

    glColor3f(0.25f, 0.25f, 0.3f);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(-10, 0, 20);
    glVertex3f(-1, 0, 20);
    glVertex3f(-1, 4, 20);
    glVertex3f(-10, 4, 20);
    glEnd();
    
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(1, 0, 20);
    glVertex3f(10, 0, 20);
    glVertex3f(10, 4, 20);
    glVertex3f(1, 4, 20);
    glEnd();
    
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(-1, 2.5f, 20);
    glVertex3f(1, 2.5f, 20);
    glVertex3f(1, 4, 20);
    glVertex3f(-1, 4, 20);
    glEnd();

    glColor3f(0.0f, 0.8f, 0.3f);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(-1, 0, 20);
    glVertex3f(1, 0, 20);
    glVertex3f(1, 2.5f, 20);
    glVertex3f(-1, 2.5f, 20);
    glEnd();

    if (mazeStarted) {
        glColor3f(0.0f, 1.0f, 0.5f);
        glLineWidth(3.0f);
        glBegin(GL_LINE_LOOP);
        glVertex3f(-1, 0, 19.99f);
        glVertex3f(1, 0, 19.99f);
        glVertex3f(1, 2.5f, 19.99f);
        glVertex3f(-1, 2.5f, 19.99f);
        glEnd();
        glLineWidth(1.0f);
    }

    glColor3f(0.25f, 0.25f, 0.3f);
    glBegin(GL_QUADS);
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(-10, 0, 0);
    glVertex3f(-10, 0, 20);
    glVertex3f(-10, 4, 20);
    glVertex3f(-10, 4, 0);
    glEnd();

    glBegin(GL_QUADS);
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(10, 0, 0);
    glVertex3f(10, 0, 20);
    glVertex3f(10, 4, 20);
    glVertex3f(10, 4, 0);
    glEnd();

    glColor3f(0.3f, 0.3f, 0.35f);
    for (int i = 0; i < NUM_MAZE_WALLS; i++) {
        Wall w = mazeWalls[i];
        float wallThickness = 0.2f;
        
        if (fabs(w.x1 - w.x2) > fabs(w.z1 - w.z2)) {
            // Horizontal wall
            glBegin(GL_QUADS);
            // Front face
            glNormal3f(0.0f, 0.0f, 1.0f);
            glVertex3f(w.x1, 0, w.z1 + wallThickness);
            glVertex3f(w.x2, 0, w.z2 + wallThickness);
            glVertex3f(w.x2, 4, w.z2 + wallThickness);
            glVertex3f(w.x1, 4, w.z1 + wallThickness);
            glEnd();
            
            glBegin(GL_QUADS);
            glNormal3f(0.0f, 0.0f, -1.0f);
            glVertex3f(w.x1, 0, w.z1 - wallThickness);
            glVertex3f(w.x2, 0, w.z2 - wallThickness);
            glVertex3f(w.x2, 4, w.z2 - wallThickness);
            glVertex3f(w.x1, 4, w.z1 - wallThickness);
            glEnd();
            
            glBegin(GL_QUADS);
            // Top face
            glNormal3f(0.0f, 1.0f, 0.0f);
            glVertex3f(w.x1, 4, w.z1 - wallThickness);
            glVertex3f(w.x2, 4, w.z2 - wallThickness);
            glVertex3f(w.x2, 4, w.z2 + wallThickness);
            glVertex3f(w.x1, 4, w.z1 + wallThickness);
            glEnd();
            
        } else {
            // Vertical wall
            glBegin(GL_QUADS);
            // Front face
            glNormal3f(1.0f, 0.0f, 0.0f);
            glVertex3f(w.x1 + wallThickness, 0, w.z1);
            glVertex3f(w.x2 + wallThickness, 0, w.z2);
            glVertex3f(w.x2 + wallThickness, 4, w.z2);
            glVertex3f(w.x1 + wallThickness, 4, w.z1);
            glEnd();
            
            glBegin(GL_QUADS);
            glNormal3f(-1.0f, 0.0f, 0.0f);
            glVertex3f(w.x1 - wallThickness, 0, w.z1);
            glVertex3f(w.x2 - wallThickness, 0, w.z2);
            glVertex3f(w.x2 - wallThickness, 4, w.z2);
            glVertex3f(w.x1 - wallThickness, 4, w.z1);
            glEnd();
            
            glBegin(GL_QUADS);
            // Top face
            glNormal3f(0.0f, 1.0f, 0.0f);
            glVertex3f(w.x1 - wallThickness, 4, w.z1);
            glVertex3f(w.x2 - wallThickness, 4, w.z2);
            glVertex3f(w.x2 + wallThickness, 4, w.z2);
            glVertex3f(w.x1 + wallThickness, 4, w.z1);
            glEnd();
        }
    }

    for(int i = 0; i < NUM_MARKERS; i++) {
        if(!markers[i].active) {
            float dx = playerX - markers[i].x;
            float dz = playerZ - markers[i].z;
            float dist = sqrt(dx*dx + dz*dz);
            
            if (dist < 4.0f) {
                glPushMatrix();
                glTranslatef(markers[i].x, 1.0f, markers[i].z);
                
                float hover = sin(glutGet(GLUT_ELAPSED_TIME) / 200.0f) * 0.2f;
                glTranslatef(0.0f, hover, 0.0f);
                
                glColor3f(1.0f, 1.0f, 0.0f);
                GLfloat yellow[] = {1.0f, 1.0f, 0.0f, 1.0f};
                glMaterialfv(GL_FRONT, GL_EMISSION, yellow);
                
                glutSolidSphere(0.2, 20, 20);
                
                GLfloat noGlow[] = {0.0f, 0.0f, 0.0f, 1.0f};
                glMaterialfv(GL_FRONT, GL_EMISSION, noGlow);
                glPopMatrix();
            }
        }
    }

    drawPlayer();

    glDisable(GL_LIGHT1);
    GLfloat resetAmbient[] = {0.3f, 0.3f, 0.3f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, resetAmbient);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    

    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(10, 580, "Level: 1 - The Dark Hunt");
    
    string countStr = "Markers: " + to_string(currentMarker) + " / " + to_string(NUM_MARKERS);
    if(currentMarker < NUM_MARKERS) glColor3f(1.0f, 0.5f, 0.0f);
    else glColor3f(0.0f, 1.0f, 0.0f);
    drawText(10, 550, countStr);
    
    if(currentMarker < NUM_MARKERS) {
        glColor3f(1.0f, 0.0f, 0.0f);
        drawText(10, 520, "Exit Locked");
    } else {
        glColor3f(0.0f, 1.0f, 0.0f);
        drawText(10, 520, "Exit Open!");
    }

    if(mazeStarted) {
        drawMinimap();
    }

    if (!mazeRoomLoaded) {
        mazeRoomLoaded = true;
        mazeStarted = false;
    }
    if (!mazeStarted) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.0f, 0.0f, 0.0f, 0.8f);
        glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(800, 0);
        glVertex2f(800, 600);
        glVertex2f(0, 600);
        glEnd();
        
        // Main dialog box
        glColor4f(0.1f, 0.1f, 0.15f, 0.75f);
        glBegin(GL_QUADS);
        glVertex2f(50, 80);
        glVertex2f(750, 80);
        glVertex2f(750, 520);
        glVertex2f(50, 520);
        glEnd();
        
        // Outer glow border
        glColor3f(1.0f, 1.0f, 0.0f);
        glLineWidth(3.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(50, 80);
        glVertex2f(750, 80);
        glVertex2f(750, 520);
        glVertex2f(50, 520);
        glEnd();
        glLineWidth(1.0f);
        
        // Title bar
        glColor4f(0.05f, 0.05f, 0.1f, 0.8f);
        glBegin(GL_QUADS);
        glVertex2f(50, 470);
        glVertex2f(750, 470);
        glVertex2f(750, 520);
        glVertex2f(50, 520);
        glEnd();
        
        glDisable(GL_BLEND);
        
        // Title
        glColor3f(0.8f, 0.8f, 0.0f);
        drawText(271, 489, "WELCOME TO THE DARK HUNT");
        glColor3f(1.0f, 1.0f, 0.0f);
        drawText(270, 490, "WELCOME TO THE DARK HUNT");
        
        // Story text
        glColor3f(0.9f, 0.9f, 0.9f);
        drawText(70, 440, "Welcome to your escape training. This dark room will help you learn");
        drawText(70, 415, "the controls before facing greater challenges ahead.");
        
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText(70, 380, "CONTROLS:");
        glColor3f(0.9f, 0.9f, 0.9f);
        drawText(90, 355, "W, A, S, D - Move around");
        drawText(90, 330, "MOUSE - Click and drag to look around");
        drawText(90, 305, "M - Return to menu");
        
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText(70, 270, "OBJECTIVE:");
        glColor3f(0.9f, 0.9f, 0.9f);
        drawText(90, 245, "Find and collect 3 yellow markers hidden in the darkness.");
        drawText(90, 220, "Use the minimap in the top-right corner to navigate.");
        drawText(90, 195, "Markers only appear when you get close to them.");
        drawText(90, 170, "Collect all markers to unlock the exit door.");
        
        // Start button
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1.0f, 0.8f, 0.0f, 0.8f);
        glBegin(GL_QUADS);
        glVertex2f(250, 130);
        glVertex2f(550, 130);
        glVertex2f(550, 90);
        glVertex2f(250, 90);
        glEnd();
        glDisable(GL_BLEND);
        
        // Button border
        glColor3f(1.0f, 1.0f, 0.0f);
        glLineWidth(2.5f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(250, 130);
        glVertex2f(550, 130);
        glVertex2f(550, 90);
        glVertex2f(250, 90);
        glEnd();
        glLineWidth(1.0f);
        
        glColor3f(0.0f, 0.0f, 0.0f);
        drawText(295, 105, "PRESS SPACE TO START");
    }
    
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
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

    // Front wall (no exit door)
    glColor3f(0.35f, 0.35f, 0.4f);
    glBegin(GL_QUADS);
    glVertex3f(-4, 0, 20);
    glVertex3f(4, 0, 20);
    glVertex3f(4, 4, 20);
    glVertex3f(-4, 4, 20);
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

    // Draw heat zones
    if (levelStarted) {
        drawHeatZones();
    }

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
    drawText(10, 580, "Final Challenge - Laser Corridor");
    
    // Display score
    glColor3f(0.0f, 1.0f, 0.5f);
    drawText(10, 550, "Score: " + to_string(playerScore));
    
    // Display high score
    glColor3f(1.0f, 1.0f, 0.0f);
    drawText(10, 520, "Best: " + to_string(highestScore));
    
    if (!laserCorridorLoaded) {
        laserCorridorLoaded = true;
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
        glVertex2f(50, 80);
        glVertex2f(750, 80);
        glVertex2f(750, 520);
        glVertex2f(50, 520);
        glEnd();
        
        // Outer glow border
        glColor3f(0.0f, 1.0f, 1.0f);
        glLineWidth(3.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(50, 80);
        glVertex2f(750, 80);
        glVertex2f(750, 520);
        glVertex2f(50, 520);
        glEnd();
        glLineWidth(1.0f);
        
        // Title bar 
        glColor4f(0.05f, 0.05f, 0.1f, 0.8f);
        glBegin(GL_QUADS);
        glVertex2f(50, 470);
        glVertex2f(750, 470);
        glVertex2f(750, 520);
        glVertex2f(50, 520);
        glEnd();
        
        glDisable(GL_BLEND);
        
        // Title 
        glColor3f(0.0f, 0.5f, 0.6f);
        drawText(251, 489, "L2 - LASER CORRIDOR");
        glColor3f(0.0f, 1.0f, 1.0f);
        drawText(250, 490, "L2 - LASER CORRIDOR");
        
        // Story text
        glColor3f(0.9f, 0.9f, 1.0f);
        drawText(70, 440, "This is the final test. You have 60 seconds to score as many points");
        drawText(70, 415, "as possible while avoiding deadly hazards.");
        
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText(70, 380, "SCORING:");
        glColor3f(0.4f, 1.0f, 0.4f);
        drawText(90, 355, "Step on GLOWING tiles: +1 point");
        glColor3f(1.0f, 0.4f, 0.4f);
        drawText(90, 330, "Touch any laser: -1 point");
        
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText(70, 295, "HAZARDS:");
        glColor3f(0.9f, 0.9f, 1.0f);
        drawText(90, 270, "Red lasers sweep vertically across the corridor");
        drawText(90, 245, "Green lasers pulse horizontally in timed patterns");
        drawText(90, 220, "Floor tiles randomly glow - step on them for points!");
        
        glColor3f(1.0f, 1.0f, 0.5f);
        drawText(70, 185, "Get the highest score before time runs out!");
        
        // Start button 
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.0f, 0.7f, 0.4f, 0.8f);
        glBegin(GL_QUADS);
        glVertex2f(250, 130);
        glVertex2f(550, 130);
        glVertex2f(550, 90);
        glVertex2f(250, 90);
        glEnd();
        glDisable(GL_BLEND);
        
        // Button border
        glColor3f(0.0f, 1.0f, 0.6f);
        glLineWidth(2.5f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(250, 130);
        glVertex2f(550, 130);
        glVertex2f(550, 90);
        glVertex2f(250, 90);
        glEnd();
        glLineWidth(1.0f);
        
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText(295, 105, "PRESS SPACE TO START");
    } else {

        float elapsedTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f - levelStartTime;
        float remainingTime = LEVEL_TIME_LIMIT - elapsedTime;
        
        if (remainingTime <= 0.0f) {
            // Game Over - Show final score
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(0.0f, 0.0f, 0.0f, 0.8f);
            glBegin(GL_QUADS);
            glVertex2f(200, 150);
            glVertex2f(600, 150);
            glVertex2f(600, 450);
            glVertex2f(200, 450);
            glEnd();
            glDisable(GL_BLEND);
            
            glColor3f(1.0f, 1.0f, 0.0f);
            drawText(320, 400, "TIME'S UP!");
            
            glColor3f(1.0f, 1.0f, 1.0f);
            drawText(280, 330, "Your Score: " + to_string(playerScore));
            
            if (playerScore > highestScore) {
                highestScore = playerScore;
                glColor3f(0.0f, 1.0f, 0.5f);
                drawText(280, 290, "NEW HIGH SCORE!");
            } else {
                glColor3f(0.8f, 0.8f, 0.8f);
                drawText(260, 290, "Best Score: " + to_string(highestScore));
            }
            
            glColor3f(0.5f, 0.5f, 0.5f);
            drawText(250, 200, "Press M to return to menu");
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

    // Update heat zones if laser corridor is active
    if (currentState == LASER_CORRIDOR && levelStarted) {
        float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
        updateHeatZones(currentTime);
        
        // Debug: Print hot zones count and player position periodically
        static int debugCounter = 0;
        if (debugCounter % 60 == 0) { // Print every 60 frames (~1 second)
            int hotCount = 0;
            for (int i = 0; i < 6; i++) {
                if (heatZones[i].isHot) hotCount++;
            }
            printf("Hot zones: %d, Player at (%.2f, %.2f), Score: %d\n", 
                   hotCount, playerX, playerZ, playerScore);
        }
        debugCounter++;
        
        // Scoring check with cooldown to prevent too rapid updates
        if (currentTime - lastScoreTime >= SCORE_COOLDOWN) {
            bool scored = false;
            
            // Check heat zone collision for +1 point (when glowing)
            int zoneHit = checkHeatZoneCollision(playerX, playerZ);
            if (zoneHit > 0) {
                printf("HIT ZONE %d! Score: %d -> %d\n", zoneHit, playerScore, playerScore + 1);
                playerScore++;
                lastScoreTime = currentTime;
                scored = true;
            }
            
            // Check laser collision for -1 point (only if not already scoring from tile)
            if (!scored && checkLaserCollision(playerX, 1.0f, playerZ)) {
                printf("HIT LASER! Score: %d -> %d\n", playerScore, playerScore - 1);
                playerScore--;
                if (playerScore < 0) playerScore = 0;
                lastScoreTime = currentTime;
            }
        }
    }

    if (currentState == MENU) {
        drawMenu();
    } else if (currentState == DARK_HUNT) {
        drawMazeRoom();
    } else if (currentState == LASER_CORRIDOR) {
        drawLaserCorridor();
    }

    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
    if (currentState == MENU) {
        if (key == '1') {
            currentState = DARK_HUNT;
            playerX = 0.0f;
            playerZ = 0.5f;
            cameraYaw = 0.0f;
            cameraPitch = 0.0f;
            mouseLocked = true;
            glutSetCursor(GLUT_CURSOR_NONE);
            glutWarpPointer(400, 300);
            mazeRoomLoaded = false;
            mazeStarted = false;
            for (int i = 0; i < NUM_MARKERS; i++) {
                markers[i].active = false;
            }
            currentMarker = 0;
        }
        if (key == '2') exit(0);
    }

    if (currentState == DARK_HUNT || currentState == LASER_CORRIDOR) {
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
            if (currentState == DARK_HUNT) {
                collision = checkCollisionMazeRoom(newX, newZ);
                
                // Check if player reached maze exit door
                if (mazeStarted && currentMarker >= NUM_MARKERS &&
                    !collision && newZ >= 19.5f && newX >= -1.0f && newX <= 1.0f) {
                    currentState = LASER_CORRIDOR;
                    playerX = 0.0f;
                    playerZ = 0.5f;
                    laserCorridorLoaded = false;
                    levelStarted = false;
                    playerScore = 0;
                    initializeHeatZones();
                    return;
                }
            } else if (currentState == LASER_CORRIDOR) {
                collision = checkCollisionLaserCorridor(newX, newZ);
            }
            
            if (!collision) {
                playerX = newX;
                playerZ = newZ;
                
                // Marker collection check
                if (currentState == DARK_HUNT && mazeStarted) {
                    float dx = playerX - markers[currentMarker].x;
                    float dz = playerZ - markers[currentMarker].z;
                    float dist = sqrt(dx*dx + dz*dz);

                    // If player touches current marker, activate next
                    if (dist < 0.7f) {
                        markers[currentMarker].active = true;
                        currentMarker++;
                    }
                }
                
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
            lastScoreTime = levelStartTime;
            playerScore = 0;
            initializeHeatZones();
        }
        
        if (key == ' ' && currentState == DARK_HUNT && !mazeStarted) {
            mazeStarted = true;
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
    if ((currentState == DARK_HUNT || currentState == LASER_CORRIDOR) && mouseLocked && mouseDown) {
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
    if ((currentState == DARK_HUNT || currentState == LASER_CORRIDOR) && mouseLocked) {
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
    srand(static_cast<unsigned>(time(0)));
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
