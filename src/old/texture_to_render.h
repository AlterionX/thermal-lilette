#ifndef TEXTURE_TO_RENDER_H
#define TEXTURE_TO_RENDER_H

#include "glfw_dispatch.h"

#include <chrono>
#include <iostream>
#include <vector>

class TextureRenderTarget {
public:
    TextureRenderTarget(const TextureRenderTarget& copy) = delete;
    TextureRenderTarget& operator=(const TextureRenderTarget& copy) = delete;
    TextureRenderTarget(TextureRenderTarget&& move) = default;
    TextureRenderTarget& operator=(TextureRenderTarget&& move) = default;

    TextureRenderTarget(void) = delete;
    ~TextureRenderTarget(void);

    TextureRenderTarget(const int& width, const int& height, const unsigned char* buf);

    void bind(void) const;
    void unbind(void) const;

    int g_tex_name() const { return tex_name_; }
    void g_tex_data(int& width, int& height, std::vector<unsigned char>& data) const;
private:
    int w_, h_;
    unsigned int f_buf_name_ = -1; // frame buffer
    bool f_buf_is_bound_ = false; // frame buffer is bound
    unsigned int tex_name_ = -1;
    unsigned int d_buf_name_ = -1; // depth buffer
};

#endif
