#include <cstdio>
#include <iostream>
#include <vector>
#include <GLEW/glew.h>
#include <GLFW/glfw3.h>
#include "ShaderProgram.h"
#include <tuple>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <glm/glm.hpp>
#include <AntTweakBar.h>

// Global variables
unsigned int gWindowWidth = 800;
unsigned int gWindowHeight = 800;

// Vertex attribute formats
struct VertexColor
{
    GLfloat position[3];
    GLfloat color[3];
};

// Variiable to control wheel rotation
float wheelRotation = 0.0f;

const float maxSpeed = 1.0f;
const float accelRate = 0.8f;
const float decelRate = 2.0f;

// Variable used to calculate delta time
double lastFrameTime = 0.0;

//ground slope
float groundRotation = 0.0f;

// frame stats
float gFrameRate = 60.0f;
float gFrameTime = 1 / gFrameRate;

// Global variable to store the truck's position along the x-axis
float truckPositionX = 0.0f;

// Global background color variable
glm::vec3 gBackgroundColor = glm::vec3(0.2f, 0.2f, 0.2f);

// wireframe mode on or off
bool gWireframe = false;

// Initial stationary velocity of the truck.
glm::vec2 truckVelocity = glm::vec2(0.0f, 0.0f);

// Initial zero acceleration of the truck.
glm::vec2 truckAcceleration = glm::vec2(0.0f, 0.0f);

// Initial position of the truck at the origin.
glm::vec2 truckPosition = glm::vec2(0.0f, 0.0f);

// Scene content
ShaderProgram gShader;
GLuint truckVBO = 0;
GLuint truckVAO = 0;
GLuint groundVBO, groundVAO; // I created a seperate VAO ans VBO for ground and truck even though VBO can be shared/ reused.



void calculateWheelAndHubCapVertices(float centerX, float centerY, float wheelRadius, float hubCapRadius, int numSegments, std::vector<GLfloat>& vertices) {
    const float PI = 3.14159265359f;
    float anglePerSegment = 2 * PI / numSegments;

    // Generate wheel (black circle)
    for (int i = 0; i < numSegments; ++i) {
        float angle = i * anglePerSegment;
        float nextAngle = (i + 1) * anglePerSegment;

        // Center vertex (black)
        vertices.push_back(centerX); vertices.push_back(centerY); vertices.push_back(0.0f);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);

        // Current vertex (black)
        vertices.push_back(centerX + wheelRadius * cos(angle)); vertices.push_back(centerY + wheelRadius * sin(angle)); vertices.push_back(0.0f);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);

        // Next vertex (black)
        vertices.push_back(centerX + wheelRadius * cos(nextAngle)); vertices.push_back(centerY + wheelRadius * sin(nextAngle)); vertices.push_back(0.0f);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
    }

    // Generate hub cap (grey circle with horizontal glint and brighter center)
       for (int i = 0; i < numSegments; ++i) {
           float angle = i * anglePerSegment;
           float nextAngle = (i + 1) * anglePerSegment;

           // Calculate y-coordinate for current and next vertex
           float yCurrent = centerY + hubCapRadius * sin(angle);
           float yNext = centerY + hubCapRadius * sin(nextAngle);

           // Check if the segment is near horizontal axis
           bool isNearHorizontal = fabs(yCurrent - centerY) < (hubCapRadius / 10.0f) || fabs(yNext - centerY) < (hubCapRadius / 10.0f);
           float glintColor = isNearHorizontal ? 0.6f : 0.5f;

           // Brighter center vertex (neutral bright grey)
           vertices.push_back(centerX); vertices.push_back(centerY); vertices.push_back(0.0f);
           vertices.push_back(0.85f); vertices.push_back(0.85f); vertices.push_back(0.85f); // Neutral bright grey for center

           // Current vertex (grey/glint)
           vertices.push_back(centerX + hubCapRadius * cos(angle)); vertices.push_back(yCurrent); vertices.push_back(0.0f);
           vertices.push_back(glintColor); vertices.push_back(glintColor); vertices.push_back(glintColor);

           // Next vertex (grey/glint)
           vertices.push_back(centerX + hubCapRadius * cos(nextAngle)); vertices.push_back(yNext); vertices.push_back(0.0f);
           vertices.push_back(glintColor); vertices.push_back(glintColor); vertices.push_back(glintColor);
       }
   }




// Function to initialise scene and render settings
static void initialiseTruck(GLFWwindow *window)
{
    // Set the color the color buffer should be cleared to
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Compile and link a vertex and fragment shader pair
    gShader.compileAndLink("color.vert", "color.frag");

    std::vector<GLfloat> baseVertices = {
        // tr1
        -0.3995f, -0.34f, 0.0f,    // Vertex 0: x, y, z
        0.4f, 0.5f, 0.5f,          // Color

        -0.3995f, -0.3995f, 0.0f,  // Vertex 1: x, y, z
        0.4f, 0.5f, 0.5f,          // Color

        0.3655f, -0.3995f, 0.0f,   // Vertex 2: x, y, z
        0.4f, 0.5f, 0.5f,          // Color
        // tr2
        -0.3995f, -0.34f, 0.0f,    // Vertex 0: x, y, z
        0.4f, 0.5f, 0.5f,          // Color

        0.3655f, -0.34f, 0.0f,     // Vertex 1: x, y, z
        0.4f, 0.5f, 0.5f,          // Color

        0.3655f, -0.3995f, 0.0f,   // Vertex 2: x, y, z
        0.4f, 0.5f, 0.5f           // Color
    };

    std::vector<GLfloat> cabVertices = {
        // Cab
        // tr1
        -0.3315f, 0.0595f, 0.0f,    // Vertex 0: x, y , z
        0.0f, 1.0f, 0.0f,           // Color: Green
        -0.102f, 0.0595f, 0.0f,     // Vertex 1: x, y , z
        0.0f, 1.0f, 0.0f,           // Color: Green
        -0.3995f, -0.153f, 0.0f,    // Vertex 5: x, y , z
        0.0f, 1.0f, 0.0f,           // Color: Green
        // tr2
        -0.102f, 0.0595f, 0.0f,     // Vertex 1: x, y , z
        0.0f, 1.0f, 0.0f,           // Color: Green
        -0.102f, -0.153f, 0.0f,     // Vertex 2: x, y , z
        1.0f, 0.0f, 0.0f,           // Color: Red
        -0.3995f, -0.153f, 0.0f,    // Vertex 5: x, y , z
        0.0f, 1.0f, 0.0f,           // Color: Green
        // tr3
        -0.3995f, -0.153f, 0.0f,    // Vertex 5: x, y , z
        0.0f, 1.0f, 0.0f,           // Color: Green
        -0.102f, -0.153f, 0.0f,     // Vertex 2: x, y , z
        1.0f, 0.0f, 0.0f,           // Color: Red
        -0.3995f, -0.34f, 0.0f,     // Vertex 4: x, y , z
        1.0f, 0.0f, 0.0f,           // Color: Red
        // tr4
        -0.3995f, -0.34f, 0.0f,     // Vertex 4: x, y , z
        1.0f, 0.0f, 0.0f,           // Color: Red
        -0.102f, -0.153f, 0.0f,     // Vertex 2: x, y , z
        1.0f, 0.0f, 0.0f,           // Color: Red
        -0.102f, -0.34f, 0.0f,      // Vertex 3: x, y , z
        1.0f, 0.0f, 0.0f,           // Color: Red
    };

    std::vector<GLfloat> windowVertices = {
        // tr1
        -0.323f, 0.00425f, 0.0f,  // x, y , z (top left of the window)
        0.4f, 0.5f, 0.5f,         // Color: Light Gray
        -0.187f, 0.00425f, 0.0f,  // x, y, z (top right of the window)
        0.2f, 0.2f, 0.2f,         // Color: Black
        -0.374f, -0.153f, 0.0f,   // x, y, z (bottom left of the window)
        0.2f, 0.2f, 0.2f,         // Color: Black

        // tr2
        -0.187f, 0.00425f, 0.0f,  // x, y, z (top right of the window, repeated)
        0.2f, 0.2f, 0.2f,         // Color: Black
        -0.374f, -0.153f, 0.0f,   // x, y, z (bottom left of the window, repeated)
        0.2f, 0.2f, 0.2f,         // Color: Black
        -0.187f, -0.153f, 0.0f,   // x, y, z (bottom right of the window)
        0.4f, 0.5f, 0.5f          // Color: Light Gray
    };

    std::vector<GLfloat> trunkVertices = {
        // tr1
        -0.102f, -0.190f, 0.0f,  // Vertex 0: x, y, z
        1.0f, 0.0f, 0.0f,        // Color: Red
        0.0f, -0.037f, 0.0f,     // Vertex 1: x, y, z
        1.0f, 0.0f, 0.0f,        // Color: Red
        0.3655f, -0.037f, 0.0f,  // Vertex 2: x, y, z
        1.0f, 0.0f, 0.0f,        // Color: Red

        // tr2
        -0.102f, -0.190f, 0.0f,  // Vertex 0: x, y, z
        1.0f, 0.0f, 0.0f,        // Color: Red
        0.3655f, -0.037f, 0.0f,  // Vertex 2: x, y, z
        1.0f, 0.0f, 0.0f,        // Color: Red
        0.4675f, -0.190f, 0.0f,  // Vertex 3: x, y, z
        0.0f, 0.0f, 1.0f,        // Color: Blue

        // tr3
        -0.102f, -0.190f, 0.0f,  // Vertex 0: x, y, z
        1.0f, 0.0f, 0.0f,        // Color: Red
        0.0f, -0.340f, 0.0f,     // Vertex 5: x, y, z
        0.0f, 0.0f, 1.0f,        // Color: Blue
        0.4675f, -0.190f, 0.0f,  // Vertex 3: x, y, z
        0.0f, 0.0f, 1.0f,        // Color: Blue

        // tr4
        0.0f, -0.340f, 0.0f,     // Vertex 1: x, y, z
        0.0f, 0.0f, 1.0f,        // Color: Blue
        0.4675f, -0.190f, 0.0f,  // Vertex 3: x, y, z
        0.0f, 0.0f, 1.0f,        // Color: Blue
        0.3655f, -0.340f, 0.0f,  // Vertex 4: x, y, z
        0.0f, 0.0f, 1.0f         // Color: Blue
    };
    
    //vector to hold wheel vertices
    std::vector<GLfloat> wheelVertices;
    calculateWheelAndHubCapVertices(-0.25075, -0.36975f, 0.13175f, 0.0765f, 32, wheelVertices);
    calculateWheelAndHubCapVertices(0.2155, -0.36975f, 0.13175f, 0.0765f, 32, wheelVertices);
        
    // Combine the vertex data into one vector
    std::vector<GLfloat> allVertices;
    
    // Append vertices to allVertices
    allVertices.insert(allVertices.end(), baseVertices.begin(), baseVertices.end());
    allVertices.insert(allVertices.end(), cabVertices.begin(), cabVertices.end());
    allVertices.insert(allVertices.end(), windowVertices.begin(), windowVertices.end());
    allVertices.insert(allVertices.end(), trunkVertices.begin(), trunkVertices.end());
    allVertices.insert(allVertices.end(), wheelVertices.begin(), wheelVertices.end());
    
    // Now create VBO and buffer the data
    glGenBuffers(1, &truckVBO);              // Generate unused VBO identifier
    glBindBuffer(GL_ARRAY_BUFFER, truckVBO); // Bind the VBO
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * allVertices.size(), &allVertices[0], GL_STATIC_DRAW);

    // Create VAO, specify VBO data and format of the data
    glGenVertexArrays(1, &truckVAO);         // Generate unused VAO identifier
    glBindVertexArray(truckVAO);             // Create VAO
    glBindBuffer(GL_ARRAY_BUFFER, truckVBO); // Bind the VBO
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexColor),
                          reinterpret_cast<void *>(offsetof(VertexColor, position))); // Specify format of position data
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexColor),
                          reinterpret_cast<void *>(offsetof(VertexColor, color))); // Specify format of colour data
    glEnableVertexAttribArray(0); // Enable vertex attributes
    glEnableVertexAttribArray(1);
}




static void initialiseGround(GLFWwindow *window)
{
    // Set the color the color buffer should be cleared to
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    // Compile and link a vertex and fragment shader pair
    gShader.compileAndLink("color.vert", "color.frag");
    
    // Define the vertices for the ground plane
    std::vector<GLfloat> groundVertices = {
        // Triangle 1
        -2.0f, -0.5015f, 0.0f, // Vertex 1 Position
        0.0f, 0.5f, 0.0f,      // Vertex 1 Color (darker green)
        2.0f, -0.5015f, 0.0f,  // Vertex 2 Position
        0.0f, 0.5f, 0.0f,      // Vertex 2 Color (darker green)
        -2.0f, -2.0f, 0.0f,    // Vertex 3 Position
        0.5f, 2.0f, 0.5f,      // Vertex 3 Color (lighter green)

        // Triangle 2
        2.0f, -0.5015f, 0.0f,  // Vertex 1 Position (repeat)
        0.0f, 0.5f, 0.0f,      // Vertex 1 Color (dark green)
        -2.0f, -2.0f, 0.0f,    // Vertex 2 Position (repeat)
        0.5f, 2.0f, 0.5f,      // Vertex 2 Color (light green)
        2.0f, -2.0f, 0.0f,     // Vertex 3 Position (repeat)
        0.5f, 2.0f, 0.5f       // Vertex 3 Color (light green)
    };

    // Generate VBO and VAO for the ground
    glGenBuffers(1, &groundVBO);
    glGenVertexArrays(1, &groundVAO);

    glBindVertexArray(groundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * groundVertices.size(), &groundVertices[0], GL_STATIC_DRAW);

    // Specify the format of the ground vertex data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
}




void renderGround() {
    // Use the shader program
    gShader.use();

    glBindVertexArray(groundVAO);

    // Declare the groundModel matrix
    glm::mat4 groundModel = glm::mat4(1.0f);

    // Set up the pivot point for rotation
    glm::vec3 pivotPoint = glm::vec3(1.0f, -0.5015f, 0.0f); // Pivot at the right corner

    // Apply transformations to the groundModel matrix
    groundModel = glm::translate(groundModel, pivotPoint);
    groundModel = glm::rotate(groundModel, glm::radians(-groundRotation), glm::vec3(0.0f, 0.0f, 1.0f));
    groundModel = glm::translate(groundModel, -pivotPoint);

    // Use the modified model matrix for rendering the ground
    gShader.setMat4("model", groundModel);

    // Render the ground with 2 triangles (using baseVertices)
    glBindVertexArray(groundVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    // Flush the graphics pipeline
    glFlush();

}




static void renderTruck()
{
    // Use the shaders associated with the shader program
    gShader.use();

    // Truck transformation setup
    glm::mat4 truckModel = glm::mat4(1.0f);
    glm::vec3 pivotPoint = glm::vec3(1.0f, -0.5015f, 0.0f); // Adjust pivotPoint as needed for your scenario

    // Apply pivot rotation and truck movement
    truckModel = glm::translate(truckModel, pivotPoint);
    truckModel = glm::rotate(truckModel, glm::radians(-groundRotation), glm::vec3(0.0f, 0.0f, 1.0f));
    truckModel = glm::translate(truckModel, -pivotPoint);
  
    // Trucks x axis position
    truckModel = glm::translate(truckModel, glm::vec3(truckPosition, 0.0f));

    // Set the model matrix for the shader (assuming gShader is your shader program and it's already in use)
    gShader.setMat4("model", truckModel);


    // Make VAO active
    glBindVertexArray(truckVAO);

    // Define vertex counts for each part of the truck
    const int baseVertexCount = 6;
    const int windowVertexCount = 6;
    const int cabVertexCount = 12;
    const int trunkVertexCount = 12;
    const int wheelVertexCount = 64 * 3;

    // Calculate the starting index for the wheel vertices
    const int totalVerticesBeforeWheel = baseVertexCount + windowVertexCount + cabVertexCount + trunkVertexCount;
    const int wheelStartIndex = totalVerticesBeforeWheel;

    // Draw the truck parts
    glDrawArrays(GL_TRIANGLES, 0, baseVertexCount);
    glDrawArrays(GL_TRIANGLES, baseVertexCount, windowVertexCount);
    glDrawArrays(GL_TRIANGLES, baseVertexCount + windowVertexCount, cabVertexCount);
    glDrawArrays(GL_TRIANGLES, baseVertexCount + windowVertexCount + cabVertexCount, trunkVertexCount);

    // Define wheel positions
    glm::vec3 wheelLeftPosition = glm::vec3(-0.25075, -0.36975f, 0.0f);
    glm::vec3 wheelRightPosition = glm::vec3(0.2155, -0.36975f, 0.0f);

    // Draw left wheel with rotation
    glm::mat4 leftWheelModel = glm::translate(glm::mat4(1.0f), wheelLeftPosition);
    leftWheelModel = glm::rotate(leftWheelModel, glm::radians(wheelRotation), glm::vec3(0.0f, 0.0f, 1.0f));
    leftWheelModel = glm::translate(leftWheelModel, -wheelLeftPosition);
    leftWheelModel = truckModel * leftWheelModel;
    gShader.setMat4("model", leftWheelModel);
    glDrawArrays(GL_TRIANGLES, wheelStartIndex, wheelVertexCount);

    // Draw right wheel with rotation
    glm::mat4 rightWheelModel = glm::translate(glm::mat4(1.0f), wheelRightPosition);
    rightWheelModel = glm::rotate(rightWheelModel, glm::radians(wheelRotation), glm::vec3(0.0f, 0.0f, 1.0f));
    rightWheelModel = glm::translate(rightWheelModel, -wheelRightPosition);
    rightWheelModel = truckModel * rightWheelModel;
    gShader.setMat4("model", rightWheelModel);
    glDrawArrays(GL_TRIANGLES, wheelStartIndex + wheelVertexCount, wheelVertexCount);

    // Flush the graphics pipeline
    glFlush();
}




// Function to calculate delta time
double calculateDeltaTime() {
    double currentFrameTime = glfwGetTime();
    double deltaTime = currentFrameTime - lastFrameTime;
    lastFrameTime = currentFrameTime;
    return deltaTime;
}



// Function to handle acceleration, deceleration and wheel rotation speed
void physics(double deltaTime) {
  // Update velocity based on acceleration
  if (truckAcceleration.x != 0.0f) {
    truckVelocity.x += truckAcceleration.x * static_cast<float>(deltaTime);
  } else {
    // Apply deceleration when no acceleration input is given
    if (truckVelocity.x > 0.0f) {
      truckVelocity.x -= decelRate * static_cast<float>(deltaTime);
      if (truckVelocity.x < 0.0f) {
        truckVelocity.x = 0.0f;
      }
    } else if (truckVelocity.x < 0.0f) {
      truckVelocity.x += decelRate * static_cast<float>(deltaTime);
      if (truckVelocity.x > 0.0f) {
        truckVelocity.x = 0.0f;
      }
    }
  }
    
  // Clamp the velocity to the maximum speed
  if (glm::length(truckVelocity) > maxSpeed) {
    truckVelocity = glm::normalize(truckVelocity) * maxSpeed;
  }

  // Apply the velocity to the truck's position
  truckPosition += truckVelocity * static_cast<float>(deltaTime);

  // Update global truck position variable
  truckPositionX = truckPosition.x;

  // Increase the rotation speed of the wheel (multiply by a constant factor)
  float rotationSpeed = 900.0f;
  wheelRotation -= rotationSpeed * truckVelocity.x * deltaTime;

  // Ensure wheelRotation stays within [-360, 360] degrees
  if (wheelRotation > 360.0f) {
    wheelRotation -= 360.0f;
  } else if (wheelRotation < -360.0f) {
    wheelRotation += 360.0f;
  }
}




// Key callback function for handling keyboard inputs.
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Close the window if the ESC key is pressed.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
        return;
    }

    // Handle left and right key presses for truck acceleration.
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
            case GLFW_KEY_LEFT:  // Decelerate/move left.
                truckAcceleration.x = -accelRate;
                break;
            case GLFW_KEY_RIGHT: // Accelerate/move right.
                truckAcceleration.x = accelRate;
                break;
        }
    } else if (action == GLFW_RELEASE) { // Reset acceleration on key release.
        switch (key) {
            case GLFW_KEY_LEFT:
            case GLFW_KEY_RIGHT:
                truckAcceleration = glm::vec2(0.0f, 0.0f);
                break;
        }
    }
    
    // Adjust ground rotation based on up and down key presses.
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
            case GLFW_KEY_UP: // Increment ground slope.
                groundRotation += 1.0f;
                if (groundRotation > 15.0f) groundRotation = 15.0f; // Max slope limit.
                break;
            case GLFW_KEY_DOWN: // Decrement ground slope.
                groundRotation -= 1.0f;
                if (groundRotation < -15.0f) groundRotation = -15.0f; // Min slope limit.
                break;
        }
    }
}





// Error callback function
static void error_callback(int error, const char *description)
{
    std::cerr << description << std::endl; // Output error description
}




// cursor movement callback function
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    // pass cursor position to tweak bar
    TwEventMousePosGLFW(static_cast<int>(xpos), static_cast<int>(ypos));
}




// mouse button callback function
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    // pass mouse button status to tweak bar
    TwEventMouseButtonGLFW(button, action);
}




// create and populate tweak bar elements
TwBar* create_UI(const std::string& name) {
    // Create a tweak bar
    TwBar* twBar = TwNewBar(name.c_str());
    
    // Configure the tweak bar
    TwDefine(" Main label='Interface' refresh=0.02 text=light size='220 265' position='0 0' "); // Tweak bar configuration

    // GUI variables
    TwAddVarRO(twBar, "Frame Rate", TW_TYPE_FLOAT, &gFrameRate, " group='Frame Stats' precision=2 ");
    TwAddVarRO(twBar, "Frame Time", TW_TYPE_FLOAT, &gFrameTime, " group='Frame Stats' precision=4 ");
    TwAddVarRW(twBar, "Wireframe", TW_TYPE_BOOLCPP, &gWireframe, " group='Display' ");
    TwAddVarRW(twBar, "BgColor", TW_TYPE_COLOR3F, &gBackgroundColor, " label='Background' group='Display' opened=true ");
    TwAddVarRW(twBar, "GroundRotation", TW_TYPE_FLOAT, &groundRotation, " group='Controls' label='Ground Slope' min=-15 max=15 step=0.1 keyIncr=R keyDecr=r");
    TwAddVarRO(twBar, "TruckPositionX", TW_TYPE_FLOAT, &truckPositionX, " group='Controls' label='Position' precision=3");
    
    return twBar;
}




// Main function
int main(void) {
    GLFWwindow *window = nullptr; // GLFW window handle

    // Set error callback and initialize GLFW
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    // Set window hints for OpenGL version (minimum 3.3)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a windowed mode window and its OpenGL context
    window = glfwCreateWindow(gWindowWidth, gWindowHeight, "Assignment 1 - abah609, 7571562", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window); // Set current context
    glfwSwapInterval(1);            // Swap buffer interval (currently V-SYNC is on)

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW initialisation failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Set key callback
    glfwSetKeyCallback(window, key_callback);

    // Initialization for the scene and rendering settings
    initialiseTruck(window);
    initialiseGround(window);

    // Initialize last frame time
    lastFrameTime = glfwGetTime();
    
    // initialise AntTweakBar
    TwInit(TW_OPENGL_CORE, nullptr);
    TwWindowSize(gWindowWidth, gWindowHeight);
    TwDefine(" TW_HELP visible=false ");
    TwDefine(" GLOBAL fontsize=3 ");
    
    // Setup callbacks for mouse and cursor interaction with TweakBar
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    TwBar* tweakBar = create_UI("Main");

    // timing data
    double lastUpdateTime = glfwGetTime();  // last update time
    double elapsedTime = lastUpdateTime;    // time since last update
    int frameCount = 0;
    
    // Main rendering loop
    while (!glfwWindowShouldClose(window)) {
        double deltaTime = calculateDeltaTime();
        
        // Set the clear color based on the tweak bar's color picker
        glClearColor(gBackgroundColor.r, gBackgroundColor.g, gBackgroundColor.b, 1.0f);
        
        // Clear color buffer
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Set polygon mode based on wireframe mode
        if (gWireframe) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Wireframe mode for the scene
            }
        
        physics(deltaTime);
        
        // Render the scene
        renderTruck();
        renderGround();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // Draw TweakBar
        TwDraw();

        glfwSwapBuffers(window); // Swap buffers
        glfwPollEvents();        // Poll for events
        
        frameCount++;
        elapsedTime = glfwGetTime() - lastUpdateTime;    // time since last update

        // if elapsed time since last update > 1 second
        if (elapsedTime > 1.0)
        {
            gFrameTime = elapsedTime / frameCount;    // average time per frame
            gFrameRate = 1 / gFrameTime;              // frames per second
            lastUpdateTime = glfwGetTime();           // set last update time to current time
            frameCount = 0;                           // reset frame counter
        }
        
        
    }

    // Clean up
    glDeleteBuffers(1, &truckVBO);
    glDeleteVertexArrays(1, &truckVAO);
    
    glDeleteBuffers(1, &groundVBO);
    glDeleteVertexArrays(1, &groundVAO);
    
    // delete and uninitialise tweak bar
    TwDeleteBar(tweakBar);
    TwTerminate();

    glfwDestroyWindow(window); // Close the window
    glfwTerminate();           // Terminate GLFW

    exit(EXIT_SUCCESS);
}
