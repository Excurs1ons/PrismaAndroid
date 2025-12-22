//
// Created by jasonngu on 2025/12/22.
//

#ifndef MY_APPLICATION_ISHADER_H
#define MY_APPLICATION_ISHADER_H

#include <string>

class IShader{
    template<class T>
    static T *loadShader(
            const std::string &vertexSource,
            const std::string &fragmentSource,
            const std::string &positionAttributeName,
            const std::string &uvAttributeName,
            const std::string &projectionMatrixUniformName);

};
#endif //MY_APPLICATION_ISHADER_H
