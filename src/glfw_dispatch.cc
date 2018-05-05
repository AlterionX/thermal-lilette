#include "glfw_dispatch.h"

#include <condition_variable>
#include <iostream>

namespace {
void ErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error: " << description << "\n";
}
bool DEBUG = false;
}

namespace gdm {
GDManager::GDManager() {
    loop_f = [this](std::function<bool(void)> initial_call) -> void {
        if (!initial_call()) return; // quietly terminate if failed
        while (!should_die) process();
    };
}
GDManager::~GDManager() {
    if (DEBUG) std::cout << "Terminating GLFW manager." << std::endl;
    end();
}

std::pair<bool, GDManager&> GDManager::inst(
    int w, int h, std::string t
) {
    static GDManager igdm;
    static bool init = false;
    static bool launched; // fail once, fail forever
    if (!init) {
        std::lock_guard<std::mutex> wait_for_it(igdm.q_lk);
        if (!init) {
            launched = igdm.start(w, h, t);
            init = true;
        }
    }
    return std::pair<bool, GDManager&>(launched, igdm);
}

bool GDManager::process(void) {
    gdm_call_t k;
    if (DEBUG) std::cout << "trying to lock, process..." << std::endl;
    { std::lock_guard<std::mutex> q_lkg(q_lk);
        if (DEBUG) std::cout << "trying to poll..." << std::endl;
        if (dq.empty()) {
            if (DEBUG) std::cout << "found nothing" << std::endl;
            return false; // should use a cond_var
        }
        k = dq.front();
        dq.pop();
    }
    if (DEBUG) std::cout << "polled..." << std::endl;
    k();
    return true;
}

bool GDManager::start(int w, int h, std::string t) {
    static bool started = false;
    if (!started) {
        if (DEBUG) std::cout << "Starting GLFW thread." << std::endl;
        should_die = false;
        static std::mutex l_lk;
        std::unique_lock<std::mutex> l_ulk(l_lk);
        std::condition_variable on_init;
        auto init = [&](void) -> bool {
            { std::lock_guard<std::mutex> l_lkg(l_lk);
                if (glfwInit()) {
                    glfwSetErrorCallback(ErrorCallback);
                    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
                    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
                    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
                    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
                    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE); // Disable resizing, for simplicity
                    glfwWindowHint(GLFW_SAMPLES, 4);
                    this->w = glfwCreateWindow(w, h, t.data(), nullptr, nullptr);
                    glfwSetInputMode(this->w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    CHECK_SUCCESS(this->w != nullptr);
                    glfwMakeContextCurrent(this->w);
                    glewExperimental = GL_TRUE;
                    CHECK_SUCCESS(glewInit() == GLEW_OK);
                    glGetError();  // clear GLEW's error for it
                    glfwSwapInterval(1);
                    const GLubyte* renderer = glGetString(GL_RENDERER);  // get renderer string
                    const GLubyte* version = glGetString(GL_VERSION);    // version as a string
                    if (DEBUG) std::cout << "Renderer: " << renderer << "\n";
                    if (DEBUG) std::cout << "OpenGL version supported:" << version << "\n";
                }
            }
            started = true;
            on_init.notify_one();
            return true;
        };
        glfw_thread = std::thread(loop_f, init);
        on_init.wait(l_ulk, []{return started;});
    }
    return started;
}
void GDManager::end(void) {
    if (!should_die) {
        if (DEBUG) std::cout << "Beginning death." << std::endl;
        dispatch([this] {
            if (DEBUG) std::cout << "Destroying window..." << std::endl;
            glfwDestroyWindow(w);
            if (DEBUG) std::cout << "Terminating GLFW." << std::endl;
            glfwTerminate();
            if (DEBUG) std::cout << "Fully terminating." << std::endl;
            should_die = true;
        });
        glfw_thread.join();
    }
}

void GDManager::dispatch(gdm_call_t f, bool wait_til_complete) {
    if (glfw_thread.get_id() == std::this_thread::get_id()) { // TODO make this aware of wait_til_complete
        f();
    } else if (wait_til_complete) {
        std::mutex cw_lk;
        std::unique_lock<std::mutex> cw_ulk(cw_lk);
        bool comp = false;
        std::condition_variable cb_comp;
        if (DEBUG) std::cout << "entrance" << &f << "." << std::endl;
        { std::lock_guard<std::mutex>q_lkg(q_lk);
            dq.push([&, f]() {
                // synchro
                if (DEBUG) std::cout << "trying to run" << &f << "....." << std::endl;
                f();
                if (DEBUG) std::cout << "ran" << &f << "....." << std::endl;
                if (DEBUG) std::cout << "trying to lock" << &f << "....." << std::endl;
                { std::lock_guard<std::mutex> cw_lkg(cw_lk);
                    comp = true;
                }
                if (DEBUG) std::cout << "notifying" << &f << "....." << std::endl;
                cb_comp.notify_one();
            });
        }
        if (DEBUG) std::cout << "waiting" << &f << "." << std::endl;
        cb_comp.wait(cw_ulk, [&](void)->bool{ return comp; });
        if (DEBUG) std::cout << "exit" << &f << "." << std::endl;
    } else {
        std::lock_guard<std::mutex>q_lkg(q_lk);
        dq.push(f);
    }
}
void qd(gdm_call_t f) {
    static std::mutex qd_lk; // TODO : Change this to thread with condition var
    std::lock_guard<std::mutex> qd_lkg(qd_lk);
    GDManager::inst().second.dispatch(f, false);
    sync();
}
void sync(void) {
    GDManager::inst().second.dispatch([](){}, true);
}
}
