#include "MP.h"

#include <glm/gtc/type_ptr.hpp>
#include <ctime>
#include <algorithm>
#include <iostream>
#include <sstream>

// Definir STB_IMAGE_IMPLEMENTATION antes de incluir stb_image.h
#define STB_IMAGE_IMPLEMENTATION
#include <objects.hpp>
#include <stb_image.h>
#include "Coin.h"

//*************************************************************************************
//
// Helper Functions

#ifndef M_PI
#define M_PI 3.14159265f
#endif

/// \desc Simple helper function to return a random number between 0.0f and 1.0f.
GLfloat getRand() {
    return static_cast<GLfloat>(rand()) / static_cast<GLfloat>(RAND_MAX);
}

//*************************************************************************************
//
// Public Interface

MP::MP()
    : CSCI441::OpenGLEngine(4, 1, 1280, 720, "MP: Over Hill and Under Hill"),
      _isShiftPressed(false),
      _isLeftMouseButtonPressed(false),
      _isZooming(false),
      _currentCameraMode(ARCBALL),
      _isSmallViewportActive(false),
      _isHeroDamaged(false),
      _heroDamageTime(0.0f),
      _heroVelocity(0.0f, 0.0f, 0.0f),
      _gameState(PLAYING)
{
    // Inicializar todas las teclas como no presionadas
    for(auto& _key : _keys) _key = GL_FALSE;
    _mousePosition = glm::vec2(MOUSE_UNINITIALIZED, MOUSE_UNINITIALIZED);
    _leftMouseButtonState = GLFW_RELEASE;

    // Crear instancias de cámaras
    _arcballCam = new ArcballCam();
    _intiFirstPersonCam = new CSCI441::FreeCam();

    // Inicializar srand para funciones aleatorias si es necesario
    srand(static_cast<unsigned int>(time(0)));

    // Inicializar punteros a zombies como nullptr
    for(int i = 0; i < NUM_ZOMBIES; ++i) {
        _zombies[i] = nullptr;
    }
}

MP::~MP() {

    // Eliminar cámaras
    delete _arcballCam;
    delete _intiFirstPersonCam;

    // Eliminar modelos
    delete _pPlane;

    // Eliminar monedas
    for(int i = 0; i < 4; ++i) {
        delete _coins[i];
    }

    // Eliminar zombies
    for(int i = 0; i < NUM_ZOMBIES; ++i) {
        delete _zombies[i];
    }
}

void MP::handleKeyEvent(GLint key, GLint action) {
    if (key != GLFW_KEY_UNKNOWN)
        _keys[key] = ((action == GLFW_PRESS) || (action == GLFW_REPEAT));

    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_Q:
            case GLFW_KEY_ESCAPE:
                setWindowShouldClose();
                break;
            case GLFW_KEY_LEFT_SHIFT:
            case GLFW_KEY_RIGHT_SHIFT:
                _isShiftPressed = true;
                break;
            case GLFW_KEY_Z:
                _currentCameraMode = ARCBALL;
                _arcballCam->setLookAtPoint(_planePosition + glm::vec3(0.0f, 1.0f, 0.0f));
                _arcballCam->setCameraView(
                    _planePosition + glm::vec3(0.0f, 10.0f, 20.0f),
                    _planePosition + glm::vec3(0.0f, 1.0f, 0.0f),
                    CSCI441::Y_AXIS
                );
                break;
            case GLFW_KEY_X:
                _currentCameraMode = FIRST_PERSON_CAM;
                _updateIntiFirstPersonCamera();
                break;
            case GLFW_KEY_1:
                _isSmallViewportActive = !_isSmallViewportActive;
                break;
            case GLFW_KEY_R:
                if (_gameState == WON || _gameState == LOST) {
                    _resetGame();
                }
                break;
            default:
                break;
        }
    } else if (action == GLFW_RELEASE) {
        if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) {
            _isShiftPressed = false;
        }
    }
}

void MP::handleMouseButtonEvent(GLint button, GLint action) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        _leftMouseButtonState = action;

        _isLeftMouseButtonPressed = (action == GLFW_PRESS);
        _isZooming = _isShiftPressed && _isLeftMouseButtonPressed;

        if (_isZooming) {
            _prevMousePosition = _mousePosition;
        }
    }
}

void MP::handleCursorPositionEvent(glm::vec2 currMousePosition) {
    if (_mousePosition.x == MOUSE_UNINITIALIZED) {
        _mousePosition = currMousePosition;
        return;
    }

    if (_isLeftMouseButtonPressed) {
        float deltaX = currMousePosition.x - _mousePosition.x;
        float deltaY = currMousePosition.y - _mousePosition.y;

        int viewportWidth, viewportHeight;
        glfwGetFramebufferSize(mpWindow, &viewportWidth, &viewportHeight);

        if (_currentCameraMode == ARCBALL) {
            if (_isShiftPressed) {
                // Zoom in ArcballCam
                _arcballCam->rotate(deltaX, deltaY, viewportWidth, viewportHeight);
            } else {
                // Rotar ArcballCam
                _arcballCam->rotate(deltaX, deltaY, viewportWidth, viewportHeight);
            }
        }
        // Eliminado: Manejo de FreeCam
    }

    _mousePosition = currMousePosition;
}

//*************************************************************************************
//
// Engine Setup

void MP::mSetupGLFW() {
    CSCI441::OpenGLEngine::mSetupGLFW();

    // Establecer las callbacks
    glfwSetKeyCallback(mpWindow, A3_engine_keyboard_callback);
    glfwSetMouseButtonCallback(mpWindow, A3_engine_mouse_button_callback);
    glfwSetCursorPosCallback(mpWindow, A3_engine_cursor_callback);
}

void MP::mSetupOpenGL() {
    glEnable(GL_DEPTH_TEST);					                    // Habilitar pruebas de profundidad
    glDepthFunc(GL_LESS);							                // Usar prueba de profundidad "menor que"

    glEnable(GL_BLEND);									            // Habilitar blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	        // Usar ecuación de blending "source alpha"

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);	                    // Limpiar el buffer de color a negro
}

void MP::mSetupShaders() {
    // Obtener el Shader Program
    _lightingShaderProgram = new CSCI441::ShaderProgram("shaders/A3.v.glsl", "shaders/A3.f.glsl");

    // Uniformes generales del Shader
    _lightingShaderUniformLocations.mvpMatrix      = _lightingShaderProgram->getUniformLocation("mvpMatrix");
    _lightingShaderUniformLocations.normalMatrix   = _lightingShaderProgram->getUniformLocation("normalMatrix");
    _lightingShaderUniformLocations.eyePosition    = _lightingShaderProgram->getUniformLocation("eyePosition");

    // Atributos generales del Shader
    _lightingShaderAttributeLocations.vPos    = _lightingShaderProgram->getAttributeLocation("vPos");
    _lightingShaderAttributeLocations.vNormal = _lightingShaderProgram->getAttributeLocation("vNormal");

    // Colores del material
    _lightingShaderUniformLocations.materialAmbientColor   = _lightingShaderProgram->getUniformLocation("materialAmbientColor");
    _lightingShaderUniformLocations.materialDiffuseColor   = _lightingShaderProgram->getUniformLocation("materialDiffuseColor");
    _lightingShaderUniformLocations.materialSpecularColor  = _lightingShaderProgram->getUniformLocation("materialSpecularColor");
    _lightingShaderUniformLocations.materialShininess      = _lightingShaderProgram->getUniformLocation("materialShininess");

    // Uniformes de la luz direccional
    _lightingShaderUniformLocations.lightDirection     = _lightingShaderProgram->getUniformLocation("lightDirection");
    _lightingShaderUniformLocations.lightAmbientColor  = _lightingShaderProgram->getUniformLocation("lightAmbientColor");
    _lightingShaderUniformLocations.lightDiffuseColor  = _lightingShaderProgram->getUniformLocation("lightDiffuseColor");
    _lightingShaderUniformLocations.lightSpecularColor = _lightingShaderProgram->getUniformLocation("lightSpecularColor");

    // Uniformes de la luz puntual
    _lightingShaderUniformLocations.pointLightPos       = _lightingShaderProgram->getUniformLocation("pointLightPos");
    _lightingShaderUniformLocations.pointLightColor     = _lightingShaderProgram->getUniformLocation("pointLightColor");
    _lightingShaderUniformLocations.pointLightConstant  = _lightingShaderProgram->getUniformLocation("pointLightConstant");
    _lightingShaderUniformLocations.pointLightLinear    = _lightingShaderProgram->getUniformLocation("pointLightLinear");
    _lightingShaderUniformLocations.pointLightQuadratic = _lightingShaderProgram->getUniformLocation("pointLightQuadratic");

    // Uniformes del spotlight
    _lightingShaderUniformLocations.spotLightPos            = _lightingShaderProgram->getUniformLocation("spotLightPos");
    _lightingShaderUniformLocations.spotLightDirection      = _lightingShaderProgram->getUniformLocation("spotLightDirection");
    _lightingShaderUniformLocations.spotLightColor          = _lightingShaderProgram->getUniformLocation("spotLightColor");
    _lightingShaderUniformLocations.spotLightCutoff         = _lightingShaderProgram->getUniformLocation("spotLightCutoff");
    _lightingShaderUniformLocations.spotLightOuterCutoff    = _lightingShaderProgram->getUniformLocation("spotLightOuterCutoff");
    _lightingShaderUniformLocations.spotLightExponent       = _lightingShaderProgram->getUniformLocation("spotLightExponent");
    _lightingShaderUniformLocations.spotLightConstant       = _lightingShaderProgram->getUniformLocation("spotLightConstant");
    _lightingShaderUniformLocations.spotLightLinear         = _lightingShaderProgram->getUniformLocation("spotLightLinear");
    _lightingShaderUniformLocations.spotLightQuadratic      = _lightingShaderProgram->getUniformLocation("spotLightQuadratic");

    _setupSkybox();
}

void MP::mSetupBuffers() {
    CSCI441::setVertexAttributeLocations(_lightingShaderAttributeLocations.vPos, _lightingShaderAttributeLocations.vNormal);

    // Inicializar el modelo del héroe (Aaron_Inti)
    _pPlane = new Aaron_Inti(_lightingShaderProgram->getShaderProgramHandle(),
                             _lightingShaderUniformLocations.mvpMatrix,
                             _lightingShaderUniformLocations.normalMatrix);

    // Inicializar las monedas
    for(int i = 0; i < 4; ++i) {
        _coins[i] = new Coin(_lightingShaderProgram->getShaderProgramHandle(),
                             _lightingShaderUniformLocations.mvpMatrix,
                             _lightingShaderUniformLocations.normalMatrix);
    }

    // Inicializar los zombies
    for(int i = 0; i < NUM_ZOMBIES; ++i) {
        _zombies[i] = new Zombie(_lightingShaderProgram->getShaderProgramHandle(),
                                 _lightingShaderUniformLocations.mvpMatrix,
                                 _lightingShaderUniformLocations.normalMatrix);
    }
    _hudShaderProgram = new CSCI441::ShaderProgram("shaders/hud.v.glsl", "shaders/hud.f.glsl");
    _createGroundBuffers();

    float heartVertices[] = {
        // Posiciones    // Coordenadas de textura
        0.0f, 1.0f,      0.0f, 1.0f, // Arriba izquierda
        1.0f, 0.0f,      1.0f, 0.0f, // Abajo derecha
        0.0f, 0.0f,      0.0f, 0.0f, // Abajo izquierda

        0.0f, 1.0f,      0.0f, 1.0f, // Arriba izquierda
        1.0f, 1.0f,      1.0f, 1.0f, // Arriba derecha
        1.0f, 0.0f,      1.0f, 0.0f  // Abajo derecha
    };

    glGenVertexArrays(1, &_heartVAO);
    glGenBuffers(1, &_heartVBO);

    glBindVertexArray(_heartVAO);

    glBindBuffer(GL_ARRAY_BUFFER, _heartVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(heartVertices), heartVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0); // Asumiendo que el atributo posición está en location 0
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    // Coordenadas de textura
    glEnableVertexAttribArray(1); // Asumiendo que el atributo textura está en location 1
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);

    float winVertices[] = {
        // Posiciones    // Coordenadas de textura
        0.0f, 1.0f,      0.0f, 1.0f, // Arriba izquierda
        1.0f, 0.0f,      1.0f, 0.0f, // Abajo derecha
        0.0f, 0.0f,      0.0f, 0.0f, // Abajo izquierda

        0.0f, 1.0f,      0.0f, 1.0f, // Arriba izquierda
        1.0f, 1.0f,      1.0f, 1.0f, // Arriba derecha
        1.0f, 0.0f,      1.0f, 0.0f  // Abajo derecha
    };


    glGenVertexArrays(1, &_winVAO);
    glGenBuffers(1, &_winVBO);

    glBindVertexArray(_winVAO);

    glBindBuffer(GL_ARRAY_BUFFER, _winVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(winVertices), winVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0); // Asumiendo que el atributo posición está en location 0
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    // Coordenadas de textura
    glEnableVertexAttribArray(1); // Asumiendo que el atributo textura está en location 1
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);

    float lostVertices[] = {
        // Posiciones    // Coordenadas de textura
        0.0f, 1.0f,      0.0f, 1.0f, // Arriba izquierda
        1.0f, 0.0f,      1.0f, 0.0f, // Abajo derecha
        0.0f, 0.0f,      0.0f, 0.0f, // Abajo izquierda

        0.0f, 1.0f,      0.0f, 1.0f, // Arriba izquierda
        1.0f, 1.0f,      1.0f, 1.0f, // Arriba derecha
        1.0f, 0.0f,      1.0f, 0.0f  // Abajo derecha
    };

    glGenVertexArrays(1, &_lostVAO);
    glGenBuffers(1, &_lostVBO);

    glBindVertexArray(_lostVAO);

    glBindBuffer(GL_ARRAY_BUFFER, _lostVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lostVertices), lostVertices, GL_STATIC_DRAW);

    // Posiciones
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    // Coordenadas de textura
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);

}

void MP::_createGroundBuffers() {
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
    };
    Vertex groundQuad[4] = {
        { {-1.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f} },
        { { 1.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f} },
        { {-1.0f, 0.0f,  1.0f}, {0.0f, 1.0f, 0.0f} },
        { { 1.0f, 0.0f,  1.0f}, {0.0f, 1.0f, 0.0f} }
    };

    GLushort indices[4] = {0,1,2,3};

    _numGroundPoints = 4;

    glGenVertexArrays(1, &_groundVAO);
    glBindVertexArray(_groundVAO);

    GLuint vbods[2];
    glGenBuffers(2, vbods);
    glBindBuffer(GL_ARRAY_BUFFER, vbods[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundQuad), groundQuad, GL_STATIC_DRAW);

    // Hook up vertex normal attribute
    glEnableVertexAttribArray(_lightingShaderAttributeLocations.vPos);
    glVertexAttribPointer(_lightingShaderAttributeLocations.vPos, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)nullptr);

    glEnableVertexAttribArray(_lightingShaderAttributeLocations.vNormal);
    glVertexAttribPointer(_lightingShaderAttributeLocations.vNormal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbods[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void MP::mSetupScene() {
    _selectedCharacter = AARON_INTI;

    _arcballCam->setCameraView(
        glm::vec3(0.0f, 20.0f, 20.0f), // Posición del ojo
        glm::vec3(10.0f, 10.0f, 0.0f),    // Punto de mirada (posición del plano)
        CSCI441::Y_AXIS                 // Vector hacia arriba
    );

    _planePosition = glm::vec3(0.0f, 0.0f, 0.0f);
    _planeHeading = 0.0f;

    _updateIntiFirstPersonCamera();

    float cornerOffset = WORLD_SIZE - 8.0f; // Deja un margen desde el borde
    float coinHeight = 1.2f;

    // Configurar posiciones de las monedas
    _coinPositions[0] = glm::vec3(-cornerOffset, coinHeight, -cornerOffset); // Esquina inferior izquierda
    _coinPositions[1] = glm::vec3(cornerOffset, coinHeight, -cornerOffset);  // Esquina inferior derecha
    _coinPositions[2] = glm::vec3(-cornerOffset, coinHeight, cornerOffset);  // Esquina superior izquierda
    _coinPositions[3] = glm::vec3(cornerOffset, coinHeight, cornerOffset);   // Esquina superior derecha

    // Configurar posiciones de los zombies (2 en cada esquina)
    float zombieHeight = 1.5f; // Altura de los zombies
    float spacing = 2.0f;      // Espacio entre los dos zombies en cada esquina

    // Esquina inferior izquierda
    _zombiePositions[0] = glm::vec3(-cornerOffset - spacing, zombieHeight, -cornerOffset - spacing);
    _zombiePositions[1] = glm::vec3(-cornerOffset + spacing, zombieHeight, -cornerOffset + spacing);

    // Esquina inferior derecha
    _zombiePositions[2] = glm::vec3(cornerOffset - spacing, zombieHeight, -cornerOffset - spacing);
    _zombiePositions[3] = glm::vec3(cornerOffset + spacing, zombieHeight, -cornerOffset + spacing);

    // Esquina superior izquierda
    _zombiePositions[4] = glm::vec3(-cornerOffset - spacing, zombieHeight, cornerOffset - spacing);
    _zombiePositions[5] = glm::vec3(-cornerOffset + spacing, zombieHeight, cornerOffset + spacing);

    // Esquina superior derecha
    _zombiePositions[6] = glm::vec3(cornerOffset - spacing, zombieHeight, cornerOffset - spacing);
    _zombiePositions[7] = glm::vec3(cornerOffset + spacing, zombieHeight, cornerOffset + spacing);

    _zombiePositions[8] = glm::vec3(-WORLD_SIZE + 3.0f, zombieHeight, 0.0f);
    _zombiePositions[9] = glm::vec3(WORLD_SIZE - 3.0f, zombieHeight, 0.0f);
    _zombiePositions[8] = glm::vec3(0.0f, zombieHeight, -WORLD_SIZE + 3.0f);
    _zombiePositions[9] = glm::vec3(0.0f, zombieHeight, WORLD_SIZE - 3.0f);

    for(int i = 0; i < NUM_ZOMBIES; ++i) {
        if(_zombies[i] != nullptr) {
            _zombies[i]->position = _zombiePositions[i];
            _zombies[i]->rotationAngle = 0.0f;

            // Asignar multiplicadores de velocidad
            if(i % 3 == 0) {
                _zombies[i]->speedMultiplier = 0.7f; // Zombies más lentos
            } else {
                _zombies[i]->speedMultiplier = 1.0f; // Velocidad normal
            }
        }
    }

    // Configurar la matriz de proyección
    int width, height;
    glfwGetFramebufferSize(mpWindow, &width, &height);
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    _projectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 1000.0f);
    _cameraSpeed = glm::vec2(0.25f, 0.02f);

    // Propiedades de la luz direccional
    glm::vec3 lightDirection = glm::vec3(-1.0f, -1.0f, 1.0f);
    glm::vec3 lightAmbientColor = glm::vec3(0.2f, 0.2f, 0.2f);
    glm::vec3 lightDiffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 lightSpecularColor = glm::vec3(1.0f, 1.0f, 1.0f);

    // Uniformes de la luz direccional
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.lightDirection, lightDirection);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.lightAmbientColor, lightAmbientColor);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.lightDiffuseColor, lightDiffuseColor);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.lightSpecularColor, lightSpecularColor);

    // Propiedades de la luz puntual
    glm::vec3 pointLightPos = glm::vec3(0.0f, 2.0f, 0.0f);
    glm::vec3 pointLightColor = glm::vec3(0.9f, 0.8f, 0.4f);
    float pointLightConstant = 1.0f;
    float pointLightLinear = 0.7f;
    float pointLightQuadratic = 0.1f;

    // Uniformes de la luz puntual
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.pointLightPos, pointLightPos);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.pointLightColor, pointLightColor);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.pointLightConstant, pointLightConstant);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.pointLightLinear, pointLightLinear);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.pointLightQuadratic, pointLightQuadratic);

    // Propiedades del spotlight
    glm::vec3 spotLightPos = glm::vec3(-2.0f, 5.0f, -2.0f);
    glm::vec3 spotLightDirection = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 spotLightColor = glm::vec3(0.7f, 0.7f, 0.7f);
    float spotLightCutoff = glm::cos(glm::radians(15.0f));
    float spotLightOuterCutoff = glm::cos(glm::radians(20.0f));
    float spotLightExponent = 30.0f;
    float spotLightConstant = 1.0f;
    float spotLightLinear = 0.7f;
    float spotLightQuadratic = 0.1f;

    // Uniformes del spotlight
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.spotLightPos, spotLightPos);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.spotLightDirection, spotLightDirection);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.spotLightColor, spotLightColor);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.spotLightCutoff, spotLightCutoff);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.spotLightOuterCutoff, spotLightOuterCutoff);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.spotLightExponent, spotLightExponent);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.spotLightConstant, spotLightConstant);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.spotLightLinear, spotLightLinear);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.spotLightQuadratic, spotLightQuadratic);

    _setupSkybox();

    int heart_width, heart_height, nrChannels;
    unsigned char *data = stbi_load("textures/heart.png", &heart_width, &heart_height, &nrChannels, 0);
    if (data) {
        glGenTextures(1, &_heartTexture);
        glBindTexture(GL_TEXTURE_2D, _heartTexture);

        // Configurar los parámetros de la textura
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        GLenum format = GL_RGBA; // Asegúrate de que la imagen tenga canal alfa
        if (nrChannels == 3)
            format = GL_RGB;

        glTexImage2D(GL_TEXTURE_2D, 0, format, heart_width, heart_height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
    } else {
        std::cout << "Failed to load heart texture" << std::endl;
    }

    int win_width, win_height, win_nrChannels;
    unsigned char *win_data = stbi_load("textures/you_win.png", &win_width, &win_height, &win_nrChannels, 0);
    if (win_data) {
        glGenTextures(1, &_winTexture);
        glBindTexture(GL_TEXTURE_2D, _winTexture);

        // Configurar los parámetros de la textura
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        GLenum format = GL_RGBA;
        if (win_nrChannels == 3)
            format = GL_RGB;

        glTexImage2D(GL_TEXTURE_2D, 0, format, win_width, win_height, 0, format, GL_UNSIGNED_BYTE, win_data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(win_data);
    } else {
        std::cout << "Failed to load 'You Win' texture" << std::endl;
    }

    int lost_width, lost_height, lost_nrChannels;
    unsigned char *lost_data = stbi_load("textures/you_lost.png", &lost_width, &lost_height, &lost_nrChannels, 0);
    if (lost_data) {
        glGenTextures(1, &_lostTexture);
        glBindTexture(GL_TEXTURE_2D, _lostTexture);

        // Configurar los parámetros de la textura
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        GLenum format = GL_RGBA;
        if (lost_nrChannels == 3)
            format = GL_RGB;

        glTexImage2D(GL_TEXTURE_2D, 0, format, lost_width, lost_height, 0, format, GL_UNSIGNED_BYTE, lost_data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(lost_data);
    } else {
        std::cout << "Failed to load 'You Lost' texture" << std::endl;
    }
}

void MP::mCleanupShaders() {
    fprintf(stdout, "[INFO]: ...deleting Shaders.\n");
    delete _lightingShaderProgram;
    fprintf(stdout, "[INFO]: ...deleting Skybox Shaders.\n");
    delete _skyboxShaderProgram;
}

void MP::mCleanupBuffers() {
    fprintf(stdout, "[INFO]: ...deleting VAOs....\n");
    CSCI441::deleteObjectVAOs();
    glDeleteVertexArrays(1, &_groundVAO);
    glDeleteVertexArrays(1, &_skyboxVAO);
    glDeleteVertexArrays(1, &_heartVAO);

    fprintf(stdout, "[INFO]: ...deleting VBOs....\n");
    CSCI441::deleteObjectVBOs();
    glDeleteBuffers(1, &_skyboxVBO);
    glDeleteBuffers(1, &_heartVBO);

    fprintf(stdout, "[INFO]: ...deleting models..\n");
    delete _pPlane;
    glDeleteTextures(1, &_heartTexture);

    glDeleteVertexArrays(1, &_winVAO);
    glDeleteBuffers(1, &_winVBO);
    glDeleteTextures(1, &_winTexture);

    glDeleteVertexArrays(1, &_lostVAO);
    glDeleteBuffers(1, &_lostVBO);
    glDeleteTextures(1, &_lostTexture);
}

void MP::_renderScene(glm::mat4 viewMtx, glm::mat4 projMtx, glm::vec3 eyePosition) const {

    // Dibujar el Skybox
    glDepthFunc(GL_LEQUAL);
    _skyboxShaderProgram->useProgram();

    glm::mat4 view = glm::mat4(glm::mat3(viewMtx)); // Eliminar la traslación de la matriz de vista
    _skyboxShaderProgram->setProgramUniform("view", view);
    _skyboxShaderProgram->setProgramUniform("projection", projMtx);

    glBindVertexArray(_skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _skyboxTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);

    // Usar el shader de iluminación
    _lightingShaderProgram->useProgram();
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.eyePosition, eyePosition);

    //// INICIO DIBUJANDO EL PLANO DE TERRENO ////
    // Dibujar el plano de terreno
    glm::mat4 groundModelMtx = glm::scale(glm::mat4(1.0f), glm::vec3(WORLD_SIZE, 1.0f, WORLD_SIZE));
    _computeAndSendMatrixUniforms(groundModelMtx, viewMtx, projMtx);

    glm::vec3 groundAmbientColor = glm::vec3(0.25f, 0.25f, 0.25f);
    glm::vec3 groundDiffuseColor = glm::vec3(0.3f, 0.8f, 0.2f); // Color existente del terreno
    glm::vec3 groundSpecularColor = glm::vec3(0.0f, 0.0f, 0.0f); // Sin especular para el terreno
    float groundShininess = 0.1f;

    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.materialAmbientColor, groundAmbientColor);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.materialDiffuseColor, groundDiffuseColor);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.materialSpecularColor, groundSpecularColor);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.materialShininess, groundShininess);

    glBindVertexArray(_groundVAO);
    glDrawElements(GL_TRIANGLE_STRIP, _numGroundPoints, GL_UNSIGNED_SHORT, (void*)0);
    //// FIN DIBUJANDO EL PLANO DE TERRENO ////

    /// INICIO DIBUJANDO EL HERO (Aaron_Inti) ////
    glm::vec3 heroAmbientColor = glm::vec3(0.1f, 0.0f, 0.0f);
    glm::vec3 heroDiffuseColor = glm::vec3(0.7f, 0.0f, 0.0f);
    glm::vec3 heroSpecularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    float heroShininess = 32.0f;

    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.materialAmbientColor, heroAmbientColor);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.materialDiffuseColor, heroDiffuseColor);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.materialSpecularColor, heroSpecularColor);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.materialShininess, heroShininess);

    glm::mat4 heroModelMtx(1.0f);
    heroModelMtx = glm::translate(heroModelMtx, _planePosition);
    heroModelMtx = glm::translate(heroModelMtx, glm::vec3(0.0f, 1.3f, 0.0f));
    heroModelMtx = glm::rotate(heroModelMtx, _planeHeading, CSCI441::Y_AXIS);
    if (_isHeroFalling) {
        // Aplicar rotación adicional alrededor del eje X o Z para simular giro
        heroModelMtx = glm::rotate(heroModelMtx, _heroFallRotation, CSCI441::X_AXIS);
    }
    _pPlane->setDamaged(_isHeroDamaged);
    _pPlane->drawVehicle(heroModelMtx, viewMtx, projMtx);
    /// FIN DIBUJANDO EL HERO (Aaron_Inti) ////

    // Dibujar las monedas
    for (int i = 0; i < 4; ++i) {
        if (_coins[i]->isActive()) {
            glm::mat4 coinModelMtx(1.0f);
            coinModelMtx = glm::translate(coinModelMtx, _coinPositions[i]);
            // Si aplicaste alguna rotación antes, puedes mantenerla
            // coinModelMtx = glm::rotate(coinModelMtx, glm::radians(-90.0f), CSCI441::X_AXIS);
            _coins[i]->drawCoin(coinModelMtx, viewMtx, projMtx);
        }
    }

    /// INICIO DIBUJANDO LOS ZOMBIES ///
    glm::vec3 zombieAmbientColor = glm::vec3(0.0f, 0.1f, 0.0f); // Opcional: color ambiental general para zombies
    glm::vec3 zombieDiffuseColor = glm::vec3(0.0f, 0.0f, 1.0f); // Opcional: color difuso general para zombies
    glm::vec3 zombieSpecularColor = glm::vec3(0.5f, 0.5f, 0.5f); // Opcional: color especular general para zombies
    float zombieShininess = 32.0f;

    // Configurar los colores del material para los zombies
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.materialAmbientColor, zombieAmbientColor);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.materialDiffuseColor, zombieDiffuseColor);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.materialSpecularColor, zombieSpecularColor);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.materialShininess, zombieShininess);

    for(int i = 0; i < NUM_ZOMBIES; ++i) {
        if(_zombies[i] != nullptr && _zombies[i]->isActive) {
            glm::mat4 zombieModelMtx(1.0f);
            _zombies[i]->drawVehicle(zombieModelMtx, viewMtx, projMtx);
        }
    }
    /// FIN DIBUJANDO LOS ZOMBIES ///
}

void MP::_updateScene(float deltaTime) {
    float moveSpeed = 0.1f;
    float rotateSpeed = glm::radians(1.5f);

    const float MIN_X = -WORLD_SIZE + 3.0f;
    const float MAX_X = WORLD_SIZE - 3.0f;
    const float MIN_Z = -WORLD_SIZE + 3.0f;
    const float MAX_Z = WORLD_SIZE - 3.0f;

    switch (_currentCameraMode) {
        case ARCBALL:
            if (_selectedCharacter == AARON_INTI) {
                if (_keys[GLFW_KEY_W]) {
                    glm::vec3 direction(
                        sinf(_planeHeading),
                        0.0f,
                        cosf(_planeHeading)
                    );
                    glm::vec3 newPosition = _planePosition - direction * moveSpeed;
                    _planePosition = newPosition;
                    if (!_isHeroFalling && _isOutOfBounds(_planePosition,0.0f)) {
                        _isHeroFalling = true;
                        _heroFallRotation = 0.0f;
                    }
                    _pPlane->moveBackward();
                }
                if (_keys[GLFW_KEY_S]) {
                    glm::vec3 direction(
                        sinf(_planeHeading),
                        0.0f,
                        cosf(_planeHeading)
                    );
                    glm::vec3 newPosition = _planePosition + direction * moveSpeed;
                    _planePosition = newPosition;
                    if (!_isHeroFalling && _isOutOfBounds(_planePosition,0.0f)) {
                        _isHeroFalling = true;
                        _heroFallRotation = 0.0f;
                    }
                    _pPlane->moveForward();
                }
                if (_keys[GLFW_KEY_A]) {
                    _planeHeading += rotateSpeed;
                }
                if (_keys[GLFW_KEY_D]) {
                    _planeHeading -= rotateSpeed;
                }

                if (_planeHeading > glm::two_pi<float>())
                    _planeHeading -= glm::two_pi<float>();
                else if (_planeHeading < 0.0f)
                    _planeHeading += glm::two_pi<float>();

                float Y_OFFSET = 2.0f;
                glm::vec3 intiLookAtPoint = _planePosition + glm::vec3(0.0f, Y_OFFSET, 0.0f);
                _arcballCam->setLookAtPoint(intiLookAtPoint);
                _updateIntiFirstPersonCamera();
            }
            break;
        case FIRST_PERSON_CAM: {
            if (_selectedCharacter == AARON_INTI) {
                if (_keys[GLFW_KEY_W]) {
                    glm::vec3 direction(
                        sinf(_planeHeading),
                        0.0f,
                        cosf(_planeHeading)
                    );
                    glm::vec3 newPosition = _planePosition - direction * moveSpeed;
                    newPosition.x = std::max(MIN_X, std::min(newPosition.x, MAX_X));
                    newPosition.z = std::max(MIN_Z, std::min(newPosition.z, MAX_Z));
                    _planePosition = newPosition;
                    _pPlane->moveBackward();
                }
                if (_keys[GLFW_KEY_S]) {
                    glm::vec3 direction(
                        sinf(_planeHeading),
                        0.0f,
                        cosf(_planeHeading)
                    );
                    glm::vec3 newPosition = _planePosition + direction * moveSpeed;
                    newPosition.x = std::max(MIN_X, std::min(newPosition.x, MAX_X));
                    newPosition.z = std::max(MIN_Z, std::min(newPosition.z, MAX_Z));
                    _planePosition = newPosition;
                    _pPlane->moveForward();
                }
                if (_keys[GLFW_KEY_A]) {
                    _planeHeading += rotateSpeed;
                }
                if (_keys[GLFW_KEY_D]) {
                    _planeHeading -= rotateSpeed;
                }

                if (_planeHeading > glm::two_pi<float>())
                    _planeHeading -= glm::two_pi<float>();
                else if (_planeHeading < 0.0f)
                    _planeHeading += glm::two_pi<float>();

                float Y_OFFSET = 2.0f;
                glm::vec3 intiLookAtPoint = _planePosition + glm::vec3(0.0f, Y_OFFSET, 0.0f);
                _arcballCam->setLookAtPoint(intiLookAtPoint);
                _updateIntiFirstPersonCamera();
            }
            break;
        }
        default:
            break;
    }

    // Comprobar colisiones con las monedas
    for (int i = 0; i < 4; ++i) {
        // Solo comprobar monedas activas
        if (_coins[i]->isActive()) {
            // Calcular la distancia entre el héroe y la moneda
            glm::vec3 coinPosition = _coinPositions[i];
            glm::vec3 heroPosition = _planePosition;

            float distance = glm::distance(coinPosition, heroPosition);

            // Si la distancia es menor que un umbral, consideramos que hay colisión
            float collisionDistance = 2.5f; // Puedes ajustar este valor según el tamaño de tu héroe y moneda

            if (distance < collisionDistance) {
                // Desactivar la moneda
                _coins[i]->deactivate();

                // Opcional: Puedes llevar un conteo de monedas recogidas o desencadenar algún evento
                std::cout << "¡Moneda recogida!" << std::endl;
            }
        }
    }

    bool allCoinsCollected = true;
    for (int i = 0; i < 4; ++i) {
        if (_coins[i]->isActive()) {
            allCoinsCollected = false;
            break;
        }
    }

    if (allCoinsCollected && _gameState == PLAYING) {
        _gameState = WON;
        std::cout << "¡Has ganado el juego!" << std::endl;
    }

    if (_gameState == PLAYING) {
        _planePosition += _heroVelocity * deltaTime;
        if (!_isHeroFalling && _isOutOfBounds(_planePosition, 0.0f)) {
            _isHeroFalling = true;
            _heroFallRotation = 0.0f;
        }

        if (!_isHeroFalling) {
            _planePosition.y = 0.0f;
        }
        // Aplicar fricción para reducir gradualmente la velocidad
        _heroVelocity *= 0.98f;
        _heroVelocity.y = 0.0f;

        float zombieMargin = -1.0f;
        for (int i = 0; i < NUM_ZOMBIES; ++i) {
            if (_zombies[i] != nullptr) {
                if (!_zombies[i]->isFalling && _isOutOfBounds(_zombies[i]->position, zombieMargin)) {
                    _zombies[i]->isFalling = true;
                    _zombies[i]->fallRotation = 0.0f;
                }
            }
        }

        _moveZombies(deltaTime);
        _collideZombiesWithZombies();

        _collideHeroWithZombies(deltaTime);

        // Actualizar el temporizador de daño del héroe
        if(_isHeroDamaged) {
            _heroDamageTime += deltaTime;
            if(_heroDamageTime >= HERO_DAMAGE_DURATION) {
                _isHeroDamaged = false;
                _heroDamageTime = 0.0f;
            }
        }

        if (_isHeroFalling) {
            int ROTATION_SPEED = 2.0f;
            int FALL_SPEED = 5.0f;
            _heroFallRotation += deltaTime * ROTATION_SPEED; // Define ROTATION_SPEED, por ejemplo, 3.0f

            // Decrementar posición Y para simular la caída
            _planePosition.y -= deltaTime * FALL_SPEED; // Define FALL_SPEED, por ejemplo, 5.0f

            // Cuando alcance cierta altura, finalizar la caída
            if (_planePosition.y <= -60.0f) { // Altura límite
                _gameState = LOST;
                std::cout << "¡El héroe ha caído fuera del mundo! Game Over." << std::endl;
            }
        }
    }

}

void MP::run() {
    glfwSetWindowUserPointer(mpWindow, this);

    // Variables para manejar el tiempo
    double previousTime = glfwGetTime();

    while (!glfwWindowShouldClose(mpWindow)) {
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - previousTime);
        previousTime = currentTime;

        glDrawBuffer(GL_BACK);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glfwGetFramebufferSize(mpWindow, &framebufferWidth, &framebufferHeight);
        _hudProjection = glm::ortho(0.0f, static_cast<float>(framebufferWidth), 0.0f, static_cast<float>(framebufferHeight));

        glViewport(0, 0, framebufferWidth, framebufferHeight);
        float aspectRatio = static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight);
        _projectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 1000.0f);


        if (_gameState == PLAYING) {
            glm::mat4 viewMatrix;
            glm::vec3 eyePosition;

            if (_currentCameraMode == ARCBALL) {
                viewMatrix = _arcballCam->getViewMatrix();
                eyePosition = _arcballCam->getPosition();
            } else if (_currentCameraMode == FIRST_PERSON_CAM) {
                // Solo hay una cámara en primera persona para AARON_INTI
                viewMatrix = _intiFirstPersonCam->getViewMatrix();
                eyePosition = _intiFirstPersonCam->getPosition();
            }

            _renderScene(viewMatrix, _projectionMatrix, eyePosition);

            if (_isSmallViewportActive) {
                GLint prevViewport[4];
                glGetIntegerv(GL_VIEWPORT, prevViewport);

                glClear(GL_DEPTH_BUFFER_BIT);

                GLint smallViewportWidth = framebufferWidth / 3;
                GLint smallViewportHeight = framebufferHeight / 3;
                GLint smallViewportX = framebufferWidth - smallViewportWidth - 10;
                GLint smallViewportY = framebufferHeight - smallViewportHeight - 10;

                glViewport(smallViewportX, smallViewportY, smallViewportWidth, smallViewportHeight);

                float smallAspectRatio = static_cast<float>(smallViewportWidth) / static_cast<float>(smallViewportHeight);

                glm::mat4 smallProjectionMatrix = glm::perspective(glm::radians(45.0f), smallAspectRatio, 0.1f, 1000.0f);

                glm::mat4 fpViewMatrix = _intiFirstPersonCam->getViewMatrix();
                glm::vec3 fpEyePosition = _intiFirstPersonCam->getPosition();

                _renderScene(fpViewMatrix, smallProjectionMatrix, fpEyePosition);

                glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

            }
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // Dibujar el HUD
            _drawHUD();

            // Restaurar estado de OpenGL
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
            _updateScene(deltaTime);
        } else if (_gameState == WON) {
            // Renderizar la pantalla de victoria
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            _drawWinScreen();

            // Restaurar estado de OpenGL
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
        } else if (_gameState == LOST) {
            // Renderizar la pantalla de derrota
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            _drawLostScreen();

            // Restaurar estado de OpenGL
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
        }

        glfwSwapBuffers(mpWindow);
        glfwPollEvents();

    }
}


void MP::_collideHeroWithZombies(float deltaTime) {
    float heroRadius = 2.0f; // Ajusta según el tamaño de tu héroe

    for(int i = 0; i < NUM_ZOMBIES; ++i) {
        Zombie* zombie = _zombies[i];
        if(zombie == nullptr) continue;

        // Calcular la distancia entre el héroe y el zombie
        glm::vec3 delta = _planePosition - zombie->position;
        float distance = glm::length(delta);

        float sumRadius = heroRadius + zombie->radius;

        // Si hay colisión
        if(distance < sumRadius) {
            // Manejar colisión
            if(!_isHeroDamaged) {
                _isHeroDamaged = true;
                _heroDamageTime = 0.0f; // Iniciar temporizador de daño
                std::cout << "¡El héroe ha sido golpeado por un zombie!" << std::endl;

                // Decrementar vidas
                _heroLives--;
                if(_heroLives <= 0) {
                    std::cout << "¡El héroe ha perdido todas sus vidas!" << std::endl;
                    _gameState = LOST;
                }

                // Calcular dirección de empuje
                glm::vec3 pushDirection = glm::normalize(delta);
                float pushStrength = 80.0f; // Ajusta este valor para modificar la fuerza del rebote

                // Aplicar velocidad al héroe
                _heroVelocity += pushDirection * pushStrength;
            }
        }
    }
}

//*************************************************************************************
//
// Private Helper Functions

void MP::_updateIntiFirstPersonCamera() {
    glm::vec3 offset(0.0f, 4.0f, 0.0f);
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), _planeHeading, CSCI441::Y_AXIS);
    glm::vec3 rotatedOffset = glm::vec3(rotation * glm::vec4(offset, 0.0f));

    glm::vec3 cameraPosition = _planePosition + rotatedOffset;
    _intiFirstPersonCam->setPosition(cameraPosition);

    glm::vec3 facingDirection = glm::vec3(
        sinf(_planeHeading),
        0.0f,
        cosf(_planeHeading)
    );

    glm::vec3 backwardDirection = -facingDirection;
    glm::vec3 lookAtPoint = cameraPosition + backwardDirection;
    _intiFirstPersonCam->setLookAtPoint(lookAtPoint);
    _intiFirstPersonCam->computeViewMatrix();
}

GLuint MP::loadCubemap(const std::vector<std::string>& faces) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for(GLuint i = 0; i < faces.size(); i++) {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            GLenum format = GL_RGB;
            if (nrChannels == 1)
                format = GL_RED;
            else if (nrChannels == 3)
                format = GL_RGB;
            else if (nrChannels == 4)
                format = GL_RGBA;

            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else {
            fprintf(stderr, "Cubemap texture failed to load at path: %s\n", faces[i].c_str());
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void MP::_setupSkybox() {
    // Definir vértices para el cubo
    float skyboxVertices[] = {
        // posiciones
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    glGenVertexArrays(1, &_skyboxVAO);
    glGenBuffers(1, &_skyboxVBO);
    glBindVertexArray(_skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, _skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); // location = 0 en skybox.v.glsl
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    std::vector<std::string> faces{
        "textures/skybox/right.bmp",
        "textures/skybox/left.bmp",
        "textures/skybox/top.bmp",
        "textures/skybox/bottom.bmp",
        "textures/skybox/front.bmp",
        "textures/skybox/back.bmp"
    };
    _skyboxTexture = loadCubemap(faces);
    _skyboxShaderProgram = new CSCI441::ShaderProgram("shaders/skybox.v.glsl", "shaders/skybox.f.glsl");
    _skyboxShaderProgram->useProgram();
    _skyboxShaderProgram->setProgramUniform("skybox", 0);
}


void MP::_moveZombies(float deltaTime) {
    for(int i = 0; i < NUM_ZOMBIES; ++i) {
        if (_zombies[i] != nullptr && _zombies[i]->isActive) {
            _zombies[i]->update(deltaTime, _planePosition);
        }
    }
}

void MP::_collideZombiesWithZombies() {
    for(unsigned int i = 0; i < NUM_ZOMBIES; i++) {
        Zombie* zombie1 = _zombies[i];
        if(zombie1 == nullptr) continue;

        for(unsigned int j = i + 1; j < NUM_ZOMBIES; j++) {
            Zombie* zombie2 = _zombies[j];
            if(zombie2 == nullptr) continue;

            // Calcular la distancia entre los zombies
            glm::vec3 delta = zombie1->position - zombie2->position;
            float distance = glm::length(delta);

            // Calcular la suma de los radios
            float sumRadius = zombie1->radius + zombie2->radius;

            // Si hay colisión
            if(distance < sumRadius) {
                // Normalizar el vector de colisión
                glm::vec3 collisionNormal = glm::normalize(delta);

                // Calcular la velocidad relativa en la dirección normal
                glm::vec3 relativeVelocity = zombie1->velocity - zombie2->velocity;
                float velocityAlongNormal = glm::dot(relativeVelocity, collisionNormal);

                // Evitar calcular si las velocidades se están separando
                if(velocityAlongNormal > 0)
                    continue;

                // Coeficiente de restitución (elasticidad del rebote)
                float e = 0.5f; // Ajusta este valor entre 0 (inelástico) y 1 (elástico)

                // Calcular el impulso escalar
                float j = -(1 + e) * velocityAlongNormal;
                j /= 2; // Suponiendo masas iguales

                // Calcular el impulso vectorial
                glm::vec3 impulse = j * 80*collisionNormal;

                // Actualizar las velocidades de los zombies
                zombie1->velocity += impulse;
                zombie2->velocity -= impulse;

                // Separar los zombies para evitar superposición
                float penetrationDepth = sumRadius - distance;
                glm::vec3 correction = (penetrationDepth / 2.0f) * collisionNormal;
                zombie1->position += correction;
                zombie2->position -= correction;

                // Ajustar la rotación para que sigan mirando al héroe
                zombie1->rotationAngle = atan2f(-zombie1->velocity.x, -zombie1->velocity.z);
                zombie2->rotationAngle = atan2f(-zombie2->velocity.x, -zombie2->velocity.z);
            }
        }
    }
}


void MP::_drawHUD() {
    _hudShaderProgram->useProgram();
    _hudShaderProgram->setProgramUniform("projection", _hudProjection);

    glBindVertexArray(_heartVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _heartTexture);
    _hudShaderProgram->setProgramUniform("texture1", 0); // Asignamos la unidad de textura 0

    // Tamaño del corazón en píxeles
    float heartWidth = 50.0f;
    float heartHeight = 50.0f;

    for(int i = 0; i < _heroLives; ++i) {
        glm::mat4 model = glm::mat4(1.0f);

        // Posición del corazón
        float x = framebufferWidth - (i + 1) * (heartWidth + 10.0f); // Separación de 10 píxeles
        float y = framebufferHeight - heartHeight - 10.0f; // 10 píxeles desde el borde superior

        model = glm::translate(model, glm::vec3(x, y, 0.0f));
        model = glm::scale(model, glm::vec3(heartWidth, heartHeight, 1.0f));

        _hudShaderProgram->setProgramUniform("model", model);


        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    glBindVertexArray(0);
}

void MP::_drawWinScreen() {
    _hudShaderProgram->useProgram();
    _hudShaderProgram->setProgramUniform("projection", _hudProjection);

    glBindVertexArray(_winVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _winTexture);
    _hudShaderProgram->setProgramUniform("texture1", 0); // Asignamos la unidad de textura 0

    // Dibujar la imagen de "You Win" cubriendo toda la pantalla
    glm::mat4 model = glm::mat4(1.0f);

    // Ajustar el tamaño y posición para cubrir toda la pantalla
    float width = static_cast<float>(framebufferWidth);
    float height = static_cast<float>(framebufferHeight);

    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(width, height, 1.0f));

    _hudShaderProgram->setProgramUniform("model", model);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
}


void MP::_drawLostScreen() {
    _hudShaderProgram->useProgram();
    _hudShaderProgram->setProgramUniform("projection", _hudProjection);

    glBindVertexArray(_lostVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _lostTexture);
    _hudShaderProgram->setProgramUniform("texture1", 0); // Asignamos la unidad de textura 0

    // Dibujar la imagen de "You Lost" cubriendo toda la pantalla
    glm::mat4 model = glm::mat4(1.0f);

    // Ajustar el tamaño y posición para cubrir toda la pantalla
    float width = static_cast<float>(framebufferWidth);
    float height = static_cast<float>(framebufferHeight);

    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(width, height, 1.0f));

    _hudShaderProgram->setProgramUniform("model", model);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
}

void MP::_resetGame() {
    // Restablecer el estado del juego
    _gameState = PLAYING;
    _isHeroFalling = false;
    _heroFallRotation = 0.0f;
    // Restablecer la posición y orientación del héroe
    _planePosition = glm::vec3(0.0f, 0.0f, 0.0f);
    _planeHeading = 0.0f;
    _heroVelocity = glm::vec3(0.0f);
    _isHeroDamaged = false;
    _heroDamageTime = 0.0f;
    _heroLives = 5; // Restablecer las vidas del héroe

    // Restablecer las posiciones y estados de los zombies
    for(int i = 0; i < NUM_ZOMBIES; ++i) {
        if(_zombies[i] != nullptr) {
            _zombies[i]->isFalling = false;
            _zombies[i]->fallRotation = 0.0f;
            _zombies[i]->isActive = true;
            _zombies[i]->position = _zombiePositions[i];
            _zombies[i]->rotationAngle = 0.0f;
            _zombies[i]->velocity = glm::vec3(0.0f);

            // Asignar multiplicadores de velocidad nuevamente si es necesario
            if(i % 3 == 0) {
                _zombies[i]->speedMultiplier = 0.7f; // Zombies más lentos
            } else {
                _zombies[i]->speedMultiplier = 1.0f; // Velocidad normal
            }
        }
    }

    // Reactivar todas las monedas
    for(int i = 0; i < 4; ++i) {
        _coins[i] = new Coin(_lightingShaderProgram->getShaderProgramHandle(),
                             _lightingShaderUniformLocations.mvpMatrix,
                             _lightingShaderUniformLocations.normalMatrix);
    }

    // Restablecer la cámara si es necesario
    _arcballCam->setCameraView(
        glm::vec3(0.0f, 20.0f, 20.0f), // Posición del ojo
        glm::vec3(10.0f, 10.0f, 0.0f),    // Punto de mirada (posición del plano)
        CSCI441::Y_AXIS                 // Vector hacia arriba
    );
    _updateIntiFirstPersonCamera();
}

bool MP::_isOutOfBounds(const glm::vec3& position, float margin = 0.0f) {
    const float BOUNDARY = WORLD_SIZE - margin;
    return position.x < -BOUNDARY || position.x > BOUNDARY ||
           position.z < -BOUNDARY || position.z > BOUNDARY;
}

void MP::_computeAndSendMatrixUniforms(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const {
    // Precompute the Model-View-Projection matrix on the CPU
    glm::mat4 mvpMtx = projMtx * viewMtx * modelMtx;
    // Then send it to the shader on the GPU to apply to every vertex
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.mvpMatrix, mvpMtx);
    glm::mat3 normalMtx = glm::transpose(glm::inverse(glm::mat3(modelMtx)));
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.normalMatrix, normalMtx);
}

//*************************************************************************************
//
// Callbacks

void A3_engine_keyboard_callback(GLFWwindow *window, int key, int scancode, int action, int mods ) {
    auto engine = static_cast<MP*>(glfwGetWindowUserPointer(window));

    // Pasar la tecla y la acción al engine
    engine->handleKeyEvent(key, action);
}

void A3_engine_cursor_callback(GLFWwindow *window, double x, double y ) {
    auto engine = static_cast<MP*>(glfwGetWindowUserPointer(window));

    // Pasar la posición del cursor al engine
    engine->handleCursorPositionEvent(glm::vec2(static_cast<float>(x), static_cast<float>(y)));
}

void A3_engine_mouse_button_callback(GLFWwindow *window, int button, int action, int mods ) {
    auto engine = static_cast<MP*>(glfwGetWindowUserPointer(window));

    // Pasar el botón del mouse y la acción al engine
    engine->handleMouseButtonEvent(button, action);
}
