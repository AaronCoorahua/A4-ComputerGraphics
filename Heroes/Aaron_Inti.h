#ifndef LAB05_PLANE_H
#define LAB05_PLANE_H

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

class Aaron_Inti {
public:
    Aaron_Inti(GLuint shaderProgramHandle, GLint mvpMtxUniformLocation, GLint normalMtxUniformLocation);

    void drawVehicle( glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx );

    void moveForward();
    void moveBackward();
    void setDamaged(bool isDamaged);
private:

    bool _isDamaged;
    glm::vec3 _colorWindow;
    glm::vec3 _scaleWindow;
    glm::vec3 _windowPositions[2];

    GLfloat _propAngle;
    GLfloat _propAngleRotationSpeed;

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
    glm::vec3 _scaleBody;

    glm::vec3 _colorTop;
    glm::vec3 _scaleTop;
    glm::vec3 _transTop;

    glm::vec3 _colorWheel;
    glm::vec3 _scaleWheel;
    glm::vec3 _wheelPositions[4];

    glm::vec3 _colorProp;
    glm::vec3 _scaleProp;
    glm::vec3 _transProp;

    glm::vec3 _colorHeadlightOn;
    glm::vec3 _colorHeadlightOff;
    glm::vec3 _scaleHeadlight;
    glm::vec3 _headlightPositions[2];
    bool _headlightState;
    GLfloat _headlightToggleTime;
    glm::vec3 _colorHeadlightReverse;
    bool _isMovingBackward;


    const GLfloat _PI = glm::pi<float>();
    const GLfloat _2PI = glm::two_pi<float>();
    const GLfloat _PI_OVER_2 = glm::half_pi<float>();

    void _drawCarBody(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx ) const;
    void _drawCarTop(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx ) const;
    void _drawCarWheels(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx ) const;
    void _drawCarPropeller(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx ) const;
    void _drawCarHeadlights(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx );
    void _drawCarWindows(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx ) const;

    void _computeAndSendMatrixUniforms(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const;
    void _setMaterialColors(glm::vec3 color, float shininess) const;
};

#endif //LAB05_PLANE_H
