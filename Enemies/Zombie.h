#ifndef ZOMBIE_H
#define ZOMBIE_H

#include <glad/gl.h> // Mantener esta inclusión
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Zombie {
public:
    Zombie(GLuint shaderProgramHandle, GLint mvpMtxUniformLocation, GLint normalMtxUniformLocation);

    void drawVehicle(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx);

    void moveForward();
    void moveBackward();

    glm::vec3 getPosition() const { return position; }
    float getRotationAngle() const { return rotationAngle; }

    /**
     * @brief Actualiza el estado del zombie.
     * @param deltaTime Tiempo transcurrido desde la última actualización.
     */
    void update(float deltaTime, glm::vec3 heroPosition); // Declaración del método update
    glm::vec3 position;
    float rotationAngle = 0.0f;

private:
    GLuint _shaderProgramHandle;
    struct ShaderProgramUniformLocations {
        GLint mvpMtx;
        GLint normalMtx;
        GLint materialAmbientColor;
        GLint materialDiffuseColor;
        GLint materialSpecularColor;
        GLint materialShininess;
    } _shaderProgramUniformLocations;

    glm::vec3 _colorBody;
    glm::vec3 _colorHead;
    glm::vec3 _colorFace;
    glm::vec3 _colorBag;
    glm::vec3 _colorArm; // Nueva variable para el color de los brazos



    void _drawBody(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const;
    void _drawArmRight(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const;
    void _drawArmLeft(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const;
    void _drawHead(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const;
    void _drawFace(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const;
    void _drawCones(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const;
    void _drawBag(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const;

    void _computeAndSendMatrixUniforms(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const;
    void _setMaterialColors(glm::vec3 color, float shininess) const;

    float _leftArmAngle = 0.0f;
    float _rightArmAngle = 0.0f;
    bool _leftArmSwingForward = true;
    bool _rightArmSwingForward = false;
    float _armSwingSpeed = glm::radians(1.0f);
    float _armSwingLimit = glm::radians(30.0f);
};

#endif //ZOMBIE_H
