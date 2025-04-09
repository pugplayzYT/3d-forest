#define GLM_ENABLE_EXPERIMENTAL
#include <glad/glad.h> // Must be included before GLFW
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp> // For pi()
#include <glm/gtx/norm.hpp>     // Include for glm::length2 (squared length/distance)
#include <glm/gtx/string_cast.hpp> // For printing vectors (debugging)


#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cstdlib> // For rand() and srand()
#include <ctime>   // For time()
#include <cstdio>  // For sscanf
#include <cmath>   // For std::sqrt, std::abs

// --- Platform Specific - Include Win32 API ---
#ifdef _WIN32 // Only include windows.h on Windows
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <CommCtrl.h> // Required for checkbox state checking
#pragma comment(lib, "Comctl32.lib") // Link against Comctl32.lib for IsDlgButtonChecked
#endif

// --- Configuration ---
const unsigned int INITIAL_SCR_WIDTH = 1280; // Initial width
const unsigned int INITIAL_SCR_HEIGHT = 720; // Initial height
const float GROUND_SIZE = 500.0f;
const int TREE_COUNT = 800;
const int BUSH_COUNT = 1500; // Bushes currently don't have collision
const int HOUSE_COUNT = 50;
const int APARTMENT_TOWER_COUNT = 25; // Number of apartment towers

// --- Physics & Player ---
const float GRAVITY = 9.81f * 2.0f; // Adjusted gravity strength
const float JUMP_FORCE = 8.0f;
const float PLAYER_EYE_HEIGHT = 1.7f; // How high the camera is off the player's feet
const float PLAYER_RADIUS = 0.3f;     // Player's collision radius for horizontal checks
const float GROUND_LEVEL = -0.45f;    // Y-coordinate of the ground surface (-0.5f center + 0.1f/2 scale)
const float PLAYER_BASE_SPEED = 5.0f; // *** Base walking/flying speed ***
const float SPRINT_MULTIPLIER = 1.8f; // *** Speed multiplier when sprinting/flying faster ***
const float FLY_VERTICAL_SPEED = 4.0f; // Speed for moving up/down in fly mode

// --- Object Dimensions for Collision & Rendering ---
const float TREE_TRUNK_RADIUS = 0.25f; // Half the scale width of the trunk (0.5f / 2)
const float TREE_TRUNK_HEIGHT = 2.0f; // Scale height of the trunk
const float HOUSE_BODY_WIDTH = 4.0f;
const float HOUSE_BODY_DEPTH = 5.0f;
const float HOUSE_BODY_HEIGHT = 3.0f; // Used for potential future vertical collision
const float TOWER_WIDTH = 8.0f;
const float TOWER_DEPTH = 8.0f;
const float TOWER_HEIGHT = 40.0f; // Significantly taller than houses

// --- NEW: Balcony Dimensions ---
const float BALCONY_WIDTH = 2.5f;
const float BALCONY_DEPTH = 1.5f;
const float BALCONY_FLOOR_HEIGHT = 0.2f; // Thickness of the balcony floor
const float BALCONY_RAILING_HEIGHT = 0.8f;
const float BALCONY_RAILING_THICKNESS = 0.1f;
const int   BALCONIES_PER_TOWER = 3; // How many balconies per tower

// --- NEW: Sun Configuration ---
const float SUN_DISTANCE_FACTOR = 0.7f; // How far out relative to ground size
const float SUN_HEIGHT_FACTOR = 0.6f;   // How high relative to ground size
const float SUN_SIZE = 30.0f;           // Scale factor for the sun cube
const glm::vec3 SUN_COLOR = glm::vec3(1.0f, 0.95f, 0.7f); // Bright yellowish color
const glm::vec3 SUN_POSITION = glm::vec3(GROUND_SIZE * SUN_DISTANCE_FACTOR, GROUND_SIZE * SUN_HEIGHT_FACTOR, -GROUND_SIZE * SUN_DISTANCE_FACTOR); // Fixed position

// --- Camera ---
glm::vec3 cameraPos = glm::vec3(0.0f, GROUND_LEVEL + PLAYER_EYE_HEIGHT, 3.0f); // Start on the ground
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraVelocityY = 0.0f; // Player's vertical velocity (used in normal mode)
bool isOnGround = true;       // Is the player currently touching the ground? (used in normal mode)

bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = INITIAL_SCR_WIDTH / 2.0f;
float lastY = INITIAL_SCR_HEIGHT / 2.0f;
float fov = 45.0f;

// --- Timing ---
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// --- Fullscreen State ---
bool isFullscreen = false;
bool f11KeyPressedLastFrame = false;
int lastWindowPosX = 100, lastWindowPosY = 100;
int lastWindowWidth = INITIAL_SCR_WIDTH, lastWindowHeight = INITIAL_SCR_HEIGHT;

// --- Sky Color ---
glm::vec3 skyColor = glm::vec3(0.5f, 0.8f, 0.95f); // Default sky blue

// --- Object Positions (Global for Collision Checks) ---
std::vector<glm::vec3> treePositions;
std::vector<glm::vec3> bushPositions;
std::vector<glm::vec3> housePositions;
std::vector<glm::vec3> apartmentTowerPositions; // Vector for tower positions

// --- NEW: Balcony Data Structure and Global Vector ---
struct Balcony {
    glm::vec3 position; // World space center position of the balcony floor
    glm::vec3 dimensions; // Width, Height (floor thickness), Depth
    // Store railing positions relative to the center for easier drawing/collision
    glm::vec3 railingFrontPosRel;
    glm::vec3 railingLeftPosRel;
    glm::vec3 railingRightPosRel;
    glm::vec3 railingDimsFront; // Width, Height, Thickness
    glm::vec3 railingDimsSide;  // Thickness, Height, Depth
};
std::vector<Balcony> balconyData; // Global vector to store all balconies

// --- Function Prototypes ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window); // Updated prototype (no functional change needed)
unsigned int compileShader(GLenum type, const char* source);
unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource);
void generateObjectPositions(std::vector<glm::vec3>& positions, float areaSize, int count);
void generateTowersAndBalconies(float areaSize, int towerCount, int balconiesPerTower); // NEW function
void toggleFullscreen(GLFWwindow* window);
bool checkCollision(glm::vec3 nextPos); // Collision detection function

// --- Win32 Specific Prototypes & Globals ---
#ifdef _WIN32
#define ID_EDIT_SEED 101
#define ID_BUTTON_OK 102
#define ID_BUTTON_RANDOM 103
#define ID_CHECKBOX_FLY 104 // *** NEW: ID for the fly mode checkbox ***

HWND hEditSeed = NULL;
HWND hCheckFly = NULL; // *** NEW: Handle for the checkbox ***
unsigned int g_seed = 0;
bool g_seedChosen = false;
bool g_flyModeEnabled = false; // *** NEW: Global flag for fly mode ***

LRESULT CALLBACK SeedDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool ShowSeedDialog(HINSTANCE hInstance);
#endif

// --- Shader Source Code (Unchanged) ---
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main() { gl_Position = projection * view * model * vec4(aPos, 1.0); }
)";
const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    uniform vec3 objectColor;
    void main() { FragColor = vec4(objectColor, 1.0); }
)";

// --- Main Function ---
int main(int argc, char** argv) {

#ifdef _WIN32
    // --- Show Win32 Seed Dialog FIRST ---
    HINSTANCE hInstance = GetModuleHandle(NULL);
    if (!ShowSeedDialog(hInstance)) {
        if (!g_seedChosen) {
            std::cerr << "Seed selection cancelled. Exiting." << std::endl;
            return 0;
        }
    }
    std::cout << "Using seed: " << (g_seed == 0 ? "Random (time-based)" : std::to_string(g_seed)) << std::endl;
    std::cout << "Fly Mode: " << (g_flyModeEnabled ? "Enabled" : "Disabled") << std::endl; // *** NEW: Print fly mode status ***

#else
    std::cout << "Non-Windows platform. Using default random seed." << std::endl;
    g_seed = 0;
    g_flyModeEnabled = false; // Default to disabled on non-Windows
#endif

    // --- Seed Random Number Generator ONCE ---
    if (g_seed == 0) {
        srand(static_cast<unsigned int>(time(0)));
        std::cout << "Seeding with time(0)" << std::endl;
    }
    else {
        srand(g_seed);
        std::cout << "Seeding with " << g_seed << std::endl;
    }

    // --- Easter Egg Check ---
    if (g_seed == 666) {
        std::cout << "Easter Egg Activated: Red Sky!" << std::endl;
        skyColor = glm::vec3(0.6f, 0.1f, 0.1f); // Set sky to red
    }
    else {
        skyColor = glm::vec3(0.5f, 0.8f, 0.95f); // Ensure default blue otherwise
    }


    // --- 1. Initialize GLFW ---
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // --- 2. Create GLFW Window ---
    GLFWwindow* window = glfwCreateWindow(INITIAL_SCR_WIDTH, INITIAL_SCR_HEIGHT, "OpenGL Procedural Forest - Walking/Flying Sim", NULL, NULL); // Update title
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwGetWindowPos(window, &lastWindowPosX, &lastWindowPosY);


    // --- 3. Initialize GLAD ---
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return -1;
    }

    // --- 4. Configure OpenGL Global State ---
    glEnable(GL_DEPTH_TEST);

    // --- 5. Build and Compile Shaders ---
    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    if (shaderProgram == 0) {
        glfwTerminate();
        return -1;
    }

    // --- 6. Set up Vertex Data and Buffers (Cube Vertices - Unchanged) ---
    float vertices[] = {
        // positions (unit cube centered at origin) - Unchanged
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f,
    };
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // --- 7. Generate Object Positions ---
    generateObjectPositions(treePositions, GROUND_SIZE, TREE_COUNT);
    generateObjectPositions(bushPositions, GROUND_SIZE, BUSH_COUNT);
    generateObjectPositions(housePositions, GROUND_SIZE, HOUSE_COUNT);
    // NEW: Generate towers and their balconies together
    generateTowersAndBalconies(GROUND_SIZE, APARTMENT_TOWER_COUNT, BALCONIES_PER_TOWER);

    // --- 8. Rendering Loop ---
    while (!glfwWindowShouldClose(window)) {
        // Timing
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        // Avoid large deltaTime steps if debugging or paused
        if (deltaTime > 0.1f) deltaTime = 0.1f;


        // Input & Physics Update (handles movement, gravity, collision, sprinting, FLY MODE)
        processInput(window); // Calls the updated function

        // Rendering
        glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Matrices
        int currentWidth, currentHeight;
        glfwGetFramebufferSize(window, &currentWidth, &currentHeight);
        // Prevent division by zero if window is minimized
        if (currentHeight == 0) currentHeight = 1;
        glm::mat4 projection = glm::perspective(glm::radians(fov), (float)currentWidth / (float)currentHeight, 0.1f, GROUND_SIZE * 2.0f); // Adjust far plane if needed
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

        GLint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");

        glBindVertexArray(VAO);

        // Draw Ground (Unchanged)
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -0.5f, 0.0f)); // Keep visual center
        model = glm::scale(model, glm::vec3(GROUND_SIZE, 0.1f, GROUND_SIZE));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3f(objectColorLoc, 0.2f, 0.8f, 0.2f);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // --- *** NEW: Draw Sun *** ---
        model = glm::mat4(1.0f);
        model = glm::translate(model, SUN_POSITION);
        model = glm::scale(model, glm::vec3(SUN_SIZE));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(objectColorLoc, 1, glm::value_ptr(SUN_COLOR));
        glDrawArrays(GL_TRIANGLES, 0, 36);
        // --- *** END Draw Sun *** ---


        // Draw Trees (Unchanged)
        glm::vec3 trunkColor = glm::vec3(0.6f, 0.4f, 0.2f);
        glm::vec3 leavesColor = glm::vec3(0.1f, 0.5f, 0.1f);
        for (const auto& pos : treePositions) {
            // Trunk
            model = glm::mat4(1.0f);
            model = glm::translate(model, pos + glm::vec3(0.0f, TREE_TRUNK_HEIGHT * 0.5f, 0.0f));
            model = glm::scale(model, glm::vec3(TREE_TRUNK_RADIUS * 2.0f, TREE_TRUNK_HEIGHT, TREE_TRUNK_RADIUS * 2.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(objectColorLoc, 1, glm::value_ptr(trunkColor));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            // Leaves
            model = glm::mat4(1.0f);
            model = glm::translate(model, pos + glm::vec3(0.0f, TREE_TRUNK_HEIGHT + 0.75f, 0.0f));
            model = glm::scale(model, glm::vec3(1.5f, 1.5f, 1.5f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(objectColorLoc, 1, glm::value_ptr(leavesColor));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // Draw Bushes (Unchanged)
        glm::vec3 bushColor = glm::vec3(0.2f, 0.6f, 0.1f);
        const float bushScaleFactor = 0.8f;
        for (const auto& pos : bushPositions) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, pos + glm::vec3(0.0f, bushScaleFactor * 0.5f, 0.0f));
            model = glm::scale(model, glm::vec3(bushScaleFactor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(objectColorLoc, 1, glm::value_ptr(bushColor));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // Draw Houses (Unchanged)
        glm::vec3 houseBodyColor = glm::vec3(0.8f, 0.7f, 0.5f);
        glm::vec3 houseRoofColor = glm::vec3(0.4f, 0.2f, 0.1f);
        glm::vec3 houseDoorColor = glm::vec3(0.3f, 0.15f, 0.05f);
        glm::vec3 houseWindowColor = glm::vec3(0.6f, 0.8f, 0.9f);
        const float roofHeight = 0.3f; const float roofOverhang = 0.4f;
        const float doorWidth = 1.0f; const float doorHeight = 2.0f; const float windowSize = 0.8f;
        for (const auto& pos : housePositions) {
            glm::mat4 identityMat = glm::mat4(1.0f);
            glm::vec3 bodyCenterPos = pos + glm::vec3(0.0f, HOUSE_BODY_HEIGHT * 0.5f, 0.0f);
            // Body
            model = glm::mat4(1.0f); model = glm::translate(model, bodyCenterPos); model = glm::scale(model, glm::vec3(HOUSE_BODY_WIDTH, HOUSE_BODY_HEIGHT, HOUSE_BODY_DEPTH));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); glUniform3fv(objectColorLoc, 1, glm::value_ptr(houseBodyColor)); glDrawArrays(GL_TRIANGLES, 0, 36);
            // Roof
            model = glm::mat4(1.0f); model = glm::translate(model, bodyCenterPos + glm::vec3(0.0f, HOUSE_BODY_HEIGHT * 0.5f + roofHeight * 0.5f, 0.0f)); model = glm::scale(model, glm::vec3(HOUSE_BODY_WIDTH + roofOverhang * 2.0f, roofHeight, HOUSE_BODY_DEPTH + roofOverhang * 2.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); glUniform3fv(objectColorLoc, 1, glm::value_ptr(houseRoofColor)); glDrawArrays(GL_TRIANGLES, 0, 36);
            // Door
            model = glm::mat4(1.0f); glm::vec3 doorOffset = glm::vec3(0.0f, -HOUSE_BODY_HEIGHT * 0.5f + doorHeight * 0.5f, HOUSE_BODY_DEPTH * 0.5f + 0.01f); model = glm::translate(model, bodyCenterPos + doorOffset); model = glm::scale(model, glm::vec3(doorWidth, doorHeight, 0.1f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); glUniform3fv(objectColorLoc, 1, glm::value_ptr(houseDoorColor)); glDrawArrays(GL_TRIANGLES, 0, 36);
            // Window 1
            model = glm::mat4(1.0f); glm::vec3 win1Offset = glm::vec3(HOUSE_BODY_WIDTH * 0.25f, 0.0f, HOUSE_BODY_DEPTH * 0.5f + 0.01f); model = glm::translate(model, bodyCenterPos + win1Offset); model = glm::scale(model, glm::vec3(windowSize, windowSize, 0.1f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); glUniform3fv(objectColorLoc, 1, glm::value_ptr(houseWindowColor)); glDrawArrays(GL_TRIANGLES, 0, 36);
            // Window 2
            model = glm::mat4(1.0f); glm::vec3 win2Offset = glm::vec3(HOUSE_BODY_WIDTH * 0.5f + 0.01f, 0.0f, 0.0f); model = glm::translate(model, bodyCenterPos + win2Offset); model = glm::scale(model, glm::vec3(0.1f, windowSize, windowSize));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); glUniform3fv(objectColorLoc, 1, glm::value_ptr(houseWindowColor)); glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // Draw Apartment Towers (Main Body) - Unchanged
        glm::vec3 towerColor = glm::vec3(0.6f, 0.6f, 0.65f); // A grey color
        for (const auto& pos : apartmentTowerPositions) {
            model = glm::mat4(1.0f);
            glm::vec3 towerCenterPos = pos + glm::vec3(0.0f, TOWER_HEIGHT * 0.5f, 0.0f);
            model = glm::translate(model, towerCenterPos);
            model = glm::scale(model, glm::vec3(TOWER_WIDTH, TOWER_HEIGHT, TOWER_DEPTH));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(objectColorLoc, 1, glm::value_ptr(towerColor));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // --- NEW: Draw Balconies and Railings --- (Unchanged)
        glm::vec3 balconyFloorColor = glm::vec3(0.7f, 0.7f, 0.75f); // Slightly lighter grey
        glm::vec3 railingColor = glm::vec3(0.4f, 0.4f, 0.4f);      // Darker grey
        for (const auto& bal : balconyData) {
            // Draw Balcony Floor
            model = glm::mat4(1.0f);
            model = glm::translate(model, bal.position); // Already center position
            model = glm::scale(model, bal.dimensions);   // Use stored dimensions
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(objectColorLoc, 1, glm::value_ptr(balconyFloorColor));
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // Draw Railings (relative to balcony center)
            // Front Railing
            model = glm::mat4(1.0f);
            model = glm::translate(model, bal.position + bal.railingFrontPosRel); // Use relative position
            model = glm::scale(model, bal.railingDimsFront); // Use specific railing dimensions
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(objectColorLoc, 1, glm::value_ptr(railingColor));
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // Left Railing
            model = glm::mat4(1.0f);
            model = glm::translate(model, bal.position + bal.railingLeftPosRel);
            model = glm::scale(model, bal.railingDimsSide);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(objectColorLoc, 1, glm::value_ptr(railingColor));
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // Right Railing
            model = glm::mat4(1.0f);
            model = glm::translate(model, bal.position + bal.railingRightPosRel);
            model = glm::scale(model, bal.railingDimsSide);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(objectColorLoc, 1, glm::value_ptr(railingColor));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }


        glBindVertexArray(0); // Unbind VAO

        // Swap Buffers & Poll Events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --- 9. Cleanup ---
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

// --- Function Implementations ---

// Toggles fullscreen mode (Unchanged)
void toggleFullscreen(GLFWwindow* window) {
    if (isFullscreen) {
        glfwSetWindowMonitor(window, NULL, lastWindowPosX, lastWindowPosY, lastWindowWidth, lastWindowHeight, 0);
        std::cout << "Switched to Windowed Mode" << std::endl;
        isFullscreen = false;
    }
    else {
        glfwGetWindowPos(window, &lastWindowPosX, &lastWindowPosY);
        glfwGetWindowSize(window, &lastWindowWidth, &lastWindowHeight);
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if (!monitor) { std::cerr << "Failed to get primary monitor" << std::endl; return; }
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if (!mode) { std::cerr << "Failed to get video mode" << std::endl; return; }
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        std::cout << "Switched to Fullscreen Mode" << std::endl;
        isFullscreen = true;
    }
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
}

// Check collision between player at nextPos and world obstacles (trees, houses, towers, balconies) - (Unchanged)
// NOTE: This function is now only called if g_flyModeEnabled is false.
bool checkCollision(glm::vec3 nextPos) {
    // Player's horizontal position (ignore Y for this check initially)
    glm::vec2 playerPosXZ(nextPos.x, nextPos.z);

    // Check against Trees (Cylinder collision) - Unchanged
    for (const auto& treeBasePos : treePositions) {
        glm::vec2 treePosXZ(treeBasePos.x, treeBasePos.z);
        float distSq = glm::length2(playerPosXZ - treePosXZ);
        float minCollisionDist = PLAYER_RADIUS + TREE_TRUNK_RADIUS;
        if (distSq < minCollisionDist * minCollisionDist) {
            return true; // Collision detected
        }
    }

    // Check against Houses (AABB collision) - Unchanged
    for (const auto& houseBasePos : housePositions) {
        float objMinX = houseBasePos.x - HOUSE_BODY_WIDTH / 2.0f;
        float objMaxX = houseBasePos.x + HOUSE_BODY_WIDTH / 2.0f;
        float objMinZ = houseBasePos.z - HOUSE_BODY_DEPTH / 2.0f;
        float objMaxZ = houseBasePos.z + HOUSE_BODY_DEPTH / 2.0f;
        float closestX = glm::clamp(playerPosXZ.x, objMinX, objMaxX);
        float closestZ = glm::clamp(playerPosXZ.y, objMinZ, objMaxZ); // Use playerPosXZ.y for Z component
        glm::vec2 closestPointXZ(closestX, closestZ);
        float distSq = glm::length2(playerPosXZ - closestPointXZ);
        if (distSq < (PLAYER_RADIUS * PLAYER_RADIUS)) {
            return true; // Collision detected
        }
    }

    // Check against Apartment Towers (AABB collision) - Unchanged
    for (const auto& towerBasePos : apartmentTowerPositions) {
        float objMinX = towerBasePos.x - TOWER_WIDTH / 2.0f;
        float objMaxX = towerBasePos.x + TOWER_WIDTH / 2.0f;
        float objMinZ = towerBasePos.z - TOWER_DEPTH / 2.0f;
        float objMaxZ = towerBasePos.z + TOWER_DEPTH / 2.0f;
        float closestX = glm::clamp(playerPosXZ.x, objMinX, objMaxX);
        float closestZ = glm::clamp(playerPosXZ.y, objMinZ, objMaxZ); // Use playerPosXZ.y for Z component
        glm::vec2 closestPointXZ(closestX, closestZ);
        float distSq = glm::length2(playerPosXZ - closestPointXZ);
        if (distSq < (PLAYER_RADIUS * PLAYER_RADIUS)) {
            return true; // Collision detected
        }
    }

    // --- NEW: Check against Balconies (AABB collision with vertical check) --- (Unchanged)
    for (const auto& bal : balconyData) {
        // Define the balcony AABB in the XZ plane (using its center and dimensions)
        float balMinX = bal.position.x - bal.dimensions.x / 2.0f;
        float balMaxX = bal.position.x + bal.dimensions.x / 2.0f;
        float balMinZ = bal.position.z - bal.dimensions.z / 2.0f; // Using depth for Z
        float balMaxZ = bal.position.z + bal.dimensions.z / 2.0f;

        // Find the closest point on the balcony AABB to the player's center XZ position
        float closestX = glm::clamp(playerPosXZ.x, balMinX, balMaxX);
        float closestZ = glm::clamp(playerPosXZ.y, balMinZ, balMaxZ); // playerPosXZ.y is player's Z

        // Calculate the distance squared between the player's XZ center and this closest point
        glm::vec2 closestPointXZ(closestX, closestZ);
        float distSq = glm::length2(playerPosXZ - closestPointXZ);

        // If the horizontal distance squared is less than the player's radius squared,
        // then check vertical alignment.
        if (distSq < (PLAYER_RADIUS * PLAYER_RADIUS)) {
            // Calculate player's vertical bounds (feet and head)
            float playerFeetY = nextPos.y - PLAYER_EYE_HEIGHT;
            float playerHeadY = nextPos.y;

            // Calculate balcony's vertical bounds (floor bottom to railing top)
            float balconyFloorBottomY = bal.position.y - bal.dimensions.y / 2.0f;
            // Consider railing height for the top bound
            float balconyEffectiveTopY = bal.position.y + bal.dimensions.y / 2.0f + BALCONY_RAILING_HEIGHT;

            // Check for vertical overlap:
            // Player is overlapping if their head is above the balcony floor AND their feet are below the balcony top (including railing)
            if (playerHeadY > balconyFloorBottomY && playerFeetY < balconyEffectiveTopY) {
                // std::cout << "Collision with Balcony! Player Y: " << nextPos.y << " Balcony Y: " << bal.position.y << std::endl; // Debug
                return true; // Collision detected
            }
        }
    }


    return false; // No collision
}


// Process keyboard input, update physics and camera position - (Unchanged)
void processInput(GLFWwindow* window) {
    // Exit
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // --- Speed Calculation ---
    float currentSpeed = PLAYER_BASE_SPEED;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        currentSpeed *= SPRINT_MULTIPLIER;
    }

    // --- Horizontal/Forward Movement Direction ---
    glm::vec3 moveDir(0.0f);
    // In fly mode, use the full cameraFront vector for movement
    // In normal mode, only use the XZ components for ground movement
    glm::vec3 forward = g_flyModeEnabled ? cameraFront : glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f))); // Right is always perpendicular to world up

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveDir += forward;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveDir -= forward;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveDir -= right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveDir += right;

    // Normalize moveDir if there's movement
    if (glm::length2(moveDir) > 0.0001f) {
        moveDir = glm::normalize(moveDir);
    }

    glm::vec3 deltaMove = moveDir * currentSpeed * deltaTime;

    // --- Apply Movement & Handle Physics/Collisions based on Mode ---
    if (g_flyModeEnabled) {
        // --- Fly Mode ---
        // No gravity, no ground check, no collision detection

        // Apply horizontal/forward movement directly
        cameraPos += deltaMove;

        // Apply vertical movement
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            cameraPos.y += FLY_VERTICAL_SPEED * deltaTime;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
            cameraPos.y -= FLY_VERTICAL_SPEED * deltaTime;
        }
        isOnGround = false; // Not on ground when flying
        cameraVelocityY = 0.0f; // Reset vertical velocity

    }
    else {
        // --- Normal (Walk/Jump) Mode ---
        // Apply gravity, ground check, collision detection

        // Collision Detection & Resolution (Simple Slide)
        glm::vec3 currentPos = cameraPos;

        // Try moving along X axis
        glm::vec3 nextPosX = currentPos;
        nextPosX.x += deltaMove.x; // Only change X for this check
        if (!checkCollision(nextPosX)) { // Check collision at the potential new X position (but original Y/Z)
            cameraPos.x = nextPosX.x; // Update camera's X if no collision
        }

        // Try moving along Z axis
        // Use the potentially updated X position from the previous step
        glm::vec3 nextPosZ = cameraPos; // Start from current camera pos (which might have updated X)
        nextPosZ.z += deltaMove.z; // Only change Z for this check
        if (!checkCollision(nextPosZ)) { // Check collision at the potential new Z position (with potentially updated X, original Y)
            cameraPos.z = nextPosZ.z; // Update camera's Z if no collision
        }

        // --- Vertical Movement (Gravity & Jump) ---
        // Apply gravity
        cameraVelocityY -= GRAVITY * deltaTime;

        // Jump
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && isOnGround) {
            cameraVelocityY = JUMP_FORCE;
            isOnGround = false; // Prevent holding space for continuous jumping
        }

        // Update vertical position
        glm::vec3 nextPosY = cameraPos; // Start from current position (potentially updated X/Z)
        nextPosY.y += cameraVelocityY * deltaTime; // Calculate potential new Y

        // Ground collision check (before applying vertical movement)
        float playerFeetY = nextPosY.y - PLAYER_EYE_HEIGHT;
        if (playerFeetY <= GROUND_LEVEL) {
            cameraPos.y = GROUND_LEVEL + PLAYER_EYE_HEIGHT; // Snap to ground
            cameraVelocityY = 0.0f; // Stop falling
            isOnGround = true;      // Allow jumping again
        }
        else {
            // If not hitting the ground, apply the calculated vertical movement
            cameraPos.y = nextPosY.y;
            isOnGround = false; // Player is in the air
        }
    }


    // --- Fullscreen Toggle (F11) - Debounced --- (Unchanged)
    bool f11Pressed = glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS;
    if (f11Pressed && !f11KeyPressedLastFrame) {
        toggleFullscreen(window);
    }
    f11KeyPressedLastFrame = f11Pressed; // Update state for next frame
}

// GLFW framebuffer size callback (Unchanged)
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// GLFW mouse callback (Unchanged)
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    lastX = xpos; lastY = ypos;
    float sensitivity = 0.1f;
    xoffset *= sensitivity; yoffset *= sensitivity;
    yaw += xoffset; pitch += yoffset;
    if (pitch > 89.0f) pitch = 89.0f; if (pitch < -89.0f) pitch = -89.0f;
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
    // Recalculate cameraUp to prevent roll issues, especially in fly mode
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, worldUp));
    cameraUp = glm::normalize(glm::cross(cameraRight, cameraFront));
}

// Compile Shader (Unchanged)
unsigned int compileShader(GLenum type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    int success; char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::" << (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT") << "::COMPILATION_FAILED\n" << infoLog << std::endl;
        glDeleteShader(shader); return 0;
    } return shader;
}

// Create Shader Program (Unchanged)
unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource); if (vertexShader == 0) return 0;
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource); if (fragmentShader == 0) { glDeleteShader(vertexShader); return 0; }
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader); glAttachShader(shaderProgram, fragmentShader); glLinkProgram(shaderProgram);
    int success; char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        glDeleteProgram(shaderProgram); shaderProgram = 0;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

// Generate Object Positions (Generic version, used for trees, bushes, houses) - Unchanged
void generateObjectPositions(std::vector<glm::vec3>& positions, float areaSize, int count) {
    float halfSize = areaSize / 2.0f;
    positions.clear();
    positions.reserve(count);
    for (int i = 0; i < count; ++i) {
        float x = (static_cast<float>(rand()) / RAND_MAX) * areaSize - halfSize;
        float z = (static_cast<float>(rand()) / RAND_MAX) * areaSize - halfSize;
        positions.push_back(glm::vec3(x, GROUND_LEVEL, z));
    }
}

// --- NEW: Generate Towers and Balconies --- (Unchanged)
void generateTowersAndBalconies(float areaSize, int towerCount, int balconiesPerTower) {
    float halfSize = areaSize / 2.0f;
    apartmentTowerPositions.clear();
    apartmentTowerPositions.reserve(towerCount);
    balconyData.clear();
    balconyData.reserve(towerCount * balconiesPerTower); // Reserve space

    for (int i = 0; i < towerCount; ++i) {
        // --- Generate Tower Position ---
        float towerX = (static_cast<float>(rand()) / RAND_MAX) * areaSize - halfSize;
        float towerZ = (static_cast<float>(rand()) / RAND_MAX) * areaSize - halfSize;
        glm::vec3 towerBasePos = glm::vec3(towerX, GROUND_LEVEL, towerZ);
        apartmentTowerPositions.push_back(towerBasePos);

        // --- Generate Balconies for this Tower ---
        for (int j = 0; j < balconiesPerTower; ++j) {
            Balcony bal;

            // Determine balcony height (distribute somewhat evenly, avoiding very top/bottom)
            float heightFraction = (static_cast<float>(j + 1) / (balconiesPerTower + 1));
            float balconyY = towerBasePos.y + TOWER_HEIGHT * heightFraction; // Center Y of the balcony floor

            // Determine which side the balcony is on (randomly)
            int side = rand() % 4; // 0: +Z, 1: -Z, 2: +X, 3: -X
            float balconyX = towerX;
            float balconyZ = towerZ;
            float railingOffsetX = BALCONY_WIDTH / 2.0f - BALCONY_RAILING_THICKNESS / 2.0f;
            float railingOffsetZ = BALCONY_DEPTH / 2.0f - BALCONY_RAILING_THICKNESS / 2.0f;
            float railingOffsetY = BALCONY_FLOOR_HEIGHT / 2.0f + BALCONY_RAILING_HEIGHT / 2.0f;

            bal.dimensions = glm::vec3(BALCONY_WIDTH, BALCONY_FLOOR_HEIGHT, BALCONY_DEPTH);
            bal.railingDimsFront = glm::vec3(BALCONY_WIDTH, BALCONY_RAILING_HEIGHT, BALCONY_RAILING_THICKNESS);
            bal.railingDimsSide = glm::vec3(BALCONY_RAILING_THICKNESS, BALCONY_RAILING_HEIGHT, BALCONY_DEPTH);


            if (side == 0) { // Front (+Z)
                balconyZ += TOWER_DEPTH / 2.0f + BALCONY_DEPTH / 2.0f;
                bal.railingFrontPosRel = glm::vec3(0.0f, railingOffsetY, railingOffsetZ);
                bal.railingLeftPosRel = glm::vec3(-railingOffsetX, railingOffsetY, 0.0f);
                bal.railingRightPosRel = glm::vec3(railingOffsetX, railingOffsetY, 0.0f);
            }
            else if (side == 1) { // Back (-Z)
                balconyZ -= TOWER_DEPTH / 2.0f + BALCONY_DEPTH / 2.0f;
                bal.railingFrontPosRel = glm::vec3(0.0f, railingOffsetY, -railingOffsetZ); // Back railing
                bal.railingLeftPosRel = glm::vec3(-railingOffsetX, railingOffsetY, 0.0f);
                bal.railingRightPosRel = glm::vec3(railingOffsetX, railingOffsetY, 0.0f);
                // Swap side railing dims W/D
                bal.railingDimsFront = glm::vec3(BALCONY_WIDTH, BALCONY_RAILING_HEIGHT, BALCONY_RAILING_THICKNESS);
                bal.railingDimsSide = glm::vec3(BALCONY_RAILING_THICKNESS, BALCONY_RAILING_HEIGHT, BALCONY_DEPTH);
            }
            else if (side == 2) { // Right (+X)
                balconyX += TOWER_WIDTH / 2.0f + BALCONY_DEPTH / 2.0f; // Use depth for offset along X
                bal.dimensions = glm::vec3(BALCONY_DEPTH, BALCONY_FLOOR_HEIGHT, BALCONY_WIDTH); // Swap W/D for dimensions
                bal.railingFrontPosRel = glm::vec3(railingOffsetZ, railingOffsetY, 0.0f); // Use Z offset for X direction railing
                bal.railingLeftPosRel = glm::vec3(0.0f, railingOffsetY, -railingOffsetX); // Use X offset for Z direction railing
                bal.railingRightPosRel = glm::vec3(0.0f, railingOffsetY, railingOffsetX); // Use X offset for Z direction railing
                // Swap railing dims
                bal.railingDimsFront = glm::vec3(BALCONY_RAILING_THICKNESS, BALCONY_RAILING_HEIGHT, BALCONY_WIDTH); // Thickness, Height, Width(as depth)
                bal.railingDimsSide = glm::vec3(BALCONY_DEPTH, BALCONY_RAILING_HEIGHT, BALCONY_RAILING_THICKNESS); // Depth(as width), Height, Thickness

            }
            else { // Left (-X)
                balconyX -= TOWER_WIDTH / 2.0f + BALCONY_DEPTH / 2.0f; // Use depth for offset along X
                bal.dimensions = glm::vec3(BALCONY_DEPTH, BALCONY_FLOOR_HEIGHT, BALCONY_WIDTH); // Swap W/D for dimensions
                bal.railingFrontPosRel = glm::vec3(-railingOffsetZ, railingOffsetY, 0.0f); // Use Z offset for X direction railing
                bal.railingLeftPosRel = glm::vec3(0.0f, railingOffsetY, -railingOffsetX); // Use X offset for Z direction railing
                bal.railingRightPosRel = glm::vec3(0.0f, railingOffsetY, railingOffsetX); // Use X offset for Z direction railing
                // Swap railing dims
                bal.railingDimsFront = glm::vec3(BALCONY_RAILING_THICKNESS, BALCONY_RAILING_HEIGHT, BALCONY_WIDTH); // Thickness, Height, Width(as depth)
                bal.railingDimsSide = glm::vec3(BALCONY_DEPTH, BALCONY_RAILING_HEIGHT, BALCONY_RAILING_THICKNESS); // Depth(as width), Height, Thickness
            }


            bal.position = glm::vec3(balconyX, balconyY, balconyZ);
            balconyData.push_back(bal);
            // std::cout << "Generated Balcony at: " << glm::to_string(bal.position) << std::endl; // Debug
        }
    }
    std::cout << "Generated " << apartmentTowerPositions.size() << " towers and " << balconyData.size() << " balconies." << std::endl;
}


// --- Win32 Specific Functions --- (Unchanged)
#ifdef _WIN32

// Window Procedure for the Seed Dialog
LRESULT CALLBACK SeedDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // Seed Input Label and Edit Box
        CreateWindowW(L"STATIC", L"Enter Seed (number) or choose Random:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 10, 260, 20, hwnd, NULL, NULL, NULL);
        hEditSeed = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 10, 35, 180, 20, hwnd, (HMENU)ID_EDIT_SEED, NULL, NULL);

        // OK Button
        CreateWindowW(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 200, 35, 70, 25, hwnd, (HMENU)ID_BUTTON_OK, NULL, NULL);

        // Random Button
        CreateWindowW(L"BUTTON", L"Generate Random", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 10, 65, 260, 25, hwnd, (HMENU)ID_BUTTON_RANDOM, NULL, NULL);

        // *** NEW: Fly Mode Checkbox ***
        hCheckFly = CreateWindowW(L"BUTTON", L"Enable Fly Mode (No Clip/Gravity)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 10, 95, 260, 20, hwnd, (HMENU)ID_CHECKBOX_FLY, NULL, NULL);
        // Optionally, set the initial state (e.g., CheckDlgButton(hwnd, ID_CHECKBOX_FLY, BST_CHECKED); )

        return 0;
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case ID_BUTTON_OK: {
            wchar_t buffer[20]; GetWindowTextW(hEditSeed, buffer, 20);
            unsigned int tempSeed = 0;
            if (swscanf_s(buffer, L"%u", &tempSeed) == 1) {
                g_seed = tempSeed;
                g_seedChosen = true;
                // *** NEW: Get checkbox state ***
                g_flyModeEnabled = (IsDlgButtonChecked(hwnd, ID_CHECKBOX_FLY) == BST_CHECKED);
                DestroyWindow(hwnd);
            }
            else {
                MessageBoxW(hwnd, L"Invalid seed. Please enter a non-negative whole number.", L"Input Error", MB_OK | MB_ICONWARNING); SetFocus(hEditSeed);
            } return 0;
        }
        case ID_BUTTON_RANDOM: {
            g_seed = 0;
            g_seedChosen = true;
            // *** NEW: Get checkbox state ***
            g_flyModeEnabled = (IsDlgButtonChecked(hwnd, ID_CHECKBOX_FLY) == BST_CHECKED);
            DestroyWindow(hwnd); return 0;
        }
                             // No action needed for ID_CHECKBOX_FLY click itself, state is read on OK/Random
        } break;
    }
    case WM_CLOSE: g_seedChosen = false; DestroyWindow(hwnd); return 0; // Handle closing the dialog window
    case WM_DESTROY: PostQuitMessage(0); return 0; // Ensure application exits cleanly if dialog is destroyed
    } return DefWindowProcW(hwnd, msg, wParam, lParam); // Default handling for other messages
}

// Function to create and show the Seed Dialog window (Unchanged)
bool ShowSeedDialog(HINSTANCE hInstance) {
    const wchar_t CLASS_NAME[] = L"SeedDialogClass";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = SeedDialogProc; wc.hInstance = hInstance; wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    if (!RegisterClassW(&wc)) {
        MessageBoxW(NULL, L"Window Registration Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }
    int screenWidth = GetSystemMetrics(SM_CXSCREEN); int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowWidth = 300;
    int windowHeight = 170; // Height including checkbox
    int posX = (screenWidth - windowWidth) / 2; int posY = (screenHeight - windowHeight) / 2;
    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"Set Generation Seed & Options", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, // Updated title
        posX, posY, windowWidth, windowHeight, NULL, NULL, hInstance, NULL
    );
    if (hwnd == NULL) {
        MessageBoxW(NULL, L"Window Creation Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        if (!IsDialogMessage(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // Check if the window handle is still valid, break if destroyed
        if (!IsWindow(hwnd)) {
            break;
        }
    }
    UnregisterClassW(CLASS_NAME, hInstance);
    return g_seedChosen;
}

#endif // _WIN32
