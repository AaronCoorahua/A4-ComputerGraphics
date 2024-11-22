#ifndef MP_ENGINE_H
#define MP_ENGINE_H

#include "Cameras/ArcballCam.h"
#include <OpenGLEngine.hpp>
#include <ShaderProgram.hpp>
#include "FreeCam.hpp"

#include "Heroes/Aaron_Inti.h"
#include "Coin.h"
#include "Enemies/Zombie.h" // Incluir el header de Zombie

#include "stb_image.h"
#include <glad/gl.h>
#include <vector>
#include <string>

/**
 * @class MP
 * @brief Clase principal del motor que gestiona la escena, cámaras, iluminación y objetos.
 */
class MP final : public CSCI441::OpenGLEngine {
public:
    /**
     * @brief Constructor de la clase MP.
     */
    MP();

    /**
     * @brief Destructor de la clase MP.
     */
    ~MP() final;

    /**
     * @brief Ejecuta el motor de OpenGL.
     */
    void run() final;

    /**
     * @brief Maneja eventos de teclado.
     *
     * @param key Tecla presionada.
     * @param action Acción de la tecla.
     */
    void handleKeyEvent(GLint key, GLint action);

    /**
     * @brief Maneja eventos de botón del mouse.
     *
     * @param button Botón del mouse.
     * @param action Acción del botón.
     */
    void handleMouseButtonEvent(GLint button, GLint action);

    /**
     * @brief Maneja eventos de movimiento del cursor.
     *
     * @param currMousePosition Posición actual del cursor.
     */
    void handleCursorPositionEvent(glm::vec2 currMousePosition);

    static constexpr GLfloat MOUSE_UNINITIALIZED = -9999.0f;

private:
    bool _isOutOfBounds(const glm::vec3& position, float margin);
    bool _isHeroFalling = false;
    float _heroFallRotation = 0.0f;
    void _drawLostScreen();
    GLuint _lostTexture;
    GLuint _lostVAO = 0;
    GLuint _lostVBO = 0;
    void _resetGame();
    void _drawWinScreen();
    GLuint _winTexture;
    GLuint _winVAO = 0;
    GLuint _winVBO = 0;
    enum GameState { PLAYING, WON, LOST };
    GameState _gameState;
    int framebufferWidth, framebufferHeight;
    void _drawHUD();
    int _heroLives = 3;
    CSCI441::ShaderProgram* _hudShaderProgram = nullptr;
    GLuint _heartVAO = 0;
    GLuint _heartVBO = 0;
    glm::mat4 _hudProjection;
    // COINS

    Coin* _coins[4]; // Arreglo para almacenar las cuatro monedas
    glm::vec3 _coinPositions[4];

    // ZOMBIES
    static constexpr int NUM_ZOMBIES = 12;
    Zombie* _zombies[NUM_ZOMBIES]; // Arreglo para almacenar los ocho zombies
    glm::vec3 _zombiePositions[NUM_ZOMBIES]; // Arreglo para las posiciones de los zombies

    struct CameraFrame {
        glm::vec3 eye;
        glm::vec3 direction;
        glm::vec3 up;
        float fov;
    };

    // Personajes
    enum Character { AARON_INTI };
    Character _selectedCharacter;

    glm::mat4 _projectionMatrix;
    glm::vec3 _planePosition;
    float _planeHeading;

    void mSetupGLFW() final;
    void mSetupOpenGL() final;
    void mSetupShaders() final;
    void mSetupBuffers() final;
    void mSetupScene() final;

    void mCleanupBuffers() final;
    void mCleanupShaders() final;

    // Dibuja la escena desde un punto de vista específico de la cámara
    void _renderScene(glm::mat4 viewMtx, glm::mat4 projMtx, glm::vec3 eyePosition) const;

    // Actualiza elementos de la escena basados en el tiempo y la entrada
    void _updateScene(float deltaTime);

    static constexpr GLuint NUM_KEYS = GLFW_KEY_LAST;
    GLboolean _keys[NUM_KEYS];

    glm::vec2 _mousePosition;
    GLint _leftMouseButtonState;

    enum CameraMode { ARCBALL, FIRST_PERSON_CAM } _currentCameraMode;

    ArcballCam* _arcballCam;
    CSCI441::FreeCam* _intiFirstPersonCam;
    glm::vec2 _cameraSpeed;

    Aaron_Inti* _pPlane;


    static constexpr GLfloat WORLD_SIZE = 105.0f;
    GLuint _groundVAO;
    GLsizei _numGroundPoints;

    void _createGroundBuffers();

    CSCI441::ShaderProgram* _lightingShaderProgram = nullptr;

    struct LightingShaderUniformLocations {
        GLint mvpMatrix;
        GLint normalMatrix;
        GLint eyePosition;

        GLint lightDirection;
        GLint lightAmbientColor;
        GLint lightDiffuseColor;
        GLint lightSpecularColor;

        GLint materialAmbientColor;
        GLint materialDiffuseColor;
        GLint materialSpecularColor;
        GLint materialShininess;

        GLint pointLightPos;
        GLint pointLightColor;
        GLint pointLightConstant;
        GLint pointLightLinear;
        GLint pointLightQuadratic;

        GLint spotLightPos;
        GLint spotLightDirection;
        GLint spotLightColor;
        GLint spotLightCutoff;
        GLint spotLightOuterCutoff;
        GLint spotLightExponent;
        GLint spotLightConstant;
        GLint spotLightLinear;
        GLint spotLightQuadratic;
    } _lightingShaderUniformLocations;

    struct LightingShaderAttributeLocations {
        GLint vPos;
        GLint vNormal;
    } _lightingShaderAttributeLocations;

    void _computeAndSendMatrixUniforms(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const;

    bool _isShiftPressed;
    bool _isLeftMouseButtonPressed;
    bool _isZooming;

    glm::vec2 _prevMousePosition;

    void _updateIntiFirstPersonCamera();

    // SKYBOX

    bool _isSmallViewportActive;
    static constexpr int NUM_CHARACTERS = 1;

    GLuint _skyboxVAO, _skyboxVBO;
    GLuint _skyboxTexture;
    CSCI441::ShaderProgram* _skyboxShaderProgram = nullptr;

    GLuint loadCubemap(const std::vector<std::string>& faces);
    void _setupSkybox();
    void _moveZombies(float deltaTime);
    void _collideZombiesWithZombies();

    glm::vec3 _heroVelocity;
    bool _isHeroDamaged;
    float _heroDamageTime;
    const float HERO_DAMAGE_DURATION = 1.0f;
    void _collideHeroWithZombies(float deltaTime);

    GLuint _heartTexture;
};

// Declaración de las funciones de callback
void A3_engine_keyboard_callback(GLFWwindow *window, int key, int scancode, int action, int mods );
void A3_engine_cursor_callback(GLFWwindow *window, double x, double y );
void A3_engine_mouse_button_callback(GLFWwindow *window, int button, int action, int mods );

#endif // MP_ENGINE_H
