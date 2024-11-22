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
      _isSmallViewportActive(false)
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
                float sensitivity = 1.0f; // Ajustar sensibilidad
                _arcballCam->zoom(deltaY * sensitivity);
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

    _createGroundBuffers();
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
        glm::vec3(0.0f, 50.0f, 100.0f), // Posición del ojo
        glm::vec3(0.0f, 0.0f, 0.0f),    // Punto de mirada (posición del plano)
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

    for(int i = 0; i < NUM_ZOMBIES; ++i) {
        if(_zombies[i] != nullptr) {
            _zombies[i]->position = _zombiePositions[i];
            _zombies[i]->rotationAngle = 0.0f;
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

    fprintf(stdout, "[INFO]: ...deleting VBOs....\n");
    CSCI441::deleteObjectVBOs();
    glDeleteBuffers(1, &_skyboxVBO);

    fprintf(stdout, "[INFO]: ...deleting models..\n");
    delete _pPlane;
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
        if(_zombies[i] != nullptr) {
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

    // Opcional: Actualizar zombies si tienen comportamientos
    for(int i = 0; i < NUM_ZOMBIES; ++i) {
        if(_zombies[i] != nullptr) {
            _zombies[i]->update(deltaTime, _planePosition); // Pasamos la posición del héroe
        }
    }

    _moveZombies(deltaTime);
    _collideZombiesWithZombies();
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

        GLint framebufferWidth, framebufferHeight;
        glfwGetFramebufferSize(mpWindow, &framebufferWidth, &framebufferHeight);

        glViewport(0, 0, framebufferWidth, framebufferHeight);
        float aspectRatio = static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight);
        _projectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 1000.0f);

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

        _updateScene(deltaTime);

        glfwSwapBuffers(mpWindow);
        glfwPollEvents();
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
        if(_zombies[i] != nullptr) {
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
                glm::vec3 impulse = j * collisionNormal;

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
