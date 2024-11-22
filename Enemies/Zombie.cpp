#include "Zombie.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <objects.hpp>
#include <OpenGLUtils.hpp>

Zombie::Zombie(GLuint shaderProgramHandle, GLint mvpMtxUniformLocation, GLint normalMtxUniformLocation)
    : _shaderProgramHandle(shaderProgramHandle),
    rotationAngle(0.0f){

    _shaderProgramUniformLocations.mvpMtx    = mvpMtxUniformLocation;
    _shaderProgramUniformLocations.normalMtx = normalMtxUniformLocation;

    _shaderProgramUniformLocations.materialAmbientColor  = glGetUniformLocation(_shaderProgramHandle, "materialAmbientColor");
    _shaderProgramUniformLocations.materialDiffuseColor  = glGetUniformLocation(_shaderProgramHandle, "materialDiffuseColor");
    _shaderProgramUniformLocations.materialSpecularColor = glGetUniformLocation(_shaderProgramHandle, "materialSpecularColor");
    _shaderProgramUniformLocations.materialShininess     = glGetUniformLocation(_shaderProgramHandle, "materialShininess");


    _colorBody = glm::vec3(0.0f, 0.0f, 1.0f); // Cuerpo azul
    _colorHead = glm::vec3(0.0f, 1.0f, 0.0f); // Cabeza verde
    _colorFace = glm::vec3(0.3f, 0.3f, 0.3f);
    _colorBag  = glm::vec3(0.6f, 0.6f, 0.6f);
    _colorArm  = glm::vec3(0.0f, 1.0f, 0.0f); // Brazos verdes

    position = glm::vec3(0.0f);
}

void Zombie::drawVehicle(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) {

    modelMtx = glm::translate(modelMtx, position);
    modelMtx = glm::rotate(modelMtx, rotationAngle, CSCI441::Y_AXIS);

    _drawBody(modelMtx, viewMtx, projMtx);
    _drawArmRight(modelMtx, viewMtx, projMtx);
    _drawArmLeft(modelMtx, viewMtx, projMtx);
    _drawHead(modelMtx, viewMtx, projMtx);
    _drawFace(modelMtx, viewMtx, projMtx);
    _drawCones(modelMtx, viewMtx, projMtx);
    _drawBag(modelMtx, viewMtx, projMtx);
}

void Zombie::moveForward() {

    if (_leftArmSwingForward) {
        _leftArmAngle += _armSwingSpeed;
        if (_leftArmAngle >= _armSwingLimit) {
            _leftArmSwingForward = false;
        }
    } else {
        _leftArmAngle -= _armSwingSpeed;
        if (_leftArmAngle <= -_armSwingLimit) {
            _leftArmSwingForward = true;
        }
    }

    if (_rightArmSwingForward) {
        _rightArmAngle += _armSwingSpeed;
        if (_rightArmAngle >= _armSwingLimit) {
            _rightArmSwingForward = false;
        }
    } else {
        _rightArmAngle -= _armSwingSpeed;
        if (_rightArmAngle <= -_armSwingLimit) {
            _rightArmSwingForward = true;
        }
    }
}

void Zombie::moveBackward() {
    if (_leftArmSwingForward) {
        _leftArmAngle += _armSwingSpeed;
        if (_leftArmAngle >= _armSwingLimit) {
            _leftArmSwingForward = false;
        }
    } else {
        _leftArmAngle -= _armSwingSpeed;
        if (_leftArmAngle <= -_armSwingLimit) {
            _leftArmSwingForward = true;
        }
    }

    if (_rightArmSwingForward) {
        _rightArmAngle += _armSwingSpeed;
        if (_rightArmAngle >= _armSwingLimit) {
            _rightArmSwingForward = false;
        }
    } else {
        _rightArmAngle -= _armSwingSpeed;
        if (_rightArmAngle <= -_armSwingLimit) {
            _rightArmSwingForward = true;
        }
    }
}

void Zombie::update(float deltaTime, glm::vec3 heroPosition) {
    // Calcular dirección hacia el héroe
    glm::vec3 direction = heroPosition - position;
    direction.y = 0.0f; // Ignorar componente Y para movimiento en plano XZ

    // Evitar división por cero
    if(glm::length(direction) < 0.1f) return;

    direction = glm::normalize(direction);

    // Calcular el ángulo deseado
    float desiredAngle = atan2f(-direction.x, -direction.z);

    // Calcular la diferencia de ángulo
    float angleDifference = desiredAngle - rotationAngle;

    // Asegurar que la diferencia de ángulo esté en el rango [-pi, pi]
    if(angleDifference > glm::pi<float>())
        angleDifference -= glm::two_pi<float>();
    if(angleDifference < -glm::pi<float>())
        angleDifference += glm::two_pi<float>();

    // Definir velocidad máxima de rotación
    float maxRotationSpeed = glm::radians(90.0f); // 90 grados por segundo

    // Limitar el cambio de rotación a la velocidad máxima
    float rotationChange = glm::clamp(angleDifference, -maxRotationSpeed * deltaTime, maxRotationSpeed * deltaTime);

    // Actualizar el ángulo de rotación
    rotationAngle += rotationChange;

    // Mantener rotationAngle dentro de [0, 2π]
    if(rotationAngle > glm::two_pi<float>())
        rotationAngle -= glm::two_pi<float>();
    else if(rotationAngle < 0.0f)
        rotationAngle += glm::two_pi<float>();

    // Actualizar posición
    float moveSpeed = 5.0f; // Unidades por segundo
    glm::vec3 forward(
        -sinf(rotationAngle),
        0.0f,
        -cosf(rotationAngle) // Invertimos el signo aquí
    );
    position += forward * moveSpeed * deltaTime;

    // Actualizar movimiento de los brazos
    moveForward();
}



void Zombie::_drawBody(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const {
    glm::mat4 bodyMtx = modelMtx;
    bodyMtx = glm::scale(bodyMtx, glm::vec3(0.8f, 2.0f, 0.5f));

    _computeAndSendMatrixUniforms(bodyMtx, viewMtx, projMtx);

    _setMaterialColors(_colorBody, 64.0f);

    CSCI441::drawSolidCube(1.0f);
}

void Zombie::_drawArmLeft(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const {
    glm::mat4 armMtx = modelMtx;

    armMtx = glm::translate(armMtx, glm::vec3(-0.55f, 0.7f, 0.0f));

    armMtx = glm::rotate(armMtx, _leftArmAngle, CSCI441::X_AXIS);

    armMtx = glm::translate(armMtx, glm::vec3(0.0f, -0.7f, 0.0f));

    armMtx = glm::scale(armMtx, glm::vec3(0.30f, 0.9f, 0.3f));

    _computeAndSendMatrixUniforms(armMtx, viewMtx, projMtx);

    _setMaterialColors(_colorArm, 64.0f); // Usar color de brazos

    CSCI441::drawSolidCube(1.0f);
}

void Zombie::_drawArmRight(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const {
    glm::mat4 armMtx = modelMtx;

    armMtx = glm::translate(armMtx, glm::vec3(0.55f, 0.7f, 0.0f));

    armMtx = glm::rotate(armMtx, _rightArmAngle, CSCI441::X_AXIS);

    armMtx = glm::translate(armMtx, glm::vec3(0.0f, -0.7f, 0.0f));

    armMtx = glm::scale(armMtx, glm::vec3(0.30f, 0.9f, 0.3f));

    _computeAndSendMatrixUniforms(armMtx, viewMtx, projMtx);

    _setMaterialColors(_colorArm, 64.0f); // Usar color de brazos

    CSCI441::drawSolidCube(1.0f);
}

void Zombie::_drawHead(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const {
    glm::mat4 headMtx = modelMtx;
    headMtx = glm::translate(headMtx, glm::vec3(0.0f, 1.1f, 0.0f));
    headMtx = glm::scale(headMtx, glm::vec3(0.8f));

    _computeAndSendMatrixUniforms(headMtx, viewMtx, projMtx);

    _setMaterialColors(_colorHead, 16.0f);

    CSCI441::drawSolidSphere(1.0f, 20, 20);
}

void Zombie::_drawFace(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const {
    glm::mat4 faceMtx = modelMtx;
    faceMtx = glm::translate(faceMtx, glm::vec3(0.0f, 1.1f, -0.18f));
    faceMtx = glm::scale(faceMtx, glm::vec3(0.7f));

    _computeAndSendMatrixUniforms(faceMtx, viewMtx, projMtx);

    _setMaterialColors(_colorFace, 16.0f);

    CSCI441::drawSolidSphere(1.0f, 20, 20);
}

void Zombie::_drawCones(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const {

    glm::mat4 coneRightMtx = modelMtx;
    coneRightMtx = glm::translate(coneRightMtx, glm::vec3(0.7f, 1.0f, 0.0f));
    coneRightMtx = glm::rotate(coneRightMtx, glm::radians(-90.0f), CSCI441::Z_AXIS);
    coneRightMtx = glm::scale(coneRightMtx, glm::vec3(0.25f, 0.6f, 0.25f));

    _computeAndSendMatrixUniforms(coneRightMtx, viewMtx, projMtx);

    _setMaterialColors(_colorFace, 16.0f);

    CSCI441::drawSolidCone(1.0f, 1.0f, 20, 20);


    glm::mat4 coneLeftMtx = modelMtx;
    coneLeftMtx = glm::translate(coneLeftMtx, glm::vec3(-0.7f, 1.0f, 0.0f));
    coneLeftMtx = glm::rotate(coneLeftMtx, glm::radians(90.0f), CSCI441::Z_AXIS);
    coneLeftMtx = glm::scale(coneLeftMtx, glm::vec3(0.25f, 0.6f, 0.25f));

    _computeAndSendMatrixUniforms(coneLeftMtx, viewMtx, projMtx);

    _setMaterialColors(_colorFace, 32.0f);

    CSCI441::drawSolidCone(1.0f, 1.0f, 20, 20);
}

void Zombie::_drawBag(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const {
    glm::mat4 bagMtx = modelMtx;
    bagMtx = glm::translate(bagMtx, glm::vec3(0.0f, 0.0f, 0.35f));
    bagMtx = glm::scale(bagMtx, glm::vec3(0.4f, 0.6f, 0.3f));

    _computeAndSendMatrixUniforms(bagMtx, viewMtx, projMtx);

    _setMaterialColors(_colorBag, 64.0f);

    CSCI441::drawSolidCube(1.0f);
}

void Zombie::_computeAndSendMatrixUniforms(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const {
    glm::mat4 mvpMtx = projMtx * viewMtx * modelMtx;
    glProgramUniformMatrix4fv(_shaderProgramHandle, _shaderProgramUniformLocations.mvpMtx, 1, GL_FALSE, glm::value_ptr(mvpMtx));

    glm::mat3 normalMtx = glm::transpose(glm::inverse(glm::mat3(modelMtx)));
    glProgramUniformMatrix3fv(_shaderProgramHandle, _shaderProgramUniformLocations.normalMtx, 1, GL_FALSE, glm::value_ptr(normalMtx));
}

void Zombie::_setMaterialColors(glm::vec3 color, float shininess) const {
    glm::vec3 ambientColor  = color * 0.2f;
    glm::vec3 diffuseColor  = color;
    glm::vec3 specularColor = glm::vec3(0.5f);

    glProgramUniform3fv(_shaderProgramHandle, _shaderProgramUniformLocations.materialAmbientColor, 1, glm::value_ptr(ambientColor));
    glProgramUniform3fv(_shaderProgramHandle, _shaderProgramUniformLocations.materialDiffuseColor, 1, glm::value_ptr(diffuseColor));
    glProgramUniform3fv(_shaderProgramHandle, _shaderProgramUniformLocations.materialSpecularColor, 1, glm::value_ptr(specularColor));
    glProgramUniform1f(_shaderProgramHandle, _shaderProgramUniformLocations.materialShininess, shininess);
}
