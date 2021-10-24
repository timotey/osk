#include<utility>
#include<stdexcept>
#include<iostream>
#include<cmath>
#include<GL/glew.h>
#include<GLFW/glfw3.h>
#include<glm/glm.hpp>

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

float ar = 1;
double const pi = 3.14592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282;

void draw_trapezoid(double start_angle, double stop_angle, double iradius, double oradius, double progress){
    auto sx = std::cos(start_angle);
    auto sy = std::sin(start_angle);
    auto ex = std::cos(stop_angle );
    auto ey = std::sin(stop_angle );
    auto cradius = std::lerp(iradius,oradius,progress);
    glBegin(GL_TRIANGLE_STRIP);
    glVertex3d(sx*iradius, sy*iradius, 0.5);
    glVertex3d(sx*oradius, sy*oradius, 0.5);
    glVertex3d(ex*iradius, ey*iradius, 0.5);
    glVertex3d(ex*oradius, ey*oradius, 0.5);
    glVertex3d(sx*iradius, sy*iradius, 0.5);
    glEnd();
}

void draw_pad(auto angle, auto extent){
    auto upfn = [](auto val, int phase){
        auto x = fmod(2*pi+val-decltype(val)(phase)*pi/4,2*pi);
        return std::max(decltype(x)(0),std::min(decltype(x)(1),std::min(pi*x, -pi*x+pi*pi/3)));
    };
    for(int i = 1; i<17; i+=2){
        auto a = 0.1*upfn(angle, (i-1)/2)*extent;
        draw_trapezoid(pi/8.0*double(i)+0.05, pi/8.0*double(i+2)-0.05, 0.4+a, 0.7+a,0.0);
    }
}


int main(){
    if(!glfwInit())return -1;
    defer d{glfwTerminate};
    glfwSetErrorCallback(+[](int, char const * desc){std::cout << desc;});
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    GLFWwindow* window = glfwCreateWindow(640, 680, "TEST", NULL, NULL);
    glfwSetFramebufferSizeCallback(window, [](auto, int a, int b){ar = float(b)/float(a);glViewport(0,0,a,b);});
    if(!window) return -1;
    glfwMakeContextCurrent(window);
    auto err = glewInit();
    if(err!=GLEW_OK) throw std::runtime_error(reinterpret_cast<const char *>(glewGetErrorString(err)));
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
