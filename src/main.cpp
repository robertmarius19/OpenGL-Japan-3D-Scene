//
//  main.cpp
//  OpenGL Project
//

#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.hpp"
#include "Model3D.hpp"
#include "Camera.hpp"
#include "SkyBox.hpp"

#include <iostream>
#include <vector>
#include <random>

// window
int glWindowWidth = 1920;
int glWindowHeight = 1024;
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

// shadows
const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

// matrices
glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;
glm::mat4 lightRotation;

// lightning
glm::vec3 lightDir;
GLuint lightDirLoc;
glm::vec3 lightColor;
GLuint lightColorLoc;

// point lights
const int MAX_POINT_LIGHTS = 6;
glm::vec3 lightPositions[MAX_POINT_LIGHTS];
glm::vec3 lightColors[MAX_POINT_LIGHTS];
int numberOfLights = 0;

GLuint lightConstantLoc, lightLinearLoc, lightQuadraticLoc;


glm::vec3 playerPos = glm::vec3(15.2349f, 5.0f, -0.0230666f);
float playerYaw = -90.0f;
float playerPitch = 0.0f;


gps::Camera myCamera(playerPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
float cameraSpeed = 0.05f;


bool isIntroSequence = true;
float introStartTime = 0.0f;
float introDuration = 7.0f;


glm::vec3 introStartPos = glm::vec3(0.0f, 40.0f, 40.0f);
float introStartYaw = -90.0f;
float introStartPitch = -50.0f;


bool pressedKeys[1024];
float angleY = 0.0f;
GLfloat lightAngle = 0.0f;
bool animateLight = false;
bool isNight = false;
bool firstMouse = true;
float lastX = 400, lastY = 300;
float yaw = -90.0f;
float pitch = 0.0f;
float sensitivity = 0.1f;

gps::Model3D mapScene;
gps::Model3D bonfireModel;
gps::Model3D snakeModel;
gps::Model3D streetLightsModel;
gps::Model3D ae86Model;
gps::Model3D airplaneModel;
gps::SkyBox mySkyBox;


gps::Shader myCustomShader;
gps::Shader skyboxShader;
gps::Shader rainShader;


GLuint shadowMapFBO;
GLuint depthMapTexture;
int fogEnabled = 0;
GLuint fogInitLoc;
int polygonMode = 0;


bool rainEnabled = false;
const int RAIN_COUNT = 5000;
std::vector<glm::vec3> rainVertices;
GLuint rainVAO, rainVBO;
float rainSpeed = 0.6f;
float rainLength = 0.4f;


const int FIRE_COUNT = 1500;
std::vector<glm::vec3> firePositions;
std::vector<float> fireLife;
GLuint fireVAO, fireVBO;

float airplaneAngle = 0.0f;


glm::vec3 bonfirePos = glm::vec3(-9.6416f, 2.3f, -14.405f);

struct BoundingBox {
    glm::vec3 min;
    glm::vec3 max;

    BoundingBox() : min(0.0f), max(0.0f) {}
    BoundingBox(glm::vec3 minPoint, glm::vec3 maxPoint) : min(minPoint), max(maxPoint) {}

    bool containsPoint(glm::vec3 point) const {
        return (point.x >= min.x && point.x <= max.x &&
            point.y >= min.y && point.y <= max.y &&
            point.z >= min.z && point.z <= max.z);
    }


    bool intersects(const BoundingBox& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x &&
            min.y <= other.max.y && max.y >= other.min.y &&
            min.z <= other.max.z && max.z >= other.min.z);
    }
};

std::vector<BoundingBox> streetLightBoxes;
BoundingBox snakeBox;
BoundingBox playerBox;

const float PLAYER_RADIUS = 0.5f;
const float PLAYER_HEIGHT = 2.0f;
const float STREETLIGHT_RADIUS = 0.8f;
const float SNAKE_LENGTH = 3.0f;
const float SNAKE_HEIGHT = 7.0f;
const float SNAKE_WIDTH = 2.0f;

// bounding box street lights
void initStreetLightBoundingBoxes() {
    streetLightBoxes.clear();

    float startX = 7.3f;
    float endX = 15.0f;
    float startZ = -4.2f;
    float endZ = -2.0f;
    float gap = 2.5f;

    for (float x = startX; x <= endX; x += gap) {
        float factor = (x - startX) / (endX - startX);
        float currentZ = startZ + factor * (endZ - startZ);


        glm::vec3 center = glm::vec3(x, 3.0f, currentZ);
        glm::vec3 minPoint = glm::vec3(
            center.x - STREETLIGHT_RADIUS,
            2.0f,
            center.z - STREETLIGHT_RADIUS
        );
        glm::vec3 maxPoint = glm::vec3(
            center.x + STREETLIGHT_RADIUS,
            6.0f,
            center.z + STREETLIGHT_RADIUS
        );

        streetLightBoxes.push_back(BoundingBox(minPoint, maxPoint));
    }
}

// snake boundingbox
void initSnakeBoundingBox() {
    glm::vec3 snakeCenter = glm::vec3(13.8729f, 3.5f, -2.89252f);

    glm::vec3 minPoint = glm::vec3(
        snakeCenter.x - SNAKE_WIDTH / 2.0f,
        2.0f,
        snakeCenter.z - SNAKE_LENGTH / 2.0f
    );
    glm::vec3 maxPoint = glm::vec3(
        snakeCenter.x + SNAKE_WIDTH / 2.0f,
        SNAKE_HEIGHT,
        snakeCenter.z + SNAKE_LENGTH / 2.0f
    );

    snakeBox = BoundingBox(minPoint, maxPoint);
}

// bound boxing update
void updatePlayerBoundingBox(glm::vec3 playerPosition) {
    glm::vec3 minPoint = glm::vec3(
        playerPosition.x - PLAYER_RADIUS,
        playerPosition.y - PLAYER_HEIGHT / 2.0f,
        playerPosition.z - PLAYER_RADIUS
    );
    glm::vec3 maxPoint = glm::vec3(
        playerPosition.x + PLAYER_RADIUS,
        playerPosition.y + PLAYER_HEIGHT / 2.0f,
        playerPosition.z + PLAYER_RADIUS
    );

    playerBox = BoundingBox(minPoint, maxPoint);
}

// street light collisions
bool checkStreetLightCollision(glm::vec3 newPosition) {
    updatePlayerBoundingBox(newPosition);

    for (const auto& lightBox : streetLightBoxes) {
        if (playerBox.intersects(lightBox)) {
            return true;
        }
    }
    return false;
}
//snake collision
bool checkSnakeCollision(glm::vec3 newPosition) {
    updatePlayerBoundingBox(newPosition);
    return playerBox.intersects(snakeBox);
}

bool checkAllCollisions(glm::vec3 newPosition) {
    if (checkStreetLightCollision(newPosition)) {
        return true;
    }

    if (checkSnakeCollision(newPosition)) {
        return true;
    }

    return false;
}

void initFire() {
    firePositions.clear();
    fireLife.clear();
    for (int i = 0; i < FIRE_COUNT; i++) {
        float r = ((rand() % 100) / 100.0f) * 0.4f;
        float angle = ((rand() % 360) / 360.0f) * 6.28f;
        float x = r * cos(angle);
        float z = r * sin(angle);
        float y = ((rand() % 100) / 100.0f) * 1.5f;

        firePositions.push_back(bonfirePos + glm::vec3(x, y, z));
        fireLife.push_back((rand() % 100) / 100.0f);
    }

    glGenVertexArrays(1, &fireVAO);
    glGenBuffers(1, &fireVBO);
    glBindVertexArray(fireVAO);
    glBindBuffer(GL_ARRAY_BUFFER, fireVBO);
    glBufferData(GL_ARRAY_BUFFER, firePositions.size() * sizeof(glm::vec3), &firePositions[0], GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
}

void updateFire() {
    for (int i = 0; i < FIRE_COUNT; i++) {
        firePositions[i].y += 0.02f;
        fireLife[i] -= 0.02f;

        firePositions[i].x += ((rand() % 100) / 5000.0f - 0.01f);
        firePositions[i].z += ((rand() % 100) / 5000.0f - 0.01f);

        if (fireLife[i] < 0.0f) {
            float r = ((rand() % 100) / 100.0f) * 0.3f;
            float angle = ((rand() % 360) / 360.0f) * 6.28f;
            float x = r * cos(angle);
            float z = r * sin(angle);

            firePositions[i] = bonfirePos + glm::vec3(x, 0.0f, z);
            fireLife[i] = 1.0f;
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, fireVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, firePositions.size() * sizeof(glm::vec3), &firePositions[0]);
}
//fire drawing
void drawFire() {
    rainShader.useShaderProgram();

    glUniformMatrix4fv(glGetUniformLocation(rainShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(rainShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(rainShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

    glUniform3f(glGetUniformLocation(rainShader.shaderProgram, "color"), 1.0f, 0.35f, 0.05f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glPointSize(4.0f);

    glBindVertexArray(fireVAO);
    glDrawArrays(GL_POINTS, 0, FIRE_COUNT);

    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void initRain() {
    rainVertices.clear();
    for (int i = 0; i < RAIN_COUNT; i++) {
        float x = (rand() % 80 - 40.0f);
        float y = (rand() % 40 + 5.0f);
        float z = (rand() % 80 - 40.0f);
        rainVertices.push_back(glm::vec3(x, y, z));
        rainVertices.push_back(glm::vec3(x, y + rainLength, z));
    }
    glGenVertexArrays(1, &rainVAO);
    glGenBuffers(1, &rainVBO);
    glBindVertexArray(rainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rainVBO);
    glBufferData(GL_ARRAY_BUFFER, rainVertices.size() * sizeof(glm::vec3), &rainVertices[0], GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
}

void updateRain() {
    if (!rainEnabled) return;
    for (int i = 0; i < rainVertices.size(); i += 2) {
        rainVertices[i].y -= rainSpeed;
        rainVertices[i + 1].y -= rainSpeed;
        if (rainVertices[i].y < -2.0f) {
            float offsetX = (rand() % 60 - 30.0f);
            float offsetZ = (rand() % 60 - 30.0f);
            float newY = 25.0f + (rand() % 10);
            rainVertices[i].y = newY;
            rainVertices[i].x = myCamera.getPosition().x + offsetX;
            rainVertices[i].z = myCamera.getPosition().z + offsetZ;
            rainVertices[i + 1].y = newY + rainLength;
            rainVertices[i + 1].x = rainVertices[i].x;
            rainVertices[i + 1].z = rainVertices[i].z;
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, rainVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, rainVertices.size() * sizeof(glm::vec3), &rainVertices[0]);
}
//rain drawing
void drawRain() {
    if (!rainEnabled) return;
    rainShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(rainShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(rainShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(rainShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
    glUniform3f(glGetUniformLocation(rainShader.shaderProgram, "color"), 0.6f, 0.7f, 0.8f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.5f);
    glBindVertexArray(rainVAO);
    glDrawArrays(GL_LINES, 0, rainVertices.size());
    glDisable(GL_BLEND);
}

GLenum glCheckError_(const char* file, int line) {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
        case GL_INVALID_ENUM: error = "INVALID_ENUM"; break;
        case GL_INVALID_VALUE: error = "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION: error = "INVALID_OPERATION"; break;
        case GL_OUT_OF_MEMORY: error = "OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    glfwGetFramebufferSize(window, &retina_width, &retina_height);
    glViewport(0, 0, retina_width, retina_height);
    projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        fogEnabled = !fogEnabled;
        myCustomShader.useShaderProgram();
        glUniform1i(fogInitLoc, fogEnabled);
        std::cout << "Fog: " << (fogEnabled ? "ON" : "OFF") << std::endl;
    }

    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        rainEnabled = !rainEnabled;
        std::cout << "Rain: " << (rainEnabled ? "ON" : "OFF") << std::endl;
    }

    if (key == GLFW_KEY_J && action == GLFW_PRESS) {
        animateLight = !animateLight;
        std::cout << "Sun Animation: " << (animateLight ? "ON" : "OFF") << std::endl;
    }

    if (key == GLFW_KEY_N && action == GLFW_PRESS) {
        isNight = !isNight;
        std::cout << "Night Mode: " << (isNight ? "ON" : "OFF") << std::endl;
    }

    if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
        polygonMode = (polygonMode + 1) % 3;
        if (polygonMode == 0) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        if (polygonMode == 1) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        if (polygonMode == 2) glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    }

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) pressedKeys[key] = true;
        else if (action == GLFW_RELEASE) pressedKeys[key] = false;
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    if (isIntroSequence) return;

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    yaw += xoffset;
    pitch += yoffset;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    myCamera.rotate(pitch, yaw);
}

bool isValidPosition(glm::vec3 newPos) {
    if (newPos.x < -13.7f) return false;
    if (newPos.x > 20.0f) return false;
    if (newPos.z < -9.0f) return false;
    if (newPos.z > 20.0f) return false;
    return true;
}

void processMovement() {
    //intro cinematic
    if (isIntroSequence) {
        float currentTime = glfwGetTime();
        if (introStartTime == 0.0f) introStartTime = currentTime;

        float elapsedTime = currentTime - introStartTime;
        float t = elapsedTime / introDuration;

        if (t >= 1.0f) {
            isIntroSequence = false;
            myCamera.setPosition(playerPos);
            myCamera.rotate(playerPitch, playerYaw);
            lastX = glWindowWidth / 2.0;
            lastY = glWindowHeight / 2.0;
            glfwSetCursorPos(glWindow, lastX, lastY);
        }
        else {
            float smoothT = t * t * (3.0f - 2.0f * t);

            glm::vec3 currentPos = glm::mix(introStartPos, playerPos, smoothT);
            float currentYaw = glm::mix(introStartYaw, playerYaw, smoothT);
            float currentPitch = glm::mix(introStartPitch, playerPitch, smoothT);

            myCamera.setPosition(currentPos);
            myCamera.rotate(currentPitch, currentYaw);

            return;
        }
    }

    // movement normal
    if (pressedKeys[GLFW_KEY_Q]) angleY -= 1.0f;
    if (pressedKeys[GLFW_KEY_E]) angleY += 1.0f;

    glm::vec3 oldPosition = myCamera.getPosition();
    glm::vec3 newPosition = oldPosition;

    if (pressedKeys[GLFW_KEY_W]) myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
    if (pressedKeys[GLFW_KEY_S]) myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
    if (pressedKeys[GLFW_KEY_A]) myCamera.move(gps::MOVE_LEFT, cameraSpeed);
    if (pressedKeys[GLFW_KEY_D]) myCamera.move(gps::MOVE_RIGHT, cameraSpeed);

    newPosition = myCamera.getPosition();

    if (checkAllCollisions(newPosition)) {

        myCamera.setPosition(oldPosition);
        newPosition = oldPosition;
    }

    // map limits
    if (!isValidPosition(newPosition)) {
        if (newPosition.x > 20.0f) newPosition.x = 20.0f;
        if (newPosition.x < -20.0f) newPosition.x = -20.0f;
        if (newPosition.z > 20.0f) newPosition.z = 20.0f;
        if (newPosition.z < -20.0f) newPosition.z = -20.0f;
        myCamera.setPosition(newPosition);
    }

    // down limit
    if (newPosition.y < 4.5f) {
        newPosition.y = 4.5f;
        myCamera.setPosition(newPosition);
    }
}

bool initOpenGLWindow() {
    if (!glfwInit()) return false;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "Japan PG project", NULL, NULL);
    if (!glWindow) { glfwTerminate(); return false; }
    glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
    glfwSetKeyCallback(glWindow, keyboardCallback);
    glfwSetCursorPosCallback(glWindow, mouseCallback);
    glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwMakeContextCurrent(glWindow);
    glfwSwapInterval(1);
#if not defined (__APPLE__)
    glewExperimental = GL_TRUE;
    glewInit();
#endif
    glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);
    return true;
}

void initOpenGLState() {
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glViewport(0, 0, retina_width, retina_height);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glPointSize(2.0f);
}

void initObjects() {
    std::cout << "Loading Map..." << std::endl;
    mapScene.LoadModel("objects/map/map.obj");

    std::cout << "Loading Bonfire..." << std::endl;
    bonfireModel.LoadModel("objects/bonfire/bonfire.obj");

    std::cout << "Loading Snake..." << std::endl;
    snakeModel.LoadModel("objects/snake/snake.obj");

    std::cout << "Loading Street Lights..." << std::endl;
    streetLightsModel.LoadModel("objects/streetlights/streetlights.obj");

    std::cout << "Loading AE86..." << std::endl;
    ae86Model.LoadModel("objects/ae86/ae86.obj");

    std::cout << "Loading Airplane..." << std::endl;
    airplaneModel.LoadModel("objects/airplane/airplane.obj");

    initRain();
    initFire();
}

void initSkyBox() {
    std::vector<const GLchar*> faces;
    faces.push_back("skybox/right.tga");
    faces.push_back("skybox/left.tga");
    faces.push_back("skybox/top.tga");
    faces.push_back("skybox/bottom.tga");
    faces.push_back("skybox/back.tga");
    faces.push_back("skybox/front.tga");
    mySkyBox.Load(faces);
}

void initShaders() {
    myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
    skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
    rainShader.loadShader("shaders/rain.vert", "shaders/rain.frag");
}

void initLights() {
    // sun
    lightPositions[0] = glm::vec3(0.0f, 20.0f, 0.0f);
    lightColors[0] = glm::vec3(1.0f, 1.0f, 1.0f);

    // bonfire
    lightPositions[1] = glm::vec3(-9.6416f, 3.5f, -14.405f);
    lightColors[1] = glm::vec3(1.0f, 0.3f, 0.01f);

    numberOfLights = 2;

    // dinamic street lights
    float startX = 7.3f;
    float endX = 15.0f;
    float startZ = -4.2f;
    float endZ = -2.0f;
    float gap = 2.5f;

    for (float x = startX; x <= endX; x += gap) {
        if (numberOfLights >= MAX_POINT_LIGHTS) break;
        float factor = (x - startX) / (endX - startX);
        float currentZ = startZ + factor * (endZ - startZ);
        lightPositions[numberOfLights] = glm::vec3(x, 5.0f, currentZ);
        lightColors[numberOfLights] = glm::vec3(1.0f, 0.6f, 0.05f);
        numberOfLights++;
    }
}

void initUniforms() {
    myCustomShader.useShaderProgram();
    model = glm::mat4(1.0f);
    modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    view = myCamera.getViewMatrix();
    viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
    projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    lightDir = glm::vec3(0.5f, 1.0f, 0.5f);
    lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::normalize(lightDir)));
    lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "lightConstant"), 1.0f);
    glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "lightLinear"), 0.2f);
    glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "lightQuadratic"), 0.1f);

    fogInitLoc = glGetUniformLocation(myCustomShader.shaderProgram, "fogInit");
    glUniform1i(fogInitLoc, fogEnabled);
}

void initFBO() {
    glGenFramebuffers(1, &shadowMapFBO);
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix(glm::vec3 lightDirection) {
    glm::vec3 lightPosition = glm::normalize(lightDirection) * 60.0f;
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::mat4 lightView = glm::lookAt(lightPosition, target, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightProjection = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, 0.1f, 200.0f);
    return lightProjection * lightView;
}

void drawObjects(gps::Shader shader, bool depthPass) {
    shader.useShaderProgram();

    // japanese city
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(1.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    mapScene.Draw(shader);

    // bonfire
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-9.6416f, 2.5f, -14.405f));
    model = glm::scale(model, glm::vec3(1.0f));

    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    bonfireModel.Draw(shader);

    // snake 
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(13.8729f, 4.23f, -2.89252f));
    model = glm::scale(model, glm::vec3(30.0f));

    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    snakeModel.Draw(shader);

    // japanese street lights
    for (int i = 2; i < numberOfLights; i++) {
        model = glm::mat4(1.0f);
        glm::vec3 lightPos = lightPositions[i];
        model = glm::translate(model, glm::vec3(lightPos.x, 4.2f, lightPos.z));
        model = glm::rotate(model, glm::radians(65.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.2f));

        glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        if (!depthPass) {
            normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
            glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
        }
        streetLightsModel.Draw(shader);
    }

    // 5. ae86
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-10.01543f, 2.0f, 5.86249f));
    model = glm::scale(model, glm::vec3(1.0f));
    model = glm::rotate(model, glm::radians(150.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    ae86Model.Draw(shader);

    // animatie airplane
    model = glm::mat4(1.0f);

    float planeRadius = 20.0f;
    float planeHeight = 15.0f;
    float angleRad = glm::radians(airplaneAngle);

    float planeX = planeRadius * cos(angleRad);
    float planeZ = planeRadius * sin(angleRad);

    model = glm::translate(model, glm::vec3(planeX, planeHeight, planeZ));
    model = glm::rotate(model, -angleRad + glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(1.0f / 30.0f));

    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    airplaneModel.Draw(shader);
}

void renderScene() {
    if (animateLight) { lightAngle += 0.5f; }
    airplaneAngle += 0.5f;

    if (isNight) {
        lightColors[0] = glm::vec3(0.0f, 0.0f, 0.02f);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    }
    else {
        lightColors[0] = glm::vec3(1.0f, 1.0f, 1.0f);
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    }

    float flicker = 0.8f + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 0.4f));
    lightColors[1] = glm::vec3(1.0f, 0.3f, 0.01f) * flicker;

    glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::vec3 rotatedLightDir = glm::vec3(rotMat * glm::vec4(lightDir, 0.0f));
    rotatedLightDir = glm::normalize(rotatedLightDir);

    updateRain();
    updateFire();

    // shadow pass
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    myCustomShader.useShaderProgram();
    glm::mat4 lightSpaceTrMatrix = computeLightSpaceTrMatrix(rotatedLightDir);
    glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceTrMatrix));

    glm::vec3 lightPos = rotatedLightDir * 60.0f;
    glm::mat4 lView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lProj = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, 0.1f, 200.0f);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(lView));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(lProj));

    glCullFace(GL_FRONT);
    drawObjects(myCustomShader, true);
    glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // render scene
    glViewport(0, 0, retina_width, retina_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    myCustomShader.useShaderProgram();
    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    for (int i = 0; i < numberOfLights; i++) {
        std::string namePos = "pointLightPositions[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, namePos.c_str()), 1, glm::value_ptr(lightPositions[i]));

        std::string nameCol = "pointLightColors[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, nameCol.c_str()), 1, glm::value_ptr(lightColors[i]));
    }
    glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "numberOfLights"), numberOfLights);

    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightDir"), 1, glm::value_ptr(rotatedLightDir));
    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightColor"), 1, glm::value_ptr(lightColors[0]));

    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "viewPos"), 1, glm::value_ptr(myCamera.getPosition()));

    glm::vec3 far1Pos = glm::vec3(-11.8861f, 2.7f, 5.39231f);
    glm::vec3 far2Pos = glm::vec3(-11.2716f, 2.7f, 4.73779f);

    glm::vec3 carCenter = glm::vec3(-10.01543f, 2.0f, 5.86249f);
    glm::vec3 roadDirectionPoint = glm::vec3(-15.0688f, 2.0f, 2.43424f);
    glm::vec3 forwardDir = glm::normalize(roadDirectionPoint - carCenter);
    forwardDir.y = -0.1f;

    float cutOff = glm::cos(glm::radians(12.5f));
    float outerCutOff = glm::cos(glm::radians(17.5f));
    glm::vec3 headlightColor = glm::vec3(5.0f, 5.0f, 0.5f);

    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "spotLight[0].position"), 1, glm::value_ptr(far1Pos));
    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "spotLight[0].direction"), 1, glm::value_ptr(forwardDir));
    glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "spotLight[0].cutOff"), cutOff);
    glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "spotLight[0].outerCutOff"), outerCutOff);
    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "spotLight[0].color"), 1, glm::value_ptr(headlightColor));
    glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "spotLight[0].constant"), 1.0f);
    glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "spotLight[0].linear"), 0.02f);
    glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "spotLight[0].quadratic"), 0.002f);

    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "spotLight[1].position"), 1, glm::value_ptr(far2Pos));
    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "spotLight[1].direction"), 1, glm::value_ptr(forwardDir));
    glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "spotLight[1].cutOff"), cutOff);
    glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "spotLight[1].outerCutOff"), outerCutOff);
    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "spotLight[1].color"), 1, glm::value_ptr(headlightColor));
    glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "spotLight[1].constant"), 1.0f);
    glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "spotLight[1].linear"), 0.02f);
    glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "spotLight[1].quadratic"), 0.002f);

    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 10);
    glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceTrMatrix));

    drawObjects(myCustomShader, false);
    drawRain();
    drawFire();

    mySkyBox.Draw(skyboxShader, view, projection);

}

void cleanup() {
    glDeleteTextures(1, &depthMapTexture);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &shadowMapFBO);
    glDeleteBuffers(1, &rainVBO);
    glDeleteVertexArrays(1, &rainVAO);
    glDeleteBuffers(1, &fireVBO);
    glDeleteVertexArrays(1, &fireVAO);
    glfwDestroyWindow(glWindow);
    glfwTerminate();
}

int main(int argc, const char* argv[]) {
    if (!initOpenGLWindow()) { glfwTerminate(); return 1; }
    initOpenGLState();
    initObjects();
    initShaders();

    initStreetLightBoundingBoxes();
    initSnakeBoundingBox();

    initLights();
    initUniforms();
    initFBO();
    initSkyBox();
    glCheckError();

    while (!glfwWindowShouldClose(glWindow)) {
        processMovement();
        renderScene();
        glfwPollEvents();
        glfwSwapBuffers(glWindow);
    }
    cleanup();
    return 0;
}