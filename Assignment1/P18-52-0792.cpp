#define GL_SILENCE_DEPRECATION
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <GLUT/glut.h>
#include <cmath>
#include <random>
#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <string>
#include <mutex>
#include <thread>
#include <SDL.h>
#include <SDL_mixer.h>

std::default_random_engine generator(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));
std::default_random_engine generatorX(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count() + 1000));
std::default_random_engine generatorY(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));
std::default_random_engine generatorStarsNum(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count() + 3000));
std::default_random_engine generatorStarsX(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count() + 4000));
std::default_random_engine generatorStarsY(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count() + 5000));

struct Time {
    int minutes;
    int seconds;
};

std::vector<std::array<float, 2>> takenCenters;

std::vector<std::array<float, 2>> collectibles;
std::vector<std::array<float, 2>> obstacles;
std::vector<std::array<float, 3>> powerUps;
std::vector<std::array<float, 2>> stars;

std::vector<std::string> audioFilesNames;

int direction = 1;
int direction2 = 1;
bool once = false;
bool once2 = false;
bool goalEffect = true;
bool gameRunning = true;
bool gameover = false;
bool won = false;
int healthPoints = 5;
int score = 0;
int scoreIncrease = 1;
int timeRemaining = 45;
Time displayedTime;
bool isTimeStopped = false;
int timeStopDuration = 10;
int doubleScoreDuration = 15;
bool isDoubleScoreActive = false;
int colCount = 5;
float objectX = 500.0f;
float objectY = 300.0f;
float playerRotation = 0.0f;
float colRotation = 0.0f;
float ObstacleRotation = 0.0f;
float earthRotation1 = 0.0f;
float earthRotation2 = 0.0f;
float removed = 0.0f;
float removed2 = 0.0f;
float twinkleLength = 10.0f;
bool isRandom = false;
int fn = 1;
float eyeColor = 0.0f;
bool isTakingDamage = false;
int damageDelay = 5;
char lastKey;

void print(int x, int y, char* string);
Time secondsToMinutesAndSeconds(int totalSeconds);
void timer(int value);
bool removeFloat2Array(std::vector<std::array<float, 2>>& vectorOfArrays, const std::array<float, 2>& targetArray);
bool checkExists(std::vector<std::array<float, 2>>& vectorOfArrays, const std::array<float, 2>& targetArray);
bool removeFloat3Array(std::vector<std::array<float, 3>>& vectorOfArrays, const std::array<float, 3>& targetArray);
bool IsTaken(const std::array<float, 2>& centerToAdd);
void goalReached();
void timeOut();
void playerDied();
void stopTime();
void doubleScore();
void normalScore();
void playerCollide();
void displayHealthBar();
void displayScore();
void displayTime();
void displayStats();
void displayGoal();
void drawStar(float xCenter, float yCenter);
void randomizeStars();
void displayPlayer();
void displayCollectibles(float centerX, float centerY, float radius);
void displayObstacles(float centerX, float centerY, float radius);
void displayPowerUps(float centerX, float centerY, float radius);
void displayZaWarudo(float centerX, float centerY, float radius);
void displayDoubleScore(float centerX, float centerY, float radius);
void randomize();
void displayBoundaries();
void Display();
void specialKeys(int key, int x, int y);
void anim();

void print(int x, int y, char* string, int font) {
    int len, i;
    glRasterPos2f(x, y);
    len = (int) strlen(string);
    for (i = 0; i < len; i++) {
        if (font == 12) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, string[i]);
        } else if (font == 18) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, string[i]);
        } else if (font == 24) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, string[i]);
        }
    }
}

Time secondsToMinutesAndSeconds(int totalSeconds) {
    Time result;
    result.minutes = totalSeconds / 60;
    result.seconds = totalSeconds % 60;
    return result;
}

void timer(int value) {
    if (!isTimeStopped && timeRemaining > 0) {
        timeRemaining--;
        displayedTime = secondsToMinutesAndSeconds(timeRemaining);
    }
    
    glutPostRedisplay();
    
    if (isTimeStopped) {
        if (timeStopDuration > 0) {
            timeStopDuration--;
        } else {
            timeStopDuration = 10;
            isTimeStopped = false;
        }
    }
    
    if (isDoubleScoreActive) {
        if (doubleScoreDuration > 0) {
            doubleScoreDuration--;
        } else {
            doubleScoreDuration = 25;
            scoreIncrease = 1;
            isDoubleScoreActive = false;
        }
    }
    
    if (timeRemaining > 0) {
        glutTimerFunc(1000, timer, 0);
    }
}

bool removeFloat2Array(std::vector<std::array<float, 2>>& vectorOfArrays, const std::array<float, 2>& targetArray) {
    auto it = std::find(vectorOfArrays.begin(), vectorOfArrays.end(), targetArray);

    if (it != vectorOfArrays.end()) {
        vectorOfArrays.erase(it);
        return true;
    } else {
        return false;
    }
}

bool checkExists(std::vector<std::array<float, 2>>& vectorOfArrays, const std::array<float, 2>& targetArray) {
    auto it = std::find(vectorOfArrays.begin(), vectorOfArrays.end(), targetArray);

    if (it != vectorOfArrays.end()) {
        return true;
    } else {
        return false;
    }
}

bool removeFloat3Array(std::vector<std::array<float, 3>>& vectorOfArrays, const std::array<float, 3>& targetArray) {
    auto it = std::find(vectorOfArrays.begin(), vectorOfArrays.end(), targetArray);

    if (it != vectorOfArrays.end()) {
        vectorOfArrays.erase(it);
        return true;
    } else {
        return false;
    }
}

bool isTaken(const std::array<float, 2>& centerToAdd) {
    int width = 90;
    
    float xLeftToAdd = centerToAdd[0] - (width / 2);
    float xRightToAdd = centerToAdd[0] + (width / 2);
    float yTopToAdd = centerToAdd[1] - (width / 2);
    float yBottomToAdd = centerToAdd[1] + (width / 2);
    
    for (const std::array<float, 2>& takenCenter : takenCenters) {
        
        float xLeftBoundary = takenCenter[0] - (width / 2);
        float xRightBoundary = takenCenter[0] + (width / 2);
        float yTopBoundary = takenCenter[1] - (width / 2);
        float yBottomBoundary = takenCenter[1] + (width / 2);
        
        if (xLeftToAdd < xRightBoundary && xRightToAdd > xLeftBoundary && yTopToAdd < yBottomBoundary && yBottomToAdd > yTopBoundary) {
            return true;
        }
    }
    return false;
}

void goalReached() {
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(0.0, 0.0);
    glVertex2f(0.0, 600.0);
    glVertex2f(1000.0, 600.0);
    glVertex2f(1000.0, 0.0);
    glEnd();
    
    glColor3f(1.0f, 1.0f, 1.0f);
    std::string text;
    if (score == colCount) {
        text = "You Won!";
    } else if (score > colCount) {
        text = "Outstanding!";
    }
    
    int textWidth = glutBitmapLength(GLUT_BITMAP_TIMES_ROMAN_24, (const unsigned char*)text.c_str());
    print(500 - textWidth / 2, 288, const_cast<char*>(text.c_str()), 24);
    
    std::string text2 = "Score: " + std::to_string(score);
    int textWidth2 = glutBitmapLength(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)text2.c_str());
    print(500 - textWidth2 / 2, 320, const_cast<char*>(text2.c_str()), 18);
}

void timeOut() {
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(0.0, 0.0);
    glVertex2f(0.0, 600.0);
    glVertex2f(1000.0, 600.0);
    glVertex2f(1000.0, 0.0);
    glEnd();
    
    glColor3f(1.0f, 1.0f, 1.0f);
    std::string text = "You're out of time :(";
    
    int textWidth = glutBitmapLength(GLUT_BITMAP_TIMES_ROMAN_24, (const unsigned char*)text.c_str());
    print(500 - textWidth / 2, 288, const_cast<char*>(text.c_str()), 24);
    
    std::string text2 = "Score: " + std::to_string(score);
    int textWidth2 = glutBitmapLength(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)text2.c_str());
    print(500 - textWidth2 / 2, 320, const_cast<char*>(text2.c_str()), 18);
}

void playerDied() {
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(0.0, 0.0);
    glVertex2f(0.0, 600.0);
    glVertex2f(1000.0, 600.0);
    glVertex2f(1000.0, 0.0);
    glEnd();
    
    glColor3f(1.0f, 1.0f, 1.0f);
    std::string text = "You died.";
    
    int textWidth = glutBitmapLength(GLUT_BITMAP_TIMES_ROMAN_24, (const unsigned char*)text.c_str());
    print(500 - textWidth / 2, 288, const_cast<char*>(text.c_str()), 24);
    
    std::string text2 = "Score: " + std::to_string(score);
    int textWidth2 = glutBitmapLength(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)text2.c_str());
    print(500 - textWidth2 / 2, 320, const_cast<char*>(text2.c_str()), 18);
}

void stopTime() {
    Mix_Chunk* soundEffect = Mix_LoadWAV(audioFilesNames.at(0).c_str());
    if (!soundEffect) {
        std::cerr << "Sound effect not loaded" << std::endl;
    }

    Mix_PlayChannel(2, soundEffect, 0);
    Mix_Volume(2, 128);
    isTimeStopped = true;
}

void doubleScore() {
    Mix_Chunk* soundEffect = Mix_LoadWAV(audioFilesNames.at(6).c_str());
    if (!soundEffect) {
        std::cerr << "Sound effect not loaded" << std::endl;
    }

    Mix_PlayChannel(2, soundEffect, 0);
    Mix_Volume(2, 128);
    isDoubleScoreActive = true;
    scoreIncrease = 2;
}

void playerCollide() {
    const float playerWidth = 40.0;
    int width = 40;
    
    float leftBoundary = objectX - playerWidth;
    float rightBoundary = objectX + playerWidth;
    float topBoundary = objectY - playerWidth;
    float bottomBoundary = objectY + playerWidth;
    
    for (const std::array<float, 2>& takenCenter : takenCenters) {
        
        if (takenCenter[0] == 500 && takenCenter[1] == 300) {
            continue;
        }
        
        float itemLeftBoundary = takenCenter[0] - (width / 2);
        float itemRightBoundary = takenCenter[0] + (width / 2);
        float itemTopBoundary = takenCenter[1] - (width / 2);
        float itemBottomBoundary = takenCenter[1] + (width / 2);
        
        bool collisionX = itemRightBoundary >= leftBoundary && itemLeftBoundary <= rightBoundary;
        bool collisionY = itemBottomBoundary >= topBoundary && itemTopBoundary <= bottomBoundary;
        
        bool isColRemoved = false;
        bool isObstacleCollision = false;
        bool isPowRemoved = false;

        if (collisionX && collisionY) {
            if (takenCenter[0] == 50 && takenCenter[1] == 550 && score >= colCount) {
                won = true;
            }
            isColRemoved = removeFloat2Array(collectibles, takenCenter);
            if (!isColRemoved) {
                isObstacleCollision = checkExists(obstacles, takenCenter);
            }
            if (!isObstacleCollision && !isColRemoved) {
                for (int i = 0; i < powerUps.size(); i++) {
                    float x = powerUps.at(i)[0];
                    float y = powerUps.at(i)[1];
                    float r = powerUps.at(i)[2];
                    if (x == takenCenter[0] && y == takenCenter[1]) {
                        std::array<float, 3> powerUpToRemove = {x, y, r};
                        isPowRemoved = removeFloat3Array(powerUps, powerUpToRemove);
                        if (isPowRemoved) {
                            if (r == 1) {
                                stopTime();
                            } else {
                                doubleScore();
                            }
                        }
                    }
                }
            }
            if (isColRemoved) {
                score += scoreIncrease;
                Mix_Chunk* soundEffect = Mix_LoadWAV(audioFilesNames.at(5).c_str());
                if (!soundEffect) {
                    std::cerr << "Sound effect not loaded" << std::endl;
                }

                Mix_PlayChannel(2, soundEffect, 0);
                Mix_Volume(2, 128);
            } else if (isObstacleCollision) {
                healthPoints --;
                if (lastKey == 'l') {
                    objectX += 10;
                } else if (lastKey == 'r') {
                    objectX -= 10;
                } else if (lastKey == 'u') {
                    objectY += 10;
                } else if (lastKey == 'd') {
                    objectY -= 10;
                }
                isTakingDamage = true;
                Mix_Chunk* soundEffect = Mix_LoadWAV(audioFilesNames.at(4).c_str());
                if (!soundEffect) {
                    std::cerr << "Sound effect not loaded" << std::endl;
                }
                Mix_PlayChannel(2, soundEffect, 0);
                Mix_Volume(2, 128);
            }
            if (!isObstacleCollision) {
                removeFloat2Array(takenCenters, takenCenter);
            }
        }
    }
}

void displayHealthBar() {
    float startX = 10;
    float startY = 50;
    
    float fixedWidth = 350;
    float width = 70 * healthPoints;
    float height = 40;
    
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(startX + fixedWidth, startY);
    glVertex2f(startX, startY);
    glVertex2f(startX, startY - height);
    glVertex2f(startX + fixedWidth, startY - height);
    glEnd();
    
    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(startX + width, startY);
    glVertex2f(startX, startY);
    glVertex2f(startX, startY - height);
    glVertex2f(startX + width, startY - height);
    glEnd();
}

void displayScore() {
    std::string text = "Score: " + std::to_string(score);
    int textWidth = glutBitmapLength(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)text.c_str());
    int x = 1000 - textWidth - 10;
    glColor3f(0.0f, 1.0f, 0.0f);
    print(x, 50, const_cast<char*>(text.c_str()), 18);
}

void displayTime() {
    std::string mins;
    std::string secs;
    if (displayedTime.minutes <= 9) {
        mins = "0" + std::to_string(displayedTime.minutes);
    } else {
        mins = std::to_string(displayedTime.minutes);
    }
    if (displayedTime.seconds <= 9) {
        secs = "0" + std::to_string(displayedTime.seconds);
    } else {
        secs = std::to_string(displayedTime.seconds);
    }
    std::string text = mins + ":" + secs;
    int textWidth = glutBitmapLength(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)text.c_str());
    int x = 1000 - textWidth - 10;
    
    if (timeRemaining <= 15) {
        if (timeRemaining % 2 == 0) {
            glColor3f(1.0f, 0.0f, 0.0f);
        } else {
            glColor3f(0.0f, 0.0f, 0.0f);
        }
    } else {
        glColor3f(0.0f, 0.0f, 0.0f);
    }
    print(x, 28, const_cast<char*>(text.c_str()), 18);
}

void displayDurations() {
    std::string secs1;
    std::string secs2;
    if (timeStopDuration <= 9) {
        secs1 = "0" + std::to_string(timeStopDuration);
    } else {
        secs1 = std::to_string(timeStopDuration);
    }
    
    if (doubleScoreDuration <= 9) {
        secs2 = "0" + std::to_string(doubleScoreDuration);
    } else {
        secs2 = std::to_string(doubleScoreDuration);
    }
    
    std::string text1 = "Time Stop: ";
    std::string text1Secs = secs1 + "s";
    int text1Width = glutBitmapLength(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)text1.c_str());
    
    std::string text2 = "Score Boost: ";
    std::string text2Secs = secs2 + "s";
    int text2Width = glutBitmapLength(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)text2.c_str());
    
    glColor3f(0.0f, 0.0f, 0.0f);
    print(15, 78, const_cast<char*>(text1.c_str()), 12);
    if (isTimeStopped) {
        glColor3f(0.0, 1.0, 0.0);
    }
    if (isTimeStopped && timeStopDuration < 5) {
        if (timeStopDuration % 2 == 0) {
            glColor3f(1.0f, 0.0f, 0.0f);
        } else {
            glColor3f(0.0f, 0.0f, 0.0f);
        }
    }
    print(15 + text1Width, 78, const_cast<char*>(text1Secs.c_str()), 12);
    
    glColor3f(0.0f, 0.0f, 0.0f);
    print(15, 95, const_cast<char*>(text2.c_str()), 12);
    if (isDoubleScoreActive) {
        glColor3f(0.0, 1.0, 0.0);
    }
    if (isDoubleScoreActive && doubleScoreDuration < 5) {
        if (doubleScoreDuration % 2 == 0) {
            glColor3f(1.0f, 0.0f, 0.0f);
        } else {
            glColor3f(0.0f, 0.0f, 0.0f);
        }
    }
    print(15 + text2Width, 95, const_cast<char*>(text2Secs.c_str()), 12);
}


void displayStats() {
    displayHealthBar();
    displayDurations();
    displayScore();
    displayTime();
}

void displayGoal() {
    glLineWidth(2.0);
    float centerX = 50;
    float centerY = 550;
    int numSegments = 100;
    
    // Earth
    glColor3f(0.0f, 0.2f, 1.0f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < numSegments; i++) {
        float angle = i * 2 * 3.14159265 / numSegments;
        float x = centerX + 20 * cos(angle);
        float y = centerY + 20 * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();
    
    glPushMatrix();
    glTranslatef(earthRotation1, 0.0, 0.0);
    
    glColor3f(0.0f, 1.0f, 0.0f);
    
    // Continent 1
    if (centerX + earthRotation1 + removed < centerX + 20) {
        glBegin(GL_QUADS);
        glVertex2f(centerX + removed, centerY - 10);
        glVertex2f(centerX + removed, centerY);
        glVertex2f(centerX + 7.5, centerY);
        glVertex2f(centerX + 7.5, centerY - 10);
        glEnd();
    } else {
        removed = 0.0f;
        earthRotation1 = 0.0f;
    }
    
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(earthRotation2, 0.0, 0.0);
    
    // Continent 2
    if (centerX + earthRotation2 + removed2 - 12 < centerX + 20) {
        glBegin(GL_QUADS);
        glVertex2f(centerX - 12 + removed2, centerY + 12);
        glVertex2f(centerX - 12 + removed2, centerY + 5);
        glVertex2f(centerX + 12, centerY + 5);
        glVertex2f(centerX + 12, centerY + 12);
        glEnd();
    } else {
        removed2 = 0.0f;
        earthRotation2 = 0.0f;
    }
    
    glPopMatrix();
    
    float a;
    float b;

    // Ring 1
    a = 30.0;
    b = 15.0;
    glColor3f(0.5686f, 0.4314f, 0.3059f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < numSegments; i++) {
        float angle = 2.0 * M_PI * i / numSegments;
        float x = centerX + a * cos(angle);
        float y = centerY + b * sin(angle);
        if (y <= centerY - 13) {
            glColor3f(0.0f, 0.2f, 1.0f);
        }
        glVertex2f(x, y);
        glColor3f(0.5686f, 0.4314f, 0.3059f);
    }
    glEnd();
    
    // Ring 1
    a = 34.0;
    b = 19.0;
    glColor3f(0.5686f, 0.4314f, 0.3059f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < numSegments; i++) {
        float angle = 2.0 * M_PI * i / numSegments;
        float x = centerX + a * cos(angle);
        float y = centerY + b * sin(angle);
        glVertex2f(x, y);
        glColor3f(0.5686f, 0.4314f, 0.3059f);
    }
    glEnd();
    
    glPopMatrix();

}

void drawStar(float xCenter, float yCenter) {
    float radius = 2.0f;
    int numSegments = 100;
    
    glColor3f(0.0, 0.0, 0.0);
    glLineWidth(0.4);
    
    glBegin(GL_POLYGON);
    for (int i = 0; i <= numSegments; i++) {
        float angle = i * 2 * 3.14159265 / numSegments;
        float x = xCenter + radius * 2 * cos(angle);
        float y = yCenter + radius * 2 * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();
    
    // top
    glBegin(GL_TRIANGLES);
    glVertex2f(xCenter - radius, yCenter);
    glVertex2f(xCenter + radius, yCenter);
    glVertex2f(xCenter, yCenter - twinkleLength);
    glEnd();
    // bottom
    glBegin(GL_TRIANGLES);
    glVertex2f(xCenter - radius, yCenter);
    glVertex2f(xCenter + radius, yCenter);
    glVertex2f(xCenter, yCenter + twinkleLength);
    glEnd();
    // left
    glBegin(GL_TRIANGLES);
    glVertex2f(xCenter, yCenter - radius);
    glVertex2f(xCenter, yCenter + radius);
    glVertex2f(xCenter - twinkleLength, yCenter);
    glEnd();
    // right
    glBegin(GL_TRIANGLES);
    glVertex2f(xCenter, yCenter - radius);
    glVertex2f(xCenter, yCenter + radius);
    glVertex2f(xCenter + twinkleLength, yCenter);
    glEnd();
}


void randomizeStars() {
    int min = 12;
    int max = 20;
    float minCordX = 125.0f;
    float maxCordX = 980.0f;
    float minCordY = 68.5f;
    float maxCordY = 570.0f;
    std::uniform_int_distribution<int> dist(min, max);
    std::uniform_real_distribution<float> xCord(minCordX, maxCordX);
    std::uniform_real_distribution<float> yCord(minCordY, maxCordY);
    
    int numStars = dist(generatorStarsNum);

    for (int i = 0; i < numStars; i++) {
        float x = xCord(generatorStarsX);
        float y = yCord(generatorStarsY);
        stars.push_back({x, y});
    }
}

void displayPlayer() {
        
    glPushMatrix();
    glTranslatef(objectX, objectY, 0.0f);
    glRotatef(playerRotation, 0.0f, 0.0f, 1.0f);
    glLineWidth(2.0);

    float centerX = 0.0f;
    float centerY = 0.0f;

    if (isTakingDamage) {
        if (damageDelay <= 0) {
            isTakingDamage = false;
        } else {
            damageDelay --;
        }
        glColor3f(220.0 / 255.0, 74.0 / 255.0, 59.0 / 255.0);
    } else {
        glColor3f(0.9255f, 0.6824f, 0.4784f);
    }
    glBegin(GL_QUADS);
    glVertex2f(centerX - 40.0, centerY - 20.0);
    glVertex2f(centerX + 40.0, centerY - 20.0);
    glVertex2f(centerX + 40.0, centerY + 40.0);
    glVertex2f(centerX - 40.0, centerY + 40.0);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    int numSegments = 100;

    // Left eye
    glBegin(GL_POLYGON);
    for (int i = 0; i < numSegments; i++) {
        float angle = i * 2 * 3.14159265 / numSegments;
        float x = centerX - 20.0 + 6 * cos(angle);
        float y = centerY + 20 - 7 + 6 * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();
    
    // Right eye
    glBegin(GL_POLYGON);
    for (int i = 0; i < numSegments; i++) {
        float angle = i * 2 * 3.14159265 / numSegments;
        float x = centerX + 20.0 + 6 * cos(angle);
        float y = centerY + 20 - 7 + 6 * sin(angle);
        glVertex2f(x, y);
    }

    glEnd();

    if (isTimeStopped || isDoubleScoreActive || won) {
        eyeColor = 1.0f;
    } else {
        eyeColor = 0.0f;
    }
    
    glColor3f(eyeColor, 0.0f, 0.0f);

    // Left eye
    glBegin(GL_POLYGON);
    for (int i = 0; i < numSegments; i++) {
        float angle = i * 2 * 3.14159265 / numSegments;
        float x = centerX - 20.0 + 3 * cos(angle);
        float y = centerY + 20 - 7 + 3 * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();
    
    // Right eye
    glBegin(GL_POLYGON);
    for (int i = 0; i < numSegments; i++) {
        float angle = i * 2 * 3.14159265 / numSegments;
        float x = centerX + 20.0 + 3 * cos(angle);
        float y = centerY + 20 - 7 + 3 * sin(angle);
        glVertex2f(x, y);
    }

    glEnd();


    glColor3ub(22, 51, 27);
    glLineWidth(4.0);

    // Mouth lines
    glBegin(GL_LINES);
    glVertex2f(centerX - 15.0, centerY + 32.0);
    glVertex2f(centerX + 15.0, centerY + 32.0);
    glEnd();

    glBegin(GL_LINES);
    glVertex2f(centerX - 20.0, centerY + 29.0);
    glVertex2f(centerX - 15.0, centerY + 32.0);
    glEnd();

    
    glBegin(GL_LINES);
    glVertex2f(centerX + 15.0, centerY + 32.0);
    glVertex2f(centerX + 20.0, centerY + 29.0);
    glEnd();

    glColor3ub(0, 0, 0);
    glLineWidth(2.0);

    // left eyebrow
    glBegin(GL_LINES);
    glVertex2f((centerX - 20) - 10, centerY + 20 - 25.0);
    glVertex2f((centerX - 20) + 10, centerY + 20 - 15.0);
    glEnd();
    
    // right eyebrow
    glBegin(GL_LINES);
    glVertex2f((centerX + 20) - 10, centerY + 20 - 15.0);
    glVertex2f((centerX + 20) + 10, centerY + 20 - 25.0);
    glEnd();

    if (isTimeStopped || isDoubleScoreActive || won) {
        glColor3f(0.7, 0.5, 0.2);
        
        // Middle left triangle
        glBegin(GL_TRIANGLES);
        glVertex2f(centerX - 20.0, centerY - 20.0);
        glVertex2f(centerX - 10.0, centerY - 48.0);
        glVertex2f(centerX, centerY - 20.0);
        glEnd();
        
        // Middle right triangle
        glBegin(GL_TRIANGLES);
        glVertex2f(centerX + 20.0, centerY - 20.0);
        glVertex2f(centerX + 10.0, centerY - 48.0);
        glVertex2f(centerX, centerY - 20.0);
        glEnd();
        
    } else {
        glColor3f(0.81568627451, 0.62745098039, 0.27058823529);
    }
    // Left triangle
    glBegin(GL_TRIANGLES);
    glVertex2f(centerX - 40.0, centerY - 20.0);
    glVertex2f(centerX - 30.0, centerY - 40.0);
    glVertex2f(centerX - 20.0, centerY - 20.0);
    glEnd();

    // Middle triangle
    glBegin(GL_TRIANGLES);
    glVertex2f(centerX - 20.0, centerY - 20.0);
    glVertex2f(centerX, centerY - 50.0);
    glVertex2f(centerX + 20, centerY - 20.0);
    glEnd();

    // Right triangle
    glBegin(GL_TRIANGLES);
    glVertex2f(centerX + 20.0, centerY - 20.0);
    glVertex2f(centerX + 30.0, centerY - 40.0);
    glVertex2f(centerX + 40.0, centerY - 20.0);
    glEnd();
    
    // Left top triangle
    glBegin(GL_TRIANGLES);
    glVertex2f(centerX - 30.0, centerY - 20.0);
    glVertex2f(centerX - 20.0, centerY - 45.0);
    glVertex2f(centerX - 10.0, centerY - 20.0);
    glEnd();

    // right top triangle
    glBegin(GL_TRIANGLES);
    glVertex2f(centerX + 10.0, centerY - 20.0);
    glVertex2f(centerX + 20.0, centerY - 45.0);
    glVertex2f(centerX + 30.0, centerY - 20.0);
    glEnd();
    
    glPopMatrix();
}

void displayCollectible(float centerX, float centerY, float radius) {
    glPushMatrix();
    glTranslatef(centerX, centerY, 0.0f);
    glRotatef(colRotation, 0.0f, 0.1f, 1.0f);
    glTranslatef(-centerX, -centerY, 0.0f);
    int numSegments = 100;

    glLineWidth(2.0);
    
    glColor3f(1.0f, 0.0f, 0.0f);
    
    // outline
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i <= numSegments; i++) {
        float angle = i * 2 * 3.14159265 / numSegments;
        float x = centerX + radius * 2 * cos(angle);
        float y = centerY - radius * 2 * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();

    // Semicircle
    glBegin(GL_POLYGON);
    for (int i = 0; i <= 180; i++) {
        float angle = i * M_PI / 180.0;
        float x = centerX + radius * cos(angle);
        float y = centerY + radius * sin(angle);
        glVertex2f(x, y + 4);
    }
    glEnd();

    // Triangle
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(centerX, centerY - 15);
    glVertex2f(centerX - 10, centerY + 4);
    glVertex2f(centerX + 10, centerY + 4);
    glEnd();
    
    glPopMatrix();
}

void displayObstacles(float centerX, float centerY, float radius) {
    
    glPushMatrix();
    glTranslatef(centerX, centerY, 0.0f);
    glRotatef(ObstacleRotation, 0.0f, 0.0f, 1.0f);
    glTranslatef(-centerX, -centerY, 0.0f);
    
    glLineWidth(2.0);

    glColor3f(208 / 255.0, 160 / 255.0, 69 / 255.0);
    glBegin(GL_POLYGON);

    int numSegments = 100;

    for (int i = 0; i < numSegments; i++) {
        float angle = i * 2 * 3.14159265 / numSegments;
        float x = centerX + radius * cos(angle);
        float y = centerY - radius * sin(angle);
        glVertex2f(x, y);
    }

    glEnd();

    glColor3f(162 / 255.0, 122 / 255.0, 145 / 255.0);
    glLineWidth(4.0);

    glBegin(GL_LINE_LOOP);

    int numPoints = 5;
    float angle = -3.14159265 / 2;
    
    for (int i = 0; i < numPoints * 2; i++) {
        float x, y;
        if (i % 2 == 0) {
            x = centerX + (radius-3) * cos(angle);
            y = centerY + (radius-3) * sin(angle);
        } else {
            x = centerX + ((radius-3) / 2) * cos(angle);
            y = centerY + ((radius-3) / 2) * sin(angle);
        }
        glVertex2f(x, y);
        angle += 2 * 3.14159265 / (numPoints * 2);
    }

    glEnd();
    
    // Outline
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < numSegments; i++) {
        float angle = i * 2 * 3.14159265 / numSegments;
        float x = centerX + radius * cos(angle);
        float y = centerY + radius * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();
    
    glPopMatrix();
}

void displayPowerUps(float centerX, float centerY, float radius) {
    float functionality;
    if (isRandom) {
        int min = 1;
        int max = 2;
        std::uniform_int_distribution<int> distribution(min, max);
        functionality = distribution(generator);
    } else {
        if (fn == 1) {
            functionality = fn;
            fn = 2;
        } else {
            functionality = fn;
            isRandom = true;
        }
    }
    powerUps.push_back({centerX, centerY, functionality});
}

void displayZaWarudo(float centerX, float centerY, float radius) {
    glPushMatrix();
    glTranslatef(centerX, centerY, 0.0f);
    glRotatef(colRotation, 0.0f, 0.0f, 1.0f);
    glTranslatef(-centerX, -centerY, 0.0f);
    
    int numSegments = 100;
    // Circle
    glColor3f(162 / 255.0, 122 / 255.0, 145 / 255.0);
    glBegin(GL_POLYGON);
    for (int i = 0; i < numSegments; i++) {
        float angle = i * 2 * 3.14159265 / numSegments;
        float x = centerX + radius * cos(angle);
        float y = centerY - radius * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();
    
    // Outline
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < numSegments; i++) {
        float angle = i * 2 * 3.14159265 / numSegments;
        float x = centerX + radius * cos(angle);
        float y = centerY + radius * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();
    
    // Pause Icon
    glColor3f(1.0f, 1.0f, 1.0f);
    // Left rectangle
    glBegin(GL_QUADS);
    glVertex2f(centerX - 8, centerY - 8); // right top
    glVertex2f(centerX - 8, centerY + 8); // right bottom
    glVertex2f(centerX - 3, centerY + 8); // left bottom
    glVertex2f(centerX - 3, centerY - 8); // left top
    glEnd();
    
    // Right rectangle
    glBegin(GL_QUADS);
    glVertex2f(centerX + 3, centerY - 8);
    glVertex2f(centerX + 3, centerY + 8);
    glVertex2f(centerX + 8, centerY + 8);
    glVertex2f(centerX + 8, centerY - 8);
    glEnd();
    
    glPopMatrix();
}

void displayDoubleScore(float centerX, float centerY, float radius) {
    glPushMatrix();
    glTranslatef(centerX, centerY, 0.0f);
    glRotatef(colRotation, 0.0f, 0.0f, 1.0f);
    glTranslatef(-centerX, -centerY, 0.0f);
    
    glLineWidth(2.0);

    // Outline
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_LINE_LOOP);
    int numSegments = 100;
    for (int i = 0; i < numSegments; i++) {
        float angle = i * 2 * 3.14159265 / numSegments;
        float x = centerX + radius * cos(angle);
        float y = centerY + radius * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();

    // Rect
    float width = radius / 2;
    float height = radius / 2 + 5;
    glBegin(GL_QUADS);
    glVertex2f(centerX - width / 2, centerY + height / 2 + 5);
    glVertex2f(centerX + width / 2, centerY + height / 2 + 5);
    glVertex2f(centerX + width / 2, centerY - height / 2 + 5);
    glVertex2f(centerX - width / 2, centerY - height / 2 + 5);
    glEnd();

    // Triangle
    glBegin(GL_TRIANGLES);
    glVertex2f(centerX + width, centerY + height / 2 - 7.5);
    glVertex2f(centerX - width, centerY + height / 2 - 7.5);
    glVertex2f(centerX, centerY - height / 2 - 7.5);
    glEnd();
    
    // Circle
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_POLYGON);
    for (int i = 0; i < numSegments; i++) {
        float angle = i * 2 * 3.14159265 / numSegments;
        float x = centerX + 2 * cos(angle);
        float y = centerY - 2 * sin(angle);
        glVertex2f(x, y - 5);
    }
    glEnd();
    
    glPopMatrix();
}

void randomize() {
    int min = 5;
    int max = 9;
    int minPU = 2;
    int maxPU = 3;
    float minX = 100.0f;
    float maxX = 950.0f;
    float minY = 120.0f;
    float maxY = 550.0f;
    float minXObstacles = 200.0f;
    float maxXObstacles = 800.0f;
    float minYObstacles = 200.0f;
    float maxYObstacles = 550.0f;
    
    std::uniform_int_distribution<int> distribution(min, max);
    std::uniform_real_distribution<float> distributionX(minX, maxX);
    std::uniform_real_distribution<float> distributionY(minY, maxY);
    std::uniform_real_distribution<float> distributionXObstacles(minXObstacles, maxXObstacles);
    std::uniform_real_distribution<float> distributionYObstacles(minYObstacles, maxYObstacles);
    std::uniform_int_distribution<int> distributionPowerUps(minPU, maxPU);
    
    int randomCollectibles = distribution(generator);
    int randomObstacles = distribution(generator);
    int randomPowerUps = distributionPowerUps(generator);
    
    colCount = randomCollectibles;
    
    for (int i = 0; i < randomCollectibles; i++) {
        float randomX, randomY;
        do {
            randomX = distributionX(generatorX);
            randomY = distributionY(generatorY);
        } while (isTaken({randomX, randomY}));
        
        collectibles.push_back({randomX, randomY});
        takenCenters.push_back({randomX, randomY});
    }

    for (int i = 0; i < randomObstacles; i++) {
        float randomX, randomY;
        do {
            randomX = distributionXObstacles(generatorX);
            randomY = distributionYObstacles(generatorY);
        } while (isTaken({randomX, randomY}));

        obstacles.push_back({randomX, randomY});
        takenCenters.push_back({randomX, randomY});
    }
    
    for (int i = 0; i < randomPowerUps; i++) {
        float randomX, randomY;
        do {
            randomX = distributionX(generatorX);
            randomY = distributionY(generatorY);
        } while (isTaken({randomX, randomY}));

        displayPowerUps(randomX, randomY, 20.0);
        takenCenters.push_back({randomX, randomY});
    }

}

void displayItems() {
    for (const std::array<float, 2>& center : stars) {
        drawStar(center[0], center[1]);
    }
    for (const std::array<float, 2>& center : collectibles) {
        displayCollectible(center[0], center[1], 10.0);
    }
    for (const std::array<float, 2>& center : obstacles) {
        displayObstacles(center[0], center[1], 20.0);
    }
    for (const std::array<float, 3>& center : powerUps) {
        if (center[2] == 1) {
            displayZaWarudo(center[0], center[1], 20.0);
        } else {
            displayDoubleScore(center[0], center[1], 20.0);
        }
    }
}

void displayBoundaries() {
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(1.5f);
    
    // top
    glBegin(GL_LINES);
    glVertex2f(10, 60);
    glVertex2f(990, 60);
    glEnd();
    glBegin(GL_LINES);
    glVertex2f(10, 58.5);
    glVertex2f(990, 58.5);
    glEnd();
    
    // left
    glBegin(GL_LINES);
    glVertex2f(10, 60);
    glVertex2f(10, 590);
    glEnd();
    glBegin(GL_LINES);
    glVertex2f(8.5, 60);
    glVertex2f(8.5, 590);
    glEnd();
    
    // bottom
    glBegin(GL_LINES);
    glVertex2f(10, 590);
    glVertex2f(990, 590);
    glEnd();
    glBegin(GL_LINES);
    glVertex2f(10, 591.5);
    glVertex2f(990, 591.5);
    glEnd();
    
    // right
    glBegin(GL_LINES);
    glVertex2f(990, 60);
    glVertex2f(990, 590);
    glEnd();
    glBegin(GL_LINES);
    glVertex2f(991.5, 60);
    glVertex2f(991.5, 590);
    glEnd();
}

void Display() {
    glClear(GL_COLOR_BUFFER_BIT);

    displayItems();
    displayBoundaries();
    if (score >= colCount) {
        displayGoal();
    }
    displayStats();
    displayPlayer();
    
    if (won) {
        gameRunning = false;
        goalReached();
        if (goalEffect) {
            goalEffect = false;
            Mix_HaltMusic();
            Mix_Chunk* soundEffect = Mix_LoadWAV(audioFilesNames.at(7).c_str());
            if (!soundEffect) {
                std::cerr << "Sound effect not loaded" << std::endl;
            }

            Mix_PlayChannel(2, soundEffect, 0);
            Mix_Volume(2, 128);
            
            while (Mix_Playing(2) != 0) {
                SDL_Delay(100);
            }
            
            soundEffect = Mix_LoadWAV(audioFilesNames.at(3).c_str());
            if (!soundEffect) {
                std::cerr << "Sound effect not loaded" << std::endl;
            }

            Mix_PlayChannel(2, soundEffect, 0);
            Mix_Volume(2, 128);
        }
    } else if (timeRemaining <= 0 && healthPoints > 0 && !won) {
        gameRunning = false;
        timeOut();
    } else if (healthPoints <= 0) {
        gameRunning = false;
        playerDied();
    }
    
    if (!gameRunning && !once && !won) {
        Mix_HaltMusic();
        Mix_Music* audio = Mix_LoadMUS(audioFilesNames.at(2).c_str());
        if (!audio) {
            std::cerr << "Loading music failed" << std::endl;
        }
        
        Mix_VolumeMusic(20);
        if (Mix_PlayMusic(audio, -1) == -1) {
            std::cerr << "Playing music failed" << std::endl;
        }
        
        once = true;
    }

    glutSwapBuffers();
    glFlush();
}

void anim() {
    colRotation += direction * 5;
    twinkleLength += direction2 * 0.5;
    
    if (colRotation == 45 || colRotation == -45) {
        direction = -direction;
    }
    if (twinkleLength == 10 || twinkleLength == 16) {
        direction2 = -direction2;
    }
    
    ObstacleRotation += 80;
    
    if (earthRotation1 < 12.5) {
        earthRotation1 += 0.1;
    } else {
        removed += 0.1;
    }
    
    if (earthRotation2 < 8) {
        earthRotation2 += 0.1;
    } else {
        removed2 += 0.1;
    }
    
    glutPostRedisplay();
}

void specialKeys(int key, int x, int y) {
    const float step = 5.0;
    const float playerWidth = 40.0;

    float leftBoundary = 10 + playerWidth;
    float rightBoundary = 990 - playerWidth;
    float topBoundary = 60 + playerWidth;
    float bottomBoundary = 590 - playerWidth;

    switch (key) {
        case GLUT_KEY_LEFT:
            lastKey = 'l';
            if (objectX - step >= leftBoundary) {
                playerRotation = 90.0f;
                objectX -= step;
            } else {
                objectX = leftBoundary + 24;
                healthPoints --;
                isTakingDamage = true;
                Mix_Chunk* soundEffect = Mix_LoadWAV(audioFilesNames.at(4).c_str());
                if (!soundEffect) {
                    std::cerr << "Sound effect not loaded" << std::endl;
                }

                Mix_PlayChannel(2, soundEffect, 0);
                Mix_Volume(2, 128);
            }
            break;
        case GLUT_KEY_RIGHT:
            lastKey = 'r';
            if (objectX + step <= rightBoundary) {
                playerRotation = -90.0f;
                objectX += step;
            } else {
                objectX = rightBoundary - 24;
                healthPoints --;
                isTakingDamage = true;
                Mix_Chunk* soundEffect = Mix_LoadWAV(audioFilesNames.at(4).c_str());
                if (!soundEffect) {
                    std::cerr << "Sound effect not loaded" << std::endl;
                }

                Mix_PlayChannel(2, soundEffect, 0);
                Mix_Volume(2, 128);
            }
            break;
        case GLUT_KEY_UP:
            lastKey = 'u';
            if (objectY - step >= topBoundary) {
                playerRotation = 180.0f;
                objectY -= step;
            } else {
                objectY = topBoundary + 24;
                healthPoints --;
                isTakingDamage = true;
                Mix_Chunk* soundEffect = Mix_LoadWAV(audioFilesNames.at(4).c_str());
                if (!soundEffect) {
                    std::cerr << "Sound effect not loaded" << std::endl;
                }

                Mix_PlayChannel(2, soundEffect, 0);
                Mix_Volume(2, 128);
            }
            break;
        case GLUT_KEY_DOWN:
            lastKey = 'd';
            if (objectY + step <= bottomBoundary) {
                playerRotation = 0.0f;
                objectY += step;
            } else {
                objectY = bottomBoundary - 24;
                healthPoints --;
                isTakingDamage = true;
                Mix_Chunk* soundEffect = Mix_LoadWAV(audioFilesNames.at(4).c_str());
                if (!soundEffect) {
                    std::cerr << "Sound effect not loaded" << std::endl;
                }

                Mix_PlayChannel(2, soundEffect, 0);
                Mix_Volume(2, 128);
            }
            break;
        case 27:
            exit(0);
            break;
    }
    
    playerCollide();
    glutPostRedisplay();
}


int main(int argc, char** argr) {
    glutInit(&argc, argr);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(1000, 600);
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL Initialization error" << std::endl;
            return 1;
        }
        
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL Mixer Initialization error" << std::endl;
        return 1;
    }

    std::vector<std::string> audioFiles = {
        "/assets/theworld.wav",
        "/assets/fnaf.wav",
        "/assets/tbc.wav",
        "/assets/ger.wav",
        "/assets/hurt.wav",
        "/assets/col.wav",
        "/assets/drinking.wav",
        "/assets/mih.wav",
    };
    
    for (int i = 0; i < audioFiles.size(); i++) {
        std::string path = __FILE__ + audioFiles.at(i);
        std::string substring = "P18-52-0792.cpp/";
        size_t found = path.find(substring);
        if (found != std::string::npos) {
            path.erase(found, substring.length());
        }
        audioFilesNames.push_back(path);
    }
    
    Mix_Music* audio = Mix_LoadMUS(audioFilesNames.at(1).c_str());
    if (!audio) {
        std::cerr << "Loading music failed" << std::endl;
    }
    
    Mix_VolumeMusic(5);
    if (Mix_PlayMusic(audio, -1) == -1) {
        std::cerr << "Playing music failed" << std::endl;
    }
    
    int screenWidth = glutGet(GLUT_SCREEN_WIDTH);
    int screenHeight = glutGet(GLUT_SCREEN_HEIGHT);
    int windowX = (screenWidth - 1000) / 2;
    int windowY = (screenHeight - 600) / 2;
    displayedTime = secondsToMinutesAndSeconds(timeRemaining);
    glutInitWindowPosition(windowX, windowY);

    glutCreateWindow("Dio's Crusade");
    glutDisplayFunc(Display);
    glutIdleFunc(anim);
    glutTimerFunc(1000, timer, 0);

    takenCenters.push_back({500,300});
    takenCenters.push_back({50,550});
    randomize();
    randomizeStars();
    glutSpecialFunc(specialKeys);
    glClearColor(0.90, 0.90, 0.90, 1.0);
    glPointSize(9.0);
    gluOrtho2D(0.0, 1000.0, 600.0, 0.0);
    glutMainLoop();

    Mix_CloseAudio();
    SDL_Quit();
    return 0;
}

#pragma clang diagnostic pop
