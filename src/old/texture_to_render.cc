#include "texture_to_render.h"
#include "glfw_dispatch.h"

#include <iostream>

Texture2DRenderTarget::Texture2DRenderTarget(const int& width, const int& height, const unsigned char* buf) {
    w_ = width;
    h_ = height;

    gdm::qd([&] GDM_OP {
        // Create frame buffer
        CHECK_GL_ERROR(glGenFramebuffers(1, &fb_));
        CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, fb_));

        // Create texture
        CHECK_GL_ERROR(glGenTextures(1, &tex_));
        CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, tex_));
        CHECK_GL_ERROR(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
        CHECK_GL_ERROR(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w_, h_, 0, GL_RGB, GL_UNSIGNED_BYTE, buf));
        CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
        CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
        CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

        // create depth render buffer
        CHECK_GL_ERROR(glGenRenderbuffers(1, &dep_));
        CHECK_GL_ERROR(glBindRenderbuffer(GL_RENDERBUFFER, dep_));
        CHECK_GL_ERROR(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w_, h_));

        // Compose frame buffer components
        CHECK_GL_ERROR(glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex_, 0));
        CHECK_GL_ERROR(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, dep_));

        GLenum draw_buf[1] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, draw_buf);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Failed to create framebuffer object as render target." << std::endl;
        } else {
            std::cerr << "Framebuffer ready." << std::endl;
        }
    });
    unbind();
}

Texture2DRenderTarget::~Texture2DRenderTarget() {
    if (fb_ < 0) return;
    gdm::qd([&] GDM_OP {
        CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        glDeleteFramebuffers(1, &fb);
        glDeleteTextures(1, &tex);
        glDeleteRenderbuffers(1, &dep);
    }, true);
}

void Texture2DRenderTarget::bind(void) const {
    gdm::qd([&] GDM_OP {
        CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, fb_));
        glClearColor(0.0, 0.0, 0.0, 0.0);
        CHECK_GL_ERROR(glViewport(0, 0, w_, h_));
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    });
}

void Texture2DRenderTarget::unbind(void) const {
    gdm::qd([&] GDM_OP {
        CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    });
}

void Texture2DRenderTarget::g_tex_data(int& width, int& height, std::vector<unsigned char>& data) const {
    width = w_;
    height = h_;
    data.resize(w_ * h_ * 3);
    gdm::qd([&] GDM_OP{
        CHECK_GL_ERROR(glGetTextureImage(tex_, 0, GL_RGB, GL_UNSIGNED_BYTE, w_ * h_ * 3, data.data()));
    });
    std::cout << "saved" << std::endl;
}
