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

void draw_trapezoid(box place, box tex, box alt, float progress, float alpha){
    auto unitl = glm::vec2(std::cos(place.begin.y), std::sin(place.begin.y)); //create a unit vector for the start position
    auto unitr = glm::vec2(std::cos(place.end  .y), std::sin(place.end  .y)); //create a unit vector for the end position
    auto midpoint = std::lerp(place.begin.x, place.end.x, progress); //determine the midpoint for our transitions
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
        glTexCoord2f(alt.begin.x, alt.end  .y); glColor4f(1,1,1, alpha); glVertex4f(points[0].x, points[0].y, 0.1f, place.begin.x);
        glTexCoord2f(alt.end  .x, alt.end  .y); glColor4f(1,1,1, alpha); glVertex4f(points[1].x, points[1].y, 0.1f, place.begin.x);
        glTexCoord2f(alt.begin.x, alt.begin.y); glColor4f(1,1,1, alpha); glVertex4f(points[2].x, points[2].y, 0.1f, midpoint);
        glTexCoord2f(alt.end  .x, alt.begin.y); glColor4f(1,1,1, alpha); glVertex4f(points[3].x, points[3].y, 0.1f, midpoint);
    }{
        gl::draw_guard d(GL_TRIANGLE_STRIP);
        glTexCoord2f(tex.begin.x, tex.end  .y); glColor4f(1,1,1, alpha); glVertex4f(points[2].x, points[2].y, 0.1f, midpoint);
        glTexCoord2f(tex.end  .x, tex.end  .y); glColor4f(1,1,1, alpha); glVertex4f(points[3].x, points[3].y, 0.1f, midpoint);
        glTexCoord2f(tex.begin.x, tex.begin.y); glColor4f(1,1,1, alpha); glVertex4f(points[4].x, points[4].y, 0.1f, place.end.x);
        glTexCoord2f(tex.end  .x, tex.begin.y); glColor4f(1,1,1, alpha); glVertex4f(points[5].x, points[5].y, 0.1f, place.end.x);
    }
}

void draw_pad(float angle, float extent, float shift, mdspan<box,1> tex, mdspan<box,1> alt, float alpha){
    auto upfn = [&](auto val, int phase){
        auto x = fmod(2*pi+val-decltype(val)(phase)*pi*2/tex.size(0),2*pi);
        return std::max(decltype(x)(0),std::min(decltype(x)(1),std::min(pi*x, -pi*x+pi*pi/3)));
    };
    for(size_t i = 0; i < tex.size(0); i++){
        auto a = 1.0-0.2*upfn(angle, i)*extent;
        auto begin = pi/tex.size(0)*double(2*i+1)+0.05;
        auto end   = pi/tex.size(0)*double(2*i+3)-0.05;
        draw_trapezoid({{1.5*a, begin}, {4*a, end}}, tex[i], alt[i], shift, alpha);
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
    defer d{glfwTerminate}; //set up a destructor for glfw

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
    glEnable( GL_TEXTURE_2D );
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    char a[] = "-_0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const mdspan<char,2> cmap{
        .strides={1,8},
        .sizes={8,8},
        .offsets={},
        .data=a,
    };
    auto map = make_charmap(cmap, 256);

    mdvec<box, 2> tex_coords_left{8UL,8UL};
    for(size_t i = 0; i < 8; ++i)
        for(size_t k = 0; k < 8; ++k)
            tex_coords_left[k][i] = box{{i/8.0, k/8.0}, {(i+2/3.0)/8.0, (k+2/3.0)/8.0}};

    mdvec<box, 2> tex_coords_right{8UL,8UL};
    for(size_t i = 0; i < 8; ++i)
        for(size_t k = 0; k < 8; ++k)
            tex_coords_right[i][k] = box{{i/8.0, k/8.0}, {(i+2/3.0)/8.0, (k+2/3.0)/8.0}};

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
            auto ratio_r = std::fmod(right.y/(2*pi)*8+7, 8.0f);
            auto ratio_l = std::fmod(left.y /(2*pi)*8+7, 8.0f);
            {
                gl::matrix_guard m;
                glTranslated(-1, 0.0, 0.0);
                draw_pad(left.y,  left.x,  std::fmod(ratio_r, 1.0f), tex_coords_left[size_t(ratio_r)], tex_coords_left[(size_t(ratio_r)+1)%8], right.x);
            }{
                gl::matrix_guard m;
                glTranslated(1, 0.0, 0.0);
                draw_pad(right.y, right.x, std::fmod(ratio_l, 1.0f), tex_coords_right[size_t(ratio_l)], tex_coords_right[(size_t(ratio_l)+1)%8], left.x);
            }
        }else{
            gl::draw_guard(GL_TRIANGLE_STRIP);
            glVertex3d(1.0, 1.0, 0.5);
            glVertex3d(-1.0, 1.0, 0.5);
            glVertex3d(0.0,0.0,0.5);
        }
        glfwSwapBuffers(window);
        glClear(GL_COLOR_BUFFER_BIT);
    }
}
