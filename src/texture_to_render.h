#ifndef TEXTURE_TO_RENDER_H
#define TEXTURE_TO_RENDER_H

#include <chrono>
#include <iostream>
#include <vector>

class TextureToRender {
public:
    TextureToRender(const int& width, const int& height, const unsigned char* buf);
    /*TextureToRender(const TextureToRender&&);
    TextureToRender& operator=(const TextureToRender&&);*/
    TextureToRender();
    ~TextureToRender();

    /*TextureToRender() = delete;
    TextureToRender(const TextureToRender&) = delete;
    TextureToRender& operator=(const TextureToRender&) = delete;*/

    void create(int width, int height, const unsigned char* buf = NULL);
    void createNoWrap(int width, int height, const unsigned char* buf = NULL);
    void bind();
    void unbind();
    int getTexture() const { return tex_; }
    void report(const std::string& prefix = "---") {
        std::cout << prefix << " FB= " << fb_  << ", TEX= " << tex_  << ", DEP= " << dep_ << std::endl;
    }
    void saveToBuf(int& width, int& height, std::vector<unsigned char>& data);
private:
    int w_, h_;
    unsigned int fb_ = -1;
    unsigned int tex_ = -1;
    unsigned int dep_ = -1;
};

#endif
