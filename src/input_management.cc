#include "input_management.h"

#include <iostream>

namespace iom {
    void kb_cb(GLFWwindow* window, int key, int scancode, int action, int mods) {
        IOManager* iom = (IOManager*) glfwGetWindowUserPointer(window);
        iom->queue_state(IOState(kb_k(key, mods), to_mode(action)));
    }
    void mp_cb(GLFWwindow* window, double mx, double my) {
        IOManager* iom = (IOManager*) glfwGetWindowUserPointer(window);
        iom->queue_state(IOState(mp(), mx, my));
    }
    void mb_cb(GLFWwindow* window, int button, int action, int mods) {
        IOManager* iom = (IOManager*) glfwGetWindowUserPointer(window);
        iom->queue_state(IOState(mb_k(button, mods), to_mode(action)));
    }
    void ms_cb(GLFWwindow* window, double dx, double dy) {
        IOManager* iom = (IOManager*) glfwGetWindowUserPointer(window);
        iom->queue_state(IOState(sp(), dx, dy));
    }
    void bind_iom(GLFWwindow* window, IOManager* ioman) {
        glfwSetWindowUserPointer(window, ioman);

        glfwSetKeyCallback(window, kb_cb);
        glfwSetCursorPosCallback(window, mp_cb);
        glfwSetMouseButtonCallback(window, mb_cb);
        glfwSetScrollCallback(window, ms_cb);
    }
    void unbind_iom(GLFWwindow* window) {
        glfwSetKeyCallback(window, nullptr);
        glfwSetCursorPosCallback(window, nullptr);
        glfwSetMouseButtonCallback(window, nullptr);
        glfwSetScrollCallback(window, nullptr);

        glfwSetWindowUserPointer(window, nullptr);
    }

    bool IOKey::is_pos(IOKey key) {
        // update internal state
        switch (key.dev) {
        case IOKey::IO_DEV::MOUSE_POS:
        case IOKey::IO_DEV::MOUSE_SCROLL:
            return true;
        default:
            return false;
        }
    }
    bool IOKey::operator==(const IOKey& rhs) const {
        return this->dev == rhs.dev && this->key == rhs.key && this->mods == rhs.mods;
    }
    bool IOKey::operator==(const IOKey&& rhs) const {
        return this->dev == rhs.dev && this->key == rhs.key && this->mods == rhs.mods;
    }
    size_t IOKey::IOKeyHasher::operator()(const IOKey& iok) const {
        return std::hash<int>()(iok.key) + ((std::hash<int>()(iok.mods) >> 1) << 1) + (std::hash<int>()(static_cast<int>(iok.dev)) << 1);
    }
    IOKey mp() {
        return IOKey {IOKey::IO_DEV::MOUSE_POS, 0, 0};
    }
    IOKey sp() {
        return IOKey {IOKey::IO_DEV::MOUSE_SCROLL, 0, 0};
    }
    IOKey kb_k(char key, int mod) {
        if (key >= 'a' && key <= 'z') key = key - 'a' + 'A';
        return IOKey {IOKey::IO_DEV::KEYBOARD, key, mod};
    }
    IOKey kb_k(int key, int mod) {
        return IOKey {IOKey::IO_DEV::KEYBOARD, key, mod};
    }
    IOKey mb_k(int key, int mod) {
        return IOKey {IOKey::IO_DEV::MOUSE_KEY, key, mod};
    }

    MODE to_mode(int action) {
        return (action == GLFW_RELEASE ? MODE::RELEASE : MODE::PRESS);
    }

    IOState::IOState(IOKey key, double x, double y) : key(key), x(x), y(y) {}
    IOState::IOState(IOKey key, MODE pm) : key(key), act(pm) {}
    IOState::IOState() : key(), x(0), y(0) {}

    IOView& IOManager::generate_view(void) {
        return view;
    }

    IOManager::IOManager() : view {dks, pos_d, pos_t} {
        g_kill = std::make_shared<bool>(false);
    }
    IOManager::~IOManager() { for (auto window : windows) deactivate(window); }

    void IOManager::activate(GLFWwindow* window) {
        windows.insert(window);
        bind_iom(window, this);
        auto l_kill = g_kill;
        con_base = std::thread([&, l_kill, this] {
            auto step = std::chrono::steady_clock::now();
            while (!*l_kill) {
                std::this_thread::sleep_until((step + duration_t(1000 / 60)));
                step = std::chrono::steady_clock::now();
                this->process();
            }
        });
        ins_base = std::thread([&, l_kill, this] {
            auto step = std::chrono::steady_clock::now();
            auto s_qu = std::queue<IOState>();
            while (!*l_kill) {
                std::this_thread::sleep_until((step + duration_t(1000 / 60)));
                step = std::chrono::steady_clock::now();
                { std::unique_lock<std::mutex> ins_ulk(ins_lk);
                    if (!ins_q.empty()) {
                        ins_q.swap(s_qu);
                    } else {
                        ins_q_cv.wait_until(
                             ins_ulk,
                             step + duration_t(1000 / 60),
                             [&]()->bool{ return !ins_q.empty() || *l_kill; }
                        );
                    }
                }
                while(!s_qu.empty()) {
                    this->update_state(s_qu.front());
                    s_qu.pop();
                }
            }
        });
    }
    void IOManager::deactivate(GLFWwindow* window) {
        *g_kill = true;
        g_kill = std::make_shared<bool>(false);
        con_base.join();
        ins_base.join();
        unbind_iom(window);
        windows.erase(window);
    }

    void IOManager::queue_state(IOState&& state) {
        std::lock_guard<std::mutex> ins_lkg(ins_lk);
        ins_q.push(state);
        ins_q_cv.notify_one();
    }
    void IOManager::update_state(const IOState& state) {
        std::lock_guard<std::mutex> k_lk_g(g_key_lk);
        if (IOKey::is_pos(state.key)) {
            pos_changed.insert(state.key);
            if (state.key.dev == IOKey::IO_DEV::MOUSE_SCROLL) {
                pos_t[state.key].x += state.dx;
                pos_t[state.key].y += state.dy;
                pos_d[state.key].x += state.dx;
                pos_d[state.key].y += state.dy;
                for (auto& f : inst_pos[state.key].vecs.dfs) f(pos_d[state.key], view);
                for (auto& f : inst_pos[state.key].vecs.tfs) f(state, view);
            } else {
                IOState diff(state.key, state.x - pos_t[state.key].x, state.y - pos_t[state.key].y);
                pos_d[state.key].dx += diff.x;
                pos_d[state.key].dy += diff.y;
                pos_t[state.key] = state;
                for (auto& f : inst_pos[state.key].vecs.dfs) f(diff, view);
                for (auto& f : inst_pos[state.key].vecs.tfs) f(pos_t[state.key], view);
            }
        } else {
            // call key instants
            for (auto& f : instant[state.key].vecs.afs) f(state, view);
            for (auto& f : instant[state.key].fs[static_cast<int>(state.act) - 1]) f(state, view);
            if (state.act == MODE::PRESS) {
                dks.insert(state.key);
                pks.insert(state.key);
            } else {
                dks.erase(state.key);
                rks.insert(state.key);
            }
        }
    }
    void IOManager::process() {
        std::lock_guard<std::mutex> k_lk_g(g_key_lk);
        auto ct = std::chrono::steady_clock::now();
        duration_t dt = std::chrono::duration_cast<duration_t>(ct - lt);
        IOState state;
        // call pos cont
        for (auto& p_ch : pos_changed) {
            if (pos_t.count(p_ch)) {
                for (auto& f : cont_pos[p_ch].vecs.dfs) f(pos_d[p_ch], dt, view);
                for (auto& f : cont_pos[p_ch].vecs.tfs) f(pos_t[p_ch], dt, view);
            }
        }
        // step syncs
        for (auto& pk : pks) {
            state = IOState(pk, MODE::DOWN);
            for (auto& f : step_sync[pk].vecs.pfs) f(state, dt, view);
            for (auto& f : step_sync[pk].vecs.afs) f(state, dt, view);
        }
        for (auto& rk : rks) {
            state = IOState(rk, MODE::UP);
            for (auto& f : step_sync[rk].vecs.rfs) f(state, dt, view);
            for (auto& f : step_sync[rk].vecs.afs) f(state, dt, view);
        }
        // cont
        for (auto& dk : dks) {
            for (auto& f : cont_down[dk]) f(state, dt, view);
            state = IOState(dk, MODE::DOWN);
            for (auto& f : cont[dk]) f(state, dt, view);
        }
        for (auto& k_vf : cont_up) {
            if (!IOKey::is_pos(k_vf.first) && dks.count(k_vf.first)) {
                for (auto& f : cont_down[k_vf.first]) f(state, dt, view);
                state = IOState(k_vf.first, MODE::DOWN);
                for (auto& f : cont[k_vf.first]) f(state, dt, view);
            }
        }
        for (auto& f : indep_cont) f(dt, view);
        // reset for new inputs
        pks.clear();
        rks.clear();
        pos_changed.clear();
        pos_d.clear();
        lt = ct;
    }

    void IOManager::rcb_ci(t_call f) {
        std::lock_guard<std::mutex> k_lk_g(g_key_lk);
        indep_cont.push_back(f);
    }
    void IOManager::rcb_c(const IOReq& activation, t_dep_call f) {
        std::lock_guard<std::mutex> k_lk_g(g_key_lk);
        if (IOKey::is_pos(activation.key)) {
            cont_pos[activation.key].fs[static_cast<int>(activation.pact) / 4].push_back(f);
        } else {
            switch (activation.pact) {
            case MODE::DOWN:
                cont_down[activation.key].push_back(f);
                break;
            case MODE::UP:
                cont_up[activation.key].push_back(f);
                break;
            case MODE::ALWAYS:
                cont[activation.key].push_back(f);
                break;
            default: // error
                return;
            }
        }
    }
    void IOManager::rcb_ss(const IOReq& activation, t_dep_call f) {
        std::lock_guard<std::mutex> k_lk_g(g_key_lk);
        if (!IOKey::is_pos(activation.key)) {
            switch (activation.pact) {
            case MODE::PRESS:
            case MODE::RELEASE:
            case MODE::ANY_ACTIVE:
                step_sync[activation.key].fs[std::abs(static_cast<int>(activation.pact))].push_back(f);
                break;
            default: // error
                return;
            }
        }
    }
    void IOManager::rcb_i(const IOReq& activation, t_indep_call f) {
        std::lock_guard<std::mutex> k_lk_g(g_key_lk);
        if (IOKey::is_pos(activation.key)) {
            inst_pos[activation.key].fs[static_cast<int>(activation.pact) / 4].push_back(f);
        } else {
            switch (activation.pact) {
            case MODE::RELEASE:
            case MODE::PRESS:
            case MODE::ANY_ACTIVE:
                instant[activation.key].fs[std::abs(static_cast<int>(activation.pact))].push_back(f);
                break;
            default: // error
                return;
            }
        }
    }
}
