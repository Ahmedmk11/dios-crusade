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
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <sndfile.h>
#include <string>
#include <mutex>
#include <thread>

int music = -1;
int effect = -1;
std::mutex musicMutex;
std::mutex effectsMutex;

float objectX = 500.0f;
float objectY = 300.0f;
float playerRotation = 0.0f;

std::default_random_engine generator(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));
std::default_random_engine generatorX(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count() + 1000));
std::default_random_engine generatorY(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));

std::vector<std::array<float, 2>> takenCenters;

std::vector<std::array<float, 2>> collectibles;
std::vector<std::array<float, 2>> obstacles;
std::vector<std::array<float, 3>> powerUps;

struct Time {
    int minutes;
    int seconds;
};

bool notTerminated = true;
bool once = false;
bool goalEffect = true;
bool gameRunning = true;
ALuint sourceMusic = 0;
ALuint sourceEffect = 1;
std::vector<ALuint> audioBuffers;
bool gameover = false;
bool won = false;
int healthPoints = 5;
int score = 0;
int scoreIncrease = 1;
int timeRemaining = 20;
Time displayedTime;
bool isTimeStopped = false;
int timeStopDuration = 10;
int doubleScoreDuration = 25;
bool isDoubleScoreActive = false;
int colCount = 5;

bool isRandom = false;
int fn = 1;

void audioThreadFunc();
bool initializeOpenAL();
void cleanupOpenAL();
bool loadAudioFilesToBuffers(const std::vector<std::string>& filenames);
void playMusic(int index);
void playEffects(int index);
void print(int x, int y, char* string);
Time secondsToMinutesAndSeconds(int totalSeconds);
void timer(int value);
bool removeFloat2Array(std::vector<std::array<float, 2>>& vectorOfArrays, const std::array<float, 2>& targetArray);
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
void displayPlayer();
void displayCollectibles(float centerX, float centerY, float radius);
void displayObstacles(float centerX, float centerY, float radius);
void displayPowerUps(float centerX, float centerY, float radius);
void displayZaWarudo(float centerX, float centerY, float radius);
void displayDoubleScore(float centerX, float centerY, float radius);
void randomize();
void displayBoundaries();
void Display();

void musicThreadFunc() {
    int index = -1;
    while (true) {
        {
            std::lock_guard<std::mutex> lock(musicMutex);
            if (music != -1) {
                index = music;
                music = -1;
            }
        }
        if (index != -1) {
            std::cout << "sourceMusic" << sourceMusic << std::endl;
            playMusic(index);
            index = -1;
        }
        if (music == 2 || music == 3) {
            alSourceStop(0);
            break;
        }
    }
}

void effectsThreadFunc() {
    int index = -1;
    while (true) {
        {
            std::lock_guard<std::mutex> lock(effectsMutex);
            if (effect != -1) {
                index = effect;
                effect = -1;
            }
        }
        if (index != -1) {
            std::cout << "sourceEffect" << sourceEffect << std::endl;
            playEffects(index);
            index = -1;
        }
    }
}

bool initializeOpenAL() {
    ALCdevice* device = alcOpenDevice(NULL);
    if (!device) {
        std::cerr << "Failed to open audio device" << std::endl;
        return false;
    }

    ALCcontext* context = alcCreateContext(device, NULL);
    if (!context) {
        std::cerr << "Failed to create audio context" << std::endl;
        alcCloseDevice(device);
        return false;
    }

    alcMakeContextCurrent(context);

    return true;
}

void cleanupOpenAL() {
    ALCcontext* context = alcGetCurrentContext();
    ALCdevice* device = alcGetContextsDevice(context);

    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(device);
}

bool loadAudioFilesToBuffers(const std::vector<std::string>& filenames) {
    ALCdevice* device = alcOpenDevice(NULL);
    if (!device) {
        std::cerr << "Failed to open audio device" << std::endl;
        return false;
    }

    ALCcontext* context = alcCreateContext(device, NULL);
    if (!context) {
        std::cerr << "Failed to create audio context" << std::endl;
        alcCloseDevice(device);
        return false;
    }

    alcMakeContextCurrent(context);

    for (const std::string& filename : filenames) {
        ALuint buffer;
        alGenBuffers(1, &buffer);

        SF_INFO sfinfo;
        SNDFILE* sndfile = sf_open(filename.c_str(), SFM_READ, &sfinfo);
        if (!sndfile) {
            std::cerr << "Failed to open audio file: " << filename << std::endl;
            alcMakeContextCurrent(NULL);
            alcDestroyContext(context);
            alcCloseDevice(device);
            return false;
        }
        
        ALenum format;
        if (sfinfo.channels == 1) {
            format = AL_FORMAT_MONO16;
        } else if (sfinfo.channels == 2) {
            format = AL_FORMAT_STEREO16;
        } else {
            std::cerr << "Unsupported number of audio channels" << std::endl;
            sf_close(sndfile);
            alcMakeContextCurrent(NULL);
            alcDestroyContext(context);
            alcCloseDevice(device);
            return false;
        }

        ALsizei dataSize = static_cast<ALsizei>(sfinfo.frames) * sfinfo.channels * sizeof(short);
        short* data = new short[dataSize];
        sf_readf_short(sndfile, data, sfinfo.frames);
        sf_close(sndfile);

        alBufferData(buffer, format, data, dataSize, sfinfo.samplerate);

        delete[] data;

        audioBuffers.push_back(buffer);
    }

    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(device);

    return true;
}

void playMusic(int index) {
    if (index >= 0 && index < audioBuffers.size()) {
        ALCdevice* device = alcOpenDevice(NULL);

        if (!device) {
            std::cerr << "Failed to open audio device" << std::endl;
            return;
        }

        // Unqueue and stop the current source
        if (alIsSource(sourceMusic)) {
            ALint queued;
            alGetSourcei(sourceMusic, AL_BUFFERS_QUEUED, &queued);
            while (queued--) {
                ALuint buffer;
                alSourceUnqueueBuffers(sourceMusic, 1, &buffer);
            }
            alSourceStop(sourceMusic);
        }

        ALCcontext* context = alcCreateContext(device, NULL);

        if (!context) {
            std::cerr << "Failed to create audio context" << std::endl;
            alcCloseDevice(device);
            return;
        }

        alcMakeContextCurrent(context);
        alGenSources(1, &sourceMusic);
        alSourcei(sourceMusic, AL_BUFFER, audioBuffers[index]);
        alSourcePlay(sourceMusic);
        alSourcef(sourceMusic, AL_GAIN, 0.08f);
        ALint sourceState;

        do {
            alGetSourcei(sourceMusic, AL_SOURCE_STATE, &sourceState);
        } while (sourceState == AL_PLAYING);
        
    } else {
        std::cerr << "Invalid audio index: " << index << std::endl;
    }
}

void playEffects(int index) {
    if (index >= 0 && index < audioBuffers.size()) {
        ALCdevice* device = alcOpenDevice(NULL);

        if (!device) {
            std::cerr << "Failed to open audio device" << std::endl;
            return;
        }

        // Unqueue and stop the current source
        if (alIsSource(sourceEffect)) {
            ALint queued;
            alGetSourcei(sourceEffect, AL_BUFFERS_QUEUED, &queued);
            while (queued--) {
                ALuint buffer;
                alSourceUnqueueBuffers(sourceEffect, 1, &buffer);
            }
            alSourceStop(sourceEffect);
        }

        ALCcontext* context = alcCreateContext(device, NULL);

        if (!context) {
            std::cerr << "Failed to create audio context" << std::endl;
            alcCloseDevice(device);
            return;
        }

        alcMakeContextCurrent(context);
        alGenSources(1, &sourceEffect);
        alSourcei(sourceEffect, AL_BUFFER, audioBuffers[index]);
        alSourcePlay(sourceEffect);
        alSourcef(sourceEffect, AL_GAIN, 1.0f);
        ALint sourceState;

        do {
            alGetSourcei(sourceEffect, AL_SOURCE_STATE, &sourceState);
        } while (sourceState == AL_PLAYING);

    } else {
        std::cerr << "Invalid audio index: " << index << std::endl;
    }
}



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
    } else if (score < colCount && score != 0) {
        text = "Good Enough..";
    } else if (score == 0) {
        text = "Meh.";
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
    alSourceStop(sourceEffect);
    {
        std::lock_guard<std::mutex> lock(effectsMutex);
        effect = 0;
    }
    isTimeStopped = true;
}

void doubleScore() {
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
        bool isObsRemoved = false;
        bool isPowRemoved = false;

        if (collisionX && collisionY) {
            if (takenCenter[0] == 50 && takenCenter[1] == 550) {
                won = true;
            }
            isColRemoved = removeFloat2Array(collectibles, takenCenter);
            if (!isColRemoved) {
                isObsRemoved = removeFloat2Array(obstacles, takenCenter);
            }
            if (!isObsRemoved && !isColRemoved) {
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
                alSourceStop(sourceEffect);
                {
                    std::lock_guard<std::mutex> lock(effectsMutex);
                    effect = 5;
                }
            } else if (isObsRemoved) {
                healthPoints --;
//                alSourceStop(sourceEffect);
//                {
//                    std::lock_guard<std::mutex> lock(effectsMutex);
//                    effect = 4;
//                }
            }
            removeFloat2Array(takenCenters, takenCenter);
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
    
    std::string text1 = "The World: ";
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
    
    glColor3f(0.0f, 1.0f, 0.0f);
    
    // Continent 1
    glBegin(GL_QUADS);
    glVertex2f(centerX, centerY - 10);
    glVertex2f(centerX, centerY);
    glVertex2f(centerX + 7.5, centerY);
    glVertex2f(centerX + 7.5, centerY - 10);
    glEnd();
    
    // Continent 2
    glBegin(GL_TRIANGLES);
    glVertex2f(centerX - 5 - 6, centerY + 10);
    glVertex2f(centerX - 6, centerY - 5 + 10);
    glVertex2f(centerX + 5 - 6, centerY + 10);
    glEnd();
    
    // Continent 3
    glColor3f(0.6f, 0.4f, 0.2f);
    glPointSize(7.0);
    glBegin(GL_POINTS);
    glVertex2f(centerX + 8, centerY + 8);
    glEnd();
    
    // Ring
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    int numRingSegments = 50;
    for (int i = 0; i < numRingSegments; i++) {
        float angle = i * 2 * 3.14159265 / numRingSegments;
        float x = centerX + 18 * cos(angle);
        float y = centerY + 18 * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();
}

void displayPlayer() {
        
    glPushMatrix();
    glTranslatef(objectX, objectY, 0.0f);
    glRotatef(playerRotation, 0.0f, 0.0f, 1.0f);
    glLineWidth(2.0);

    float centerX = 0.0f;
    float centerY = 0.0f;

    glColor3f(0.9255f, 0.6824f, 0.4784f);
    glBegin(GL_QUADS);
    glVertex2f(centerX - 40.0, centerY - 20.0);
    glVertex2f(centerX + 40.0, centerY - 20.0);
    glVertex2f(centerX + 40.0, centerY + 40.0);
    glVertex2f(centerX - 40.0, centerY + 40.0);
    glEnd();

    glColor3f(208 / 255.0, 160 / 255.0, 69 / 255.0);

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

    glColor3f(1.0f, 0.0f, 0.0f);

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

    
    glPopMatrix();
}

void displayCollectible(float centerX, float centerY, float radius) {
    int numSegments = 100;

    glLineWidth(2.0);
    
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_LINE_LOOP);

    for (int i = 0; i <= numSegments; i++) {
        float angle = i * 2 * 3.14159265 / numSegments;
        float x = centerX + radius * 2 * cos(angle);
        float y = centerY - radius * 2 * sin(angle);
        glVertex2f(x, y);
    }

    glEnd();

    glBegin(GL_POLYGON);

    for (int i = 0; i <= numSegments; i++) {
        float angle = i * 2 * 3.14159265 / numSegments;
        float x = centerX + radius * cos(angle);
        float y = centerY + 10 - radius * sin(angle);
        glVertex2f(x, y);
    }

    glEnd();

    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(centerX - radius, centerY + 10);
    glVertex2f(centerX + radius, centerY + 10);
    glVertex2f(centerX + radius / 2, centerY + 10 - 2 * radius);
    glVertex2f(centerX - radius / 2, centerY + 10 - 2 * radius);
    glEnd();
    
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(centerX - radius / 2, centerY + 10 - 2 * radius);
    glVertex2f(centerX, centerY + 10 - 3 * radius);
    glVertex2f(centerX + radius / 2, centerY + 10 - 2 * radius);
    glEnd();
}

void displayObstacles(float centerX, float centerY, float radius) {
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
    
    // Pause Icon
    glColor3f(1.0f, 1.0f, 1.0f);
    // Left rectangle
    glBegin(GL_QUADS);
    glVertex2f(centerX - radius / 2.5, centerY - radius / 2.5);
    glVertex2f(centerX - radius / 2.5, centerY + radius / 2.5);
    glVertex2f(centerX - radius / 6, centerY + radius / 2.5);
    glVertex2f(centerX - radius / 6, centerY - radius / 2.5);
    glEnd();
    // Right rectangle
    glBegin(GL_QUADS);
    glVertex2f(centerX + radius / 6, centerY - radius / 2.5);
    glVertex2f(centerX + radius / 6, centerY + radius / 2.5);
    glVertex2f(centerX + radius / 2.5, centerY + radius / 2.5);
    glVertex2f(centerX + radius / 2.5, centerY - radius / 2.5);
    glEnd();
    
    // Ring white
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < numSegments; i++) {
        float angle = i * 2 * 3.14159265 / numSegments;
        float x = centerX + (radius - 2) * cos(angle);
        float y = centerY + (radius - 2) * sin(angle);
        glVertex2f(x, y);
    }
    float xStart = centerX + (radius - 2) * cos(0);
    float yStart = centerY + (radius - 2) * sin(0);
    glVertex2f(xStart, yStart);
    glEnd();
    
    // Ring yellow
    glColor3f(1.0f, 0.9451f, 0.2667f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < numSegments; i++) {
        float angle = i * 2 * 3.14159265 / numSegments;
        float x = centerX + (radius - 5) * cos(angle);
        float y = centerY + (radius - 5) * sin(angle);
        glVertex2f(x, y);
    }
    xStart = centerX + (radius - 5) * cos(0);
    yStart = centerY + (radius - 5) * sin(0);
    glVertex2f(xStart, yStart);
    glEnd();
}

void displayDoubleScore(float centerX, float centerY, float radius) {
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

    // Small Circles
    glColor3f(1.0f, 0.0f, 0.0f);
    int numCircles = 3;
    float circleRadius = 1.0f;
    for (int i = 0; i < numCircles; i++) {
        float angle = i * 2 * 3.14159265 / numCircles;
        float x = centerX + (radius - 15) * cos(angle);
        float y = centerY + (radius - 15) * sin(angle);
        glBegin(GL_POLYGON);
        for (int j = 0; j < numSegments; j++) {
            float circleAngle = j * 2 * 3.14159265 / numSegments;
            float circleX = x + circleRadius * cos(circleAngle);
            float circleY = y + circleRadius * sin(circleAngle);
            glVertex2f(circleX, circleY);
        }
        glEnd();
    }
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

    displayBoundaries();
    displayPlayer();
    displayGoal();
    displayStats();
    displayItems();
    
    if (won) {
        gameRunning = false;
        goalReached();
        if (goalEffect) {
            goalEffect = false;
            alSourceStop(sourceEffect);
            {
                std::lock_guard<std::mutex> lock(effectsMutex);
                effect = 3;
            }
        }
    } else if (timeRemaining <= 0 && healthPoints > 0 && !won) {
        gameRunning = false;
        timeOut();
    } else if (healthPoints <= 0) {
        gameRunning = false;
        playerDied();
    }
    
    if (!gameRunning && !once) {
        bool FLAG = alIsSource(0);
        std::cout << FLAG << std::endl;
        alSourceStop(0);
        std::cout << FLAG << std::endl;
        alSourceStop(sourceEffect);
        {
            std::lock_guard<std::mutex> lock(effectsMutex);
            music = 2;
        }
        {
            std::lock_guard<std::mutex> lock(effectsMutex);
            effect = 2;
        }

        once = true;
    }


    glutSwapBuffers();
    glFlush();
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
            if (objectX - step >= leftBoundary) {
                playerRotation = 90.0f;
                objectX -= step;
            } else {
                objectX = leftBoundary + 24;
                healthPoints --;
//                alSourceStop(sourceEffect);
//                {
//                    std::lock_guard<std::mutex> lock(effectsMutex);
//                    effect = 4;
//                }
            }
            break;
        case GLUT_KEY_RIGHT:
            if (objectX + step <= rightBoundary) {
                playerRotation = -90.0f;
                objectX += step;
            } else {
                objectX = rightBoundary - 24;
                healthPoints --;
//                alSourceStop(sourceEffect);
//                {
//                    std::lock_guard<std::mutex> lock(effectsMutex);
//                    effect = 4;
//                }
            }
            break;
        case GLUT_KEY_UP:
            if (objectY - step >= topBoundary) {
                playerRotation = 180.0f;
                objectY -= step;
            } else {
                objectY = topBoundary + 24;
                healthPoints --;
//                alSourceStop(sourceEffect);
//                {
//                    std::lock_guard<std::mutex> lock(effectsMutex);
//                    effect = 4;
//                }
            }
            break;
        case GLUT_KEY_DOWN:
            if (objectY + step <= bottomBoundary) {
                playerRotation = 0.0f;
                objectY += step;
            } else {
                objectY = bottomBoundary - 24;
                healthPoints --;
//                alSourceStop(sourceEffect);
//                {
//                    std::lock_guard<std::mutex> lock(effectsMutex);
//                    effect = 4;
//                }
            }
            break;
    }
    
    playerCollide();
    glutPostRedisplay();
}


int main(int argc, char** argr) {
    glutInit(&argc, argr);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(1000, 600);
    
    if (!initializeOpenAL()) {
        std::cerr << "Failed to initialize OpenAL" << std::endl;
        return 1;
    }
    
    std::thread musicThread(musicThreadFunc);
    std::thread effectsThread(effectsThreadFunc);

    std::vector<std::string> audioFiles = {
        "/assets/zawarudo.wav",
        "/assets/giorno's_theme.wav",
        "/assets/to_be_continued.wav",
        "/assets/pillermen.wav",
        "/assets/hurt.wav",
        "/assets/col.wav",
    };
    
    std::vector<std::string> audioFilesNames;
    
    for (int i = 0; i < audioFiles.size(); i++) {
        std::string path = __FILE__ + audioFiles.at(i);
        std::string substring = "main.cpp/";
        size_t found = path.find(substring);
        if (found != std::string::npos) {
            path.erase(found, substring.length());
        }
        audioFilesNames.push_back(path);
    }
    
    std::cout << "Audio files loaded successfully." << std::endl;
    
    if (loadAudioFilesToBuffers(audioFilesNames)) {
        std::cout << "Audio files loaded successfully." << std::endl;
    } else {
        std::cerr << "Failed to load audio files." << std::endl;
    }
    
    int screenWidth = glutGet(GLUT_SCREEN_WIDTH);
    int screenHeight = glutGet(GLUT_SCREEN_HEIGHT);
    int windowX = (screenWidth - 1000) / 2;
    int windowY = (screenHeight - 600) / 2;
    displayedTime = secondsToMinutesAndSeconds(timeRemaining);
    glutInitWindowPosition(windowX, windowY);

    glutCreateWindow("Dio's Crusade");
    {
        std::lock_guard<std::mutex> lock(musicMutex);
        music = 1;
    }
    glutDisplayFunc(Display);
    glutTimerFunc(1000, timer, 0);

    takenCenters.push_back({500,300});
    takenCenters.push_back({50,550});
    randomize();
    glutSpecialFunc(specialKeys);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glPointSize(9.0);
    gluOrtho2D(0.0, 1000.0, 600.0, 0.0);
    glutMainLoop();

    musicThread.join();
    effectsThread.join();
    cleanupOpenAL();
    return 0;
}

#pragma clang diagnostic pop
