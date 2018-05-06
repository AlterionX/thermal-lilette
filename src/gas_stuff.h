#pragma once

#ifndef __GAS_STUFF_H__
#define __GAS_STUFF_H__

#include <glm/glm.hpp>
#include <vector>

class GasModel {
public:
    GasModel(glm::ivec3 sz);

    char* get_tex3d(void) { return tex3d.data(); }
    glm::ivec3 get_size(void) const { return size; }
	void simulate_step(float dt, bool nocol);

    void shift_diff(float diff_shift);
    void shift_visc(float visc_shift);
    void apply_burst(int preset);
    void retract_burst(void);

private:

    std::vector<char> tex3d; // W x H x L x 4
    glm::ivec3 size; // (W, H, L)

	void update_tex3d(bool nocol);

    // simulation, from http://www.intpowertechcorp.com/GDC03.pdf
	void add_source(std::vector<float>& x, std::vector<float>& s, float dt);
	void diffuse(int b, std::vector<float>& x, std::vector<float>& x0, float diff, float dt);
	void advect(int b, std::vector<float>& d, std::vector<float>& d0,
            std::vector<float>& u, std::vector<float>& v, std::vector<float>& w,
            float dt);
	void den_step(std::vector<float>& x, std::vector<float>& den_s, std::vector<float>& x0,
            std::vector<float>& u, std::vector<float>& v, std::vector<float>& w,
            float diff, float dt);
	void vel_step(std::vector<float>& u, std::vector<float>& u0, std::vector<float>& u_s,
            std::vector<float>& v, std::vector<float>& v0, std::vector<float>& v_s,
            std::vector<float>& w, std::vector<float>& w0, std::vector<float>& w_s,
            float visc, float dt);
	void project(std::vector<float>& u, std::vector<float>& v, std::vector<float>& w,
            std::vector<float>& p, std::vector<float>& div);
	void set_bnd(int b, std::vector<float>& x);
    std::vector<float> vel_u, vel_v, vel_w, vel_u_0, vel_v_0, vel_w_0, vel_u_s, vel_v_s, vel_w_s;
    std::vector<float> den, den_0, den_s;
    std::vector<float> temp;
    float visc = 1.0;
    float diff = 0.01;
    int bnd_type = 1;

    int curr_burst = 0;
    int burst_timer = 20;

	void make_ellipse(void); // for debugging
};

#endif