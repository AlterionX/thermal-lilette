#pragma once

#ifndef __GLFW_DISPATCH_H__
#define __GLFW_DISPATCH_H__

#include <thread>
#include <atomic>
#include <queue>
#include <functional>
#include <mutex>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <dirent.h>
#include <debuggl.h>

#define GDM_OP (void) -> void

namespace gdm {
    typedef std::function<void(void)> gdm_call_t;
    class GDManager {
    public: // static accesss
        static std::pair<bool, GDManager&> inst(
            int w=0, int h=0, std::string t=""
        );
    private: // Defaults and deletes
        GDManager();
        GDManager(const GDManager&) = delete;
        GDManager(GDManager&&) = delete;
        ~GDManager();
        void operator=(GDManager const&) = delete;
        void operator=(GDManager const&&) = delete;
    private: // class vars & typedefs
        std::atomic<bool> should_die;
        std::thread glfw_thread;
        std::function<void(std::function<bool(void)>)> loop_f;
        std::mutex q_lk;
        std::queue<gdm_call_t> dq;
        bool process(void);
        GLFWwindow* w;
        bool start(
            int w, int h, std::string t
        );
        void end();
    public: // globals
        void dispatch(gdm_call_t f, bool wait_til_complete = false);
        GLFWwindow* getWindow(void) const { return w; }
    };
    void qd(gdm_call_t f);
    void sync(void);
}

#endif
