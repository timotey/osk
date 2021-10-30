#include<utility>
#include<stdexcept>
#include<iostream>
#include<array>
#include<cmath>
#include<GL/glew.h>
#include<GLFW/glfw3.h>
#include<glm/glm.hpp>
#include<ft2build.h>
#include FT_FREETYPE_H
#include"mdspan.hpp"

struct box{
    glm::vec2 begin;
    glm::vec2 end;
};

template<class F>
struct defer{
    F f;
    ~defer(){
        f();
    }
};
template<class T>
defer(T)->defer<T>;

glm::vec2 polar(glm::vec2 cart){
    return{std::sqrt(cart.x*cart.x + cart.y*cart.y), std::atan2(cart.y, cart.x)};
}

float ar = 0.5;
double const pi = 3.14592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282;

void draw_trapezoid(box place, box texture[2], float progress){
    auto unitl = glm::vec2(std::cos(place.begin.y), std::sin(place.begin.y)); //create a unit vector for the start position
    auto unitr = glm::vec2(std::cos(place.end  .y), std::sin(place.end  .y)); //create a unit vector for the end position
    auto midpoint = std::lerp(place.begin.x, place.end.x, progress); //determine the midpoint for our transitions
    std::array points = {
        unitl*place.end.x,   //an upper-left point
        unitr*place.end.x,   //an upper-right point
        unitl*midpoint,      //an middle-left point 
        unitr*midpoint,      //an middle-right point
        unitl*place.begin.x, //an lower-left point
        unitr*place.begin.x, //an lower-right point
    };
    glBegin(GL_TRIANGLE_STRIP);
    //creating a 2-square strip:
    //1----2
    //|   /|
    //|  / |
    //| /  |
    //|/   |
    //3----4
    //|   /|
    //|  / |
    //| /  |
    //|/   |
    //5----6
    for (size_t i = 0; i < 6; ++i){
        glColor3f(i&1?1.0:0.0, i&2?1.0:0.0,i&4?1.0:0.0);
        glVertex3f(points[i].x, points[i].y,0.5);
    }
    glEnd();
}

void draw_pad(auto angle, auto extent){
    auto upfn = [](auto val, int phase){
        auto x = fmod(2*pi+val-decltype(val)(phase)*pi/4,2*pi);
        return std::max(decltype(x)(0),std::min(decltype(x)(1),std::min(pi*x, -pi*x+pi*pi/3)));
    };
    for(int i = 1; i < 17; i+=2){
        auto a = 1.0+0.2*upfn(angle, (i-1)/2)*extent;
        auto begin = pi/8.0*double(i)+0.05;
        auto end = pi/8.0*double(i+2)-0.05;
        draw_trapezoid({{0.4*a, begin}, {0.7*a, end}}, {},0.5);
    }
}

int main(){
    FT_Library freetype;
    FT_Face face;
    if(auto error = FT_Init_FreeType(&freetype)) return -1; //set up freetype
    defer ft{[&]{FT_Done_FreeType(freetype);}}; //set up a destructor for freetype
    if(auto error = FT_New_Face(freetype, "/usr/share/fonts/TTF/FiraSans-Bold.ttf", 0, &face)) return -error;
    defer ff{[&]{FT_Done_Face(face);}};
    if(!glfwInit())return -1; //set up glfw
    defer d{glfwTerminate}; //set up a destructor for glfw

    //window setup
    glfwSetErrorCallback([](int, char const * desc){std::cout << desc;});
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(640,320, "TEST", NULL, NULL);
    glfwSetFramebufferSizeCallback(window, [](auto, int a, int b){ar = float(b)/float(a);glViewport(0,0,a,b);});
    if(!window) return -1;
    glfwMakeContextCurrent(window);
    auto err = glewInit();
    if(err!=GLEW_OK) throw std::runtime_error(reinterpret_cast<const char *>(glewGetErrorString(err)));
    glEnable(GL_COLOR_MATERIAL);
    
    while(!glfwWindowShouldClose(window)){
        glLoadIdentity();
        glScalef(ar, 1, 1);
        GLFWgamepadstate state;
        if(glfwGetGamepadState(GLFW_JOYSTICK_1, &state)){
            auto left = polar({state.axes[GLFW_GAMEPAD_AXIS_LEFT_X],  -state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]});
            auto right= polar({state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X], -state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]});
            glPushMatrix();
            glTranslated(-1, 0.0, 0.0);
            draw_pad(left.y, left.x);
            glPopMatrix();
            glPushMatrix();
            glTranslated(1, 0.0, 0.0);
            draw_pad(right.y, right.x);
            glPopMatrix();
        }
        glfwSwapBuffers(window);
        glClear(GL_COLOR_BUFFER_BIT);
        glfwPollEvents();
    }
}
