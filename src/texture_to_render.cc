#include "texture_to_render.h"
#include "glfw_dispatch.h"

#include <iostream>

namespace {
int createOpenGLTexture(int width, int height) {
    
}
}

TextureToRender::TextureToRender(const int& width, const int& height, const unsigned char* buf) {
    create(width, height, buf);
}
TextureToRender::TextureToRender() {
    report("-C-");
}

/*TextureToRender::TextureToRender(const int& width, const int& height) {
    report("-C-");
}
TextureToRender(const TextureToRender&&);
TextureToRender& operator=(const TextureToRender&&);*/

TextureToRender::~TextureToRender() {
    report("-d-");
    if (fb_ < 0) return;
    unsigned int fb = fb_;
    unsigned int tex = tex_;
    unsigned int dep = dep_;
    std::thread([fb, tex, dep]{gdm::qd([&] GDM_OP {
        CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        glDeleteFramebuffers(1, &fb);
        glDeleteTextures(1, &tex);
        glDeleteRenderbuffers(1, &dep);
    }); }).detach();
    report("-D-");
}

void TextureToRender::create(int width, int height, const unsigned char* buf) {
    gdm::qd([&] GDM_OP {createNoWrap(width, height, buf);});
}
void TextureToRender::createNoWrap(int width, int height, const unsigned char* buf) {
    w_ = width;
    h_ = height;

    CHECK_GL_ERROR(glGenFramebuffers(1, &fb_));
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, fb_));

    CHECK_GL_ERROR(glGenTextures(1, &tex_));
    CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, tex_));
    CHECK_GL_ERROR(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
    CHECK_GL_ERROR(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w_, h_, 0, GL_RGB, GL_UNSIGNED_BYTE, buf));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

    CHECK_GL_ERROR(glGenRenderbuffers(1, &dep_));
    CHECK_GL_ERROR(glBindRenderbuffer(GL_RENDERBUFFER, dep_));
    CHECK_GL_ERROR(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w_, h_));
    CHECK_GL_ERROR(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, dep_));

    CHECK_GL_ERROR(glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex_, 0));

    GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, DrawBuffers);
    
    report("+++");
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Failed to create framebuffer object as render target" << std::endl;
    } else {
        std::cerr << "Framebuffer ready" << std::endl;
    }
    unbind();
}

void TextureToRender::bind() {
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, fb_));
    glClearColor(0.0, 0.0, 0.0, 0.0);
    CHECK_GL_ERROR(glViewport(0, 0, w_, h_));
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void TextureToRender::unbind() {
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void TextureToRender::saveToBuf(int& width, int& height, std::vector<unsigned char>& data) {
    width = w_;
    height = h_;
    data.resize(w_ * h_ * 3);
    gdm::qd([&]GDM_OP{
        std::cout << "inside" << std::endl;
        CHECK_GL_ERROR(glGetTextureImage(tex_, 0, GL_RGB, GL_UNSIGNED_BYTE, w_ * h_ * 3, data.data()));
        std::cout << "outside" << std::endl;
    });
    std::cout << "saved" << std::endl;
}
