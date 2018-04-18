#pragma once

#ifndef __INPUT_MANAGEMENT_H__
#define __INPUT_MANAGEMENT_H__

#include "glfw_dispatch.h"

#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_set>
#include <unordered_map>

#define SI_LAMBDA (const iom::duration_t& dt, const iom::IOView& view)
#define TD_LAMBDA (const iom::IOState& state, const iom::duration_t& dt, const iom::IOView& view)
#define TI_LAMBDA (const iom::IOState& state, const iom::IOView& view)

namespace iom {
    typedef std::chrono::duration<double, std::ratio<1,1000>> duration_t;
    struct IOKey {
        static bool is_pos(IOKey key);
        enum class IO_DEV : int { MOUSE_POS, MOUSE_KEY, MOUSE_SCROLL, KEYBOARD };
        IO_DEV dev;
        int key;
        int mods;
        bool operator==(const IOKey& rhs) const;
        bool operator==(const IOKey&& rhs) const;
        struct IOKeyHasher {
            std::size_t operator()(const IOKey& iok) const;
        };
    };
    IOKey mp();
    IOKey sp();
    IOKey kb_k(char key, int mod = 0);
    IOKey kb_k(int key, int mod = 0);
    IOKey mb_k(int key, int mod = 0);

    enum class MODE : int {
        RELEASE = 1, PRESS = 2, ANY_ACTIVE = 3,
        DOWN = -2, UP = -1, ALWAYS = -3,
        T = 4, F = 0
    }; // Always makes this closer to frp system

    MODE to_mode(int action);

    struct IOReq {
        IOKey key;
        MODE pact;
    };
    struct IOState {
        IOKey key;
        // union for slightly less space + cause I can
        MODE act;
        union {
            struct {
                double dx;
                double dy;
            };
            struct {
                double x;
                double y;
            };
        };

        IOState(IOKey, double, double);
        IOState(IOKey, MODE);
        IOState();
    };

    struct IOView {
        const std::unordered_set<IOKey, IOKey::IOKeyHasher>& dks;
        const std::unordered_map<IOKey, IOState, IOKey::IOKeyHasher>& pos_d;
        const std::unordered_map<IOKey, IOState, IOKey::IOKeyHasher>& pos_t;
    };

    class IOManager {
        std::chrono::steady_clock::time_point lt;

        std::unordered_set<IOKey, IOKey::IOKeyHasher> pos_changed;
        std::unordered_map<IOKey, IOState, IOKey::IOKeyHasher> pos_d;
        std::unordered_map<IOKey, IOState, IOKey::IOKeyHasher> pos_t;

        std::unordered_set<IOKey, IOKey::IOKeyHasher> dks; // state
        std::unordered_set<IOKey, IOKey::IOKeyHasher> pks; // pressed
        std::unordered_set<IOKey, IOKey::IOKeyHasher> rks; // released

        typedef std::function<void(const duration_t, const IOView&)> t_call;
        typedef std::function<void(const IOState&, const duration_t, const IOView&)> t_dep_call;
        typedef std::function<void(const IOState&, const IOView&)> t_indep_call;

        std::vector<t_call> indep_cont;
        std::unordered_map<IOKey, std::vector<t_dep_call>, IOKey::IOKeyHasher> cont;
        std::unordered_map<IOKey, std::vector<t_dep_call>, IOKey::IOKeyHasher> cont_up;
        std::unordered_map<IOKey, std::vector<t_dep_call>, IOKey::IOKeyHasher> cont_down;

        struct PosConts {
            PosConts() : fs({std::vector<t_dep_call>(), std::vector<t_dep_call>()}) {}
            ~PosConts() {}
            union {
                std::vector<t_dep_call> fs[2];
                struct {
                    std::vector<t_dep_call> dfs;
                    std::vector<t_dep_call> tfs;
                } vecs;
            };
        };
        std::unordered_map<IOKey, PosConts, IOKey::IOKeyHasher> cont_pos;

        struct Instants {
            Instants() : fs({std::vector<t_indep_call>(), std::vector<t_indep_call>(), std::vector<t_indep_call>()}) {}
            ~Instants() {}
            union {
                std::vector<t_indep_call> fs[3];
                struct {
                    std::vector<t_indep_call> pfs;
                    std::vector<t_indep_call> rfs;
                    std::vector<t_indep_call> afs;
                } vecs;
            };
        };
        std::unordered_map<IOKey, Instants, IOKey::IOKeyHasher> instant;

        struct PosInsts {
            PosInsts() : fs({std::vector<t_indep_call>(), std::vector<t_indep_call>()}) {}
            ~PosInsts() {}
            union {
                std::vector<t_indep_call> fs[2];
                struct {
                    std::vector<t_indep_call> dfs;
                    std::vector<t_indep_call> tfs;
                } vecs;
            };
        };
        std::unordered_map<IOKey, PosInsts, IOKey::IOKeyHasher> inst_pos;

        struct StepSyncs {
            StepSyncs() : fs({std::vector<t_dep_call>(), std::vector<t_dep_call>(), std::vector<t_dep_call>()}) {}
            ~StepSyncs() {}
            union {
                std::vector<t_dep_call> fs[3];
                struct {
                    std::vector<t_dep_call> pfs;
                    std::vector<t_dep_call> rfs;
                    std::vector<t_dep_call> afs;
                } vecs;
            };
        };
        std::unordered_map<IOKey, StepSyncs, IOKey::IOKeyHasher> step_sync;

        std::mutex g_key_lk;
        std::shared_ptr<bool> g_kill;

        std::thread con_base;
        std::thread ins_base;
        std::queue<IOState> ins_q;
        std::mutex ins_lk;
        std::condition_variable ins_q_cv;

        std::unordered_set<GLFWwindow*> windows;

        IOView view;
        IOView& generate_view(void);

    public:
        IOManager();
        ~IOManager();

        void process();
        void queue_state(IOState&& state);
        void update_state(const IOState& state);

        void activate(GLFWwindow* window);
        void deactivate(GLFWwindow* window);

        void rcb_ci(t_call);
        void rcb_ss(const IOReq&, t_dep_call);
        void rcb_c(const IOReq&, t_dep_call);
        void rcb_i(const IOReq&, t_indep_call);
    };
}

#endif
