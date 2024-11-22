#include "Aaron_Inti.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <objects.hpp>
#include <OpenGLUtils.hpp>

Aaron_Inti::Aaron_Inti(GLuint shaderProgramHandle, GLint mvpMtxUniformLocation, GLint normalMtxUniformLocation)
    : _shaderProgramHandle(shaderProgramHandle), _isDamaged(false) {
    _propAngle = 0.0f;
    _propAngleRotationSpeed = _PI / 16.0f;

    _shaderProgramUniformLocations.mvpMtx    = mvpMtxUniformLocation;
    _shaderProgramUniformLocations.normalMtx = normalMtxUniformLocation;

    _shaderProgramUniformLocations.materialAmbientColor  = glGetUniformLocation(_shaderProgramHandle, "materialAmbientColor");
    _shaderProgramUniformLocations.materialDiffuseColor  = glGetUniformLocation(_shaderProgramHandle, "materialDiffuseColor");
    _shaderProgramUniformLocations.materialSpecularColor = glGetUniformLocation(_shaderProgramHandle, "materialSpecularColor");
    _shaderProgramUniformLocations.materialShininess     = glGetUniformLocation(_shaderProgramHandle, "materialShininess");

    _colorBody = glm::vec3(1.0f, 1.0f, 1.0f);
    _scaleBody = glm::vec3(2.0f, 1.5f, 6.0f);

    _colorTop = glm::vec3(0.5f, 0.5f, 0.5f);
    _scaleTop = glm::vec3(1.5f, 1.0f, 4.0f);
    _transTop = glm::vec3(0.0f, 1.25f, 0.5f);

    _colorWheel = glm::vec3(0.0f, 0.0f, 0.0f);
    _scaleWheel = glm::vec3(1.0f, 1.0f, 1.0f);

    _wheelPositions[0] = glm::vec3(1.0f, -0.75f, 2.5f);
    _wheelPositions[1] = glm::vec3(-1.2f, -0.75f, 2.5f);
    _wheelPositions[2] = glm::vec3(1.0f, -0.75f, -2.5f);
    _wheelPositions[3] = glm::vec3(-1.2f, -0.75f, -2.5f);

    _colorProp = glm::vec3(1.0f, 1.0f, 1.0f);
    _scaleProp = glm::vec3(1.5f, 0.2f, 0.1f);
    _transProp = glm::vec3(0.0f, 0.5f, 3.1f);

    _colorHeadlightOn = glm::vec3(1.0f, 1.0f, 0.0f);
    _colorHeadlightOff = glm::vec3(0.7f, 0.7f, 0.0f);
    _scaleHeadlight = glm::vec3(0.4f, 0.4f, 0.4f);
    _headlightPositions[0] = glm::vec3(0.8f, 0.2f, -3.0f);
    _headlightPositions[1] = glm::vec3(-0.8f, 0.2f, -3.0f);
    _headlightState = true;
    _headlightToggleTime = 0.0f;

    _colorWindow = glm::vec3(0.4f, 0.8f, 1.0f);
    _scaleWindow = glm::vec3(0.1f, 0.5f, 2.0f);
    _windowPositions[0] = glm::vec3(0.8f, 1.0f, 0.0f);
    _windowPositions[1] = glm::vec3(-0.8f, 1.0f, 0.0f);

    _colorHeadlightReverse = glm::vec3(1.0f, 0.0f, 0.0f);
    _isMovingBackward = false;
}

void Aaron_Inti::drawVehicle(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) {
    _drawCarBody(modelMtx, viewMtx, projMtx);
    _drawCarTop(modelMtx, viewMtx, projMtx);
    _drawCarWindows(modelMtx, viewMtx, projMtx);
    _drawCarWheels(modelMtx, viewMtx, projMtx);
    _drawCarPropeller(modelMtx, viewMtx, projMtx);
    _drawCarHeadlights(modelMtx, viewMtx, projMtx);
}

void Aaron_Inti::moveForward() {
    _isMovingBackward = false;
    _propAngle += _propAngleRotationSpeed;
    if (_propAngle > _2PI) _propAngle -= _2PI;

    _headlightToggleTime += 0.2f;
    if (_headlightToggleTime >= 1.0f) {
        _headlightToggleTime = 0.0f;
        _headlightState = !_headlightState;
    }
}

void Aaron_Inti::moveBackward() {
    _isMovingBackward = true;
    _propAngle -= _propAngleRotationSpeed;
    if (_propAngle < 0.0f) _propAngle += _2PI;

    _headlightToggleTime += 0.2f;
    if (_headlightToggleTime >= 1.0f) {
        _headlightToggleTime = 0.0f;
        _headlightState = !_headlightState;
    }
}

void Aaron_Inti::_drawCarBody(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const {
    glm::mat4 bodyMtx = modelMtx;
    bodyMtx = glm::scale(bodyMtx, _scaleBody);

    _computeAndSendMatrixUniforms(bodyMtx, viewMtx, projMtx);

    _setMaterialColors(_colorBody, 32.0f);

    CSCI441::drawSolidCube(1.0f);
}

void Aaron_Inti::_drawCarTop(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const {
    glm::mat4 topMtx = modelMtx;
    topMtx = glm::translate(topMtx, _transTop);
    topMtx = glm::scale(topMtx, _scaleTop);

    _computeAndSendMatrixUniforms(topMtx, viewMtx, projMtx);

    _setMaterialColors(_colorTop, 16.0f);

    CSCI441::drawSolidCube(1.0f);
}

void Aaron_Inti::_drawCarWheels(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const {
    int numSpokes = 8;
    for (int i = 0; i < 4; ++i) {
        glm::mat4 wheelMtx = modelMtx;
        wheelMtx = glm::translate(wheelMtx, _wheelPositions[i]);
        wheelMtx = glm::rotate(wheelMtx, glm::radians(-90.0f), CSCI441::Z_AXIS);
        wheelMtx = glm::rotate(wheelMtx, _propAngle, CSCI441::Y_AXIS);
        wheelMtx = glm::scale(wheelMtx, _scaleWheel);

        _computeAndSendMatrixUniforms(wheelMtx, viewMtx, projMtx);

        _setMaterialColors(_colorWheel, 10.0f);

        CSCI441::drawSolidCylinder(0.5f, 0.5f, 0.2f, 16, 16);

        for (int j = 0; j < numSpokes; ++j) {
            glm::mat4 spokeMtx = wheelMtx;
            spokeMtx = glm::rotate(spokeMtx, glm::radians(360.0f / numSpokes * j), CSCI441::Z_AXIS);
            spokeMtx = glm::translate(spokeMtx, glm::vec3(0.0f, 0.0f, 0.25f));
            spokeMtx = glm::scale(spokeMtx, glm::vec3(0.05f, 0.05f, 0.5f));

            _computeAndSendMatrixUniforms(spokeMtx, viewMtx, projMtx);

            CSCI441::drawSolidCube(1.0f);
        }
    }
}

void Aaron_Inti::_drawCarPropeller(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const {
    glm::mat4 propMtx = modelMtx;
    propMtx = glm::translate(propMtx, _transProp);
    propMtx = glm::rotate(propMtx, _propAngle, CSCI441::Z_AXIS);
    propMtx = glm::scale(propMtx, _scaleProp);

    _computeAndSendMatrixUniforms(propMtx, viewMtx, projMtx);

    _setMaterialColors(_colorProp, 32.0f);

    CSCI441::drawSolidCube(1.0f);
}

void Aaron_Inti::_drawCarHeadlights(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) {
    glm::vec3 headlightColor;

    if (_isMovingBackward) {
        headlightColor = _headlightState ? _colorHeadlightOn : _colorHeadlightOff;
    } else {
        headlightColor = _colorHeadlightReverse;
    }

    for (int i = 0; i < 2; ++i) {
        glm::mat4 lightMtx = modelMtx;
        lightMtx = glm::translate(lightMtx, _headlightPositions[i]);
        lightMtx = glm::scale(lightMtx, _scaleHeadlight);
        _computeAndSendMatrixUniforms(lightMtx, viewMtx, projMtx);

        _setMaterialColors(headlightColor, 64.0f);

        CSCI441::drawSolidCube(1.0f);
    }
}

void Aaron_Inti::_drawCarWindows(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const {
    for (int i = 0; i < 2; ++i) {
        glm::mat4 windowMtx = modelMtx;

        windowMtx = glm::translate(windowMtx, _windowPositions[i]);

        windowMtx = glm::scale(windowMtx, _scaleWindow);

        _computeAndSendMatrixUniforms(windowMtx, viewMtx, projMtx);

        _setMaterialColors(_colorWindow, 16.0f);

        CSCI441::drawSolidCube(1.0f);
    }
}

void Aaron_Inti::_computeAndSendMatrixUniforms(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const {
    glm::mat4 mvpMtx = projMtx * viewMtx * modelMtx;
    glProgramUniformMatrix4fv(_shaderProgramHandle, _shaderProgramUniformLocations.mvpMtx, 1, GL_FALSE, glm::value_ptr(mvpMtx));

    glm::mat3 normalMtx = glm::mat3(glm::transpose(glm::inverse(modelMtx)));
    glProgramUniformMatrix3fv(_shaderProgramHandle, _shaderProgramUniformLocations.normalMtx, 1, GL_FALSE, glm::value_ptr(normalMtx));
}

void Aaron_Inti::_setMaterialColors(glm::vec3 color, float shininess) const {
    if (_isDamaged) {
        color = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    glm::vec3 ambientColor  = color * 0.2f;
    glm::vec3 diffuseColor  = color;
    glm::vec3 specularColor = glm::vec3(0.5f);

    glProgramUniform3fv(_shaderProgramHandle, _shaderProgramUniformLocations.materialAmbientColor, 1, glm::value_ptr(ambientColor));
    glProgramUniform3fv(_shaderProgramHandle, _shaderProgramUniformLocations.materialDiffuseColor, 1, glm::value_ptr(diffuseColor));
    glProgramUniform3fv(_shaderProgramHandle, _shaderProgramUniformLocations.materialSpecularColor, 1, glm::value_ptr(specularColor));
    glProgramUniform1f(_shaderProgramHandle, _shaderProgramUniformLocations.materialShininess, shininess);
}


void Aaron_Inti::setDamaged(bool isDamaged) {
    _isDamaged = isDamaged;
}