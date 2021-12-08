#include<utility>
#include<stdexcept>
#include<iterator>
#include<iostream>
#include<iomanip>
#include<fstream>
#include<array>
#include<cmath>
#include<GL/glew.h>
#include<GLFW/glfw3.h>
#include<glm/glm.hpp>
#include<ft2build.h>
#include FT_FREETYPE_H
#include"gl.hpp"
#include"algo.hpp"
#include"mdspan.hpp"
#include"mdvec.hpp"

using quad = std::array<glm::vec2, 4>;
using aabb = std::array<glm::vec2, 2>;

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

glm::vec2 cart(glm::vec2 polar){
    return glm::vec2{std::sin(polar.y), std::cos(polar.y)}*polar.x;
}

float ar = 0.5;
double const pi = 3.14592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282;

void draw_trapezoid(aabb place, quad tex, quad alt, float progress, float alpha){
    auto unitl = glm::vec2(std::cos(place[0].y), std::sin(place[0].y)); //create a unit vector for the start position
    auto unitr = glm::vec2(std::cos(place[1].y), std::sin(place[1].y)); //create a unit vector for the end position
    auto midpoint = std::lerp(place[0].x, place[1].x, progress); //determine the midpoint for our transitions
    std::array points = {
        unitl, //an lower-left point
        unitr, //an lower-right point
        unitl, //an middle-left point 
        unitr, //an middle-right point
        unitl, //an upper-left point
        unitr, //an upper-right point
    };
    {
        gl::draw_guard d(GL_TRIANGLE_STRIP);
        glTexCoord2f(alt[0].x, alt[0].y); glColor4f(1,1,1, alpha); glVertex4f(points[0].x, points[0].y, 0.1f, place[0].x);
        glTexCoord2f(alt[1].x, alt[1].y); glColor4f(1,1,1, alpha); glVertex4f(points[1].x, points[1].y, 0.1f, place[0].x);
        glTexCoord2f(alt[2].x, alt[2].y); glColor4f(1,1,1, alpha); glVertex4f(points[2].x, points[2].y, 0.1f, midpoint);
        glTexCoord2f(alt[3].x, alt[3].y); glColor4f(1,1,1, alpha); glVertex4f(points[3].x, points[3].y, 0.1f, midpoint);
    }{
        gl::draw_guard d(GL_TRIANGLE_STRIP);
        glTexCoord2f(tex[0].x, tex[0].y); glColor4f(1,1,1, alpha); glVertex4f(points[2].x, points[2].y, 0.1f, midpoint);
        glTexCoord2f(tex[1].x, tex[1].y); glColor4f(1,1,1, alpha); glVertex4f(points[3].x, points[3].y, 0.1f, midpoint);
        glTexCoord2f(tex[2].x, tex[2].y); glColor4f(1,1,1, alpha); glVertex4f(points[4].x, points[4].y, 0.1f, place[1].x);
        glTexCoord2f(tex[3].x, tex[3].y); glColor4f(1,1,1, alpha); glVertex4f(points[5].x, points[5].y, 0.1f, place[1].x);
    }
}

void draw_pad(float angle, float extent, float shift, mdspan<quad,1> tex, mdspan<quad,1> alt, float alpha){
    auto upfn = [&](auto val, int phase){
        return std::max(sin(2*pi*(float(phase)/tex.size(0)+0.25)-val)*tex.size(0)-tex.size(0)+1, 0.0);
    };
    for(size_t i = 0; i < tex.size(0); i++){
        auto a = 1.0-0.2*upfn(angle, i+1)*extent;
        auto begin = pi/tex.size(0)*double(2*i+1)+0.000;
        auto end   = pi/tex.size(0)*double(2*i+3)-0.000;
        draw_trapezoid({glm::vec2{1.5*a, begin}, glm::vec2{4*a, end}}, tex[i], alt[i], shift, alpha);
    }
}

mdvec<unsigned char, 2> make_charmap(mdspan<char, 2> charmap, size_t cellsize){
    FT_Library freetype;
    FT_Face face;
    if(FT_Init_FreeType(&freetype)) throw std::runtime_error("could not initialize freetype"); //set up freetype
    defer ft{[&]{FT_Done_FreeType(freetype);}}; //set up a destructor for freetype
    if(FT_New_Face(freetype, "/usr/share/fonts/TTF/FiraSans-Bold.ttf", 0, &face)) throw std::runtime_error("could not load font");
    defer ff{[&]{FT_Done_Face(face);}};
    if(FT_Set_Pixel_Sizes(face, FT_UInt(cellsize), FT_UInt(cellsize))) throw std::runtime_error("could not se pixel size");
    mdvec<unsigned char, 2>ret (charmap.size(0)*cellsize*3/2, charmap.size(1)*cellsize*3/2);
    size_t i=0, j=0;
    for (auto x:charmap){
        for (auto v:x){
            auto ss = ret.subspan({i*cellsize*3/2,j*cellsize*3/2}, {i*cellsize*3/2+cellsize,j*cellsize*3/2+cellsize});
            auto glyph_idx = FT_Get_Char_Index(face, FT_ULong(v));
            if(FT_Load_Glyph(face, glyph_idx, FT_LOAD_RENDER)) throw std::runtime_error("could not load glyph");
            //face->glyph->format = FT_GLYPH_FORMAT_BITMAP;
            //face->glyph->bitmap_left=cellsize/2;
            //face->glyph->bitmap_top =cellsize/2;
            if(FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) throw std::runtime_error("Could not render glyph");
            auto bmp = face->glyph->bitmap;
            const mdspan<unsigned char,2> src{
                .strides={1, size_t(bmp.pitch)},
                .sizes={bmp.width, bmp.rows},
                .offsets={},
                .data=bmp.buffer,
            };
            ss.offsets[0]+=(cellsize-src.sizes[0])/2;
            ss.offsets[1]+=(cellsize-src.sizes[1])/2;
            ss = src;
            i++;
        }
        j++;
    }
    return ret;
}

int main(){
    if(!glfwInit())return -1; //set up glfw
    std::string buf;
    std::ifstream db("controllerdb.txt");
    std::copy(std::istream_iterator<char>(db),
            std::istream_iterator<char>(),
            std::back_inserter(buf));
    glfwUpdateGamepadMappings(buf.c_str());
    defer glfw{glfwTerminate}; //set up a destructor for glfw

    //window setup
    glfwSetErrorCallback([](int, char const * desc){std::cout << desc;});
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(640,320, "TEST", NULL, NULL);
    glfwSetFramebufferSizeCallback(window, [](auto, int a, int b){
        ar = float(b)/float(a);glViewport(0,0,a,b);
        glLoadIdentity();
        glScalef(ar, 1,1);
    });
    if(!window) return -1;
    glfwMakeContextCurrent(window);
    auto err = glewInit();
    if(err!=GLEW_OK) throw std::runtime_error(reinterpret_cast<const char *>(glewGetErrorString(err)));
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    char a[] = "-_0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const mdspan<char,2> cmap{
        .strides={1,6},
        .sizes={6,8},
        .offsets={},
        .data=a,
    };
    auto map = make_charmap(cmap, 256);

    auto fill_tex_map = [](auto&& tc, bool idx_by_x){
        for(size_t i = 0; i < tc.size(0); ++i)
            for(size_t k = 0; k < tc.size(1); ++k){
                float phase = idx_by_x ? pi*2*(i+1)/tc.size(0) : pi*2*(k+1)/tc.size(1);
                auto center = glm::vec2{i+(1/3.0), k+(1/3.0)};
                auto scale = glm::vec2{tc.size(0), tc.size(1)};
                tc(k,i) = quad{
                    (center+cart({std::sqrt(2)/3, 1*pi/4+phase}))/scale,
                    (center+cart({std::sqrt(2)/3, 3*pi/4+phase}))/scale,
                    (center+cart({std::sqrt(2)/3, 7*pi/4+phase}))/scale,
                    (center+cart({std::sqrt(2)/3, 5*pi/4+phase}))/scale,
                };
            }
    };

    mdvec<quad, 2> tex_coords_left{cmap.size(0),cmap.size(1)};
    fill_tex_map(tex_coords_left,true);


    mdvec<quad, 2> tex_coords_right{cmap.size(1), cmap.size(0)};
    fill_tex_map(transpose_view{tex_coords_right}, false);

    GLuint tex;
    glGenTextures(1, &tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture( GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA8, int(map.size(0)), int(map.size(1)), 0, GL_ALPHA, GL_UNSIGNED_BYTE, map.data);
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

    int gamepad;
    for(gamepad=0;gamepad < GLFW_JOYSTICK_LAST; ++gamepad)
        if (glfwJoystickIsGamepad(gamepad))break;
    
    glScalef(ar, 1,1);
    while(!glfwWindowShouldClose(window)){
        glfwPollEvents();
        GLFWgamepadstate state;
        if(glfwGetGamepadState(gamepad, &state)){
            auto left  = polar({state.axes[GLFW_GAMEPAD_AXIS_LEFT_X],  -state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]});
            auto right = polar({state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X], -state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]});
            auto calc_angle = [](auto v, size_t max){
                return std::fmod(v/(2*pi)*max+(max-1), max);
            };
            auto ratio_r = calc_angle(right.y, tex_coords_right.size(0));
            auto ratio_l = calc_angle(left .y, tex_coords_left .size(0));
            {
                gl::matrix_guard m;
                glTranslated(-1, 0.0, 0.0);
                draw_pad(left.y,  left.x,  std::fmod(ratio_r, 1.0f), tex_coords_left[size_t(ratio_r)], tex_coords_left[(size_t(ratio_r)+1)%tex_coords_right.size(0)], right.x);
            }{
                gl::matrix_guard m;
                glTranslated(1, 0.0, 0.0);
                draw_pad(right.y, right.x, std::fmod(ratio_l, 1.0f), tex_coords_right[size_t(ratio_l)], tex_coords_right[(size_t(ratio_l)+1)%tex_coords_left.size(0)], left.x);
            }
        }else{
            gl::draw_guard d(GL_TRIANGLE_STRIP);
            glVertex3d(1.0, 1.0, 0.5);
            glVertex3d(-1.0, 1.0, 0.5);
            glVertex3d(0.0,0.0,0.5);
        }
        glfwSwapBuffers(window);
        glClear(GL_COLOR_BUFFER_BIT);
    }
}
