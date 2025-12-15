//
// Created by madoka on 2025/12/15.
//

#ifndef MADOKAWAII_SHAPE_H
#define MADOKAWAII_SHAPE_H

namespace Madokawaii::Platform::Shape {
    // Rectangle, 4 components
    typedef struct Rectangle {
        float x;                // Rectangle top-left corner position x
        float y;                // Rectangle top-left corner position y
        float width;            // Rectangle width
        float height;           // Rectangle height
    } Rectangle;
}

#endif //MADOKAWAII_SHAPE_H