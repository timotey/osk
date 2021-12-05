#ifndef SRC_GL_HPP__
#define SRC_GL_HPP__
#include<GL/glew.h>
#include<glm/glm.hpp>

namespace gl{

struct draw_guard{
    draw_guard(GLenum i){glBegin(i);}
    draw_guard(const draw_guard &) = delete;
    draw_guard(draw_guard &&) = delete;
    draw_guard operator=(const draw_guard &) = delete;
    draw_guard operator=(draw_guard &&) = delete;
    ~draw_guard(){glEnd();}
};

struct matrix_guard{
    matrix_guard(){glPushMatrix();}
    matrix_guard(const matrix_guard &) = delete;
    matrix_guard(matrix_guard &&) = delete;
    matrix_guard operator=(const matrix_guard &) = delete;
    matrix_guard operator=(matrix_guard &&) = delete;
    ~matrix_guard(){glPopMatrix();}
};
    
}

#endif //SRC_GL_HPP__
