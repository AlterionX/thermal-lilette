#include "gas_stuff.h"

#include <random>

#include <iostream> // TODO: remove this
#include <glm/gtx/string_cast.hpp> // TODO: remove this

#define N_ITER 20
#define EPS 1e-6
#define NUM_THREADS 4
#define at(x, y, z) (x)*size[1]*size[2] + (y)*size[2] + (z) // 1d array coordinate
#define pat(x, y, z) (x)*size[1]*size[2]*4 + (y)*size[2]*4 + (z)*4 // RGBA coordinate

GasModel::GasModel(glm::ivec3 sz) : size(sz), tex3d(sz.x*sz.y*sz.z*4, 0),
        vel_u(sz.x*sz.y*sz.z, 0.0), vel_v(sz.x*sz.y*sz.z, 0.0), vel_w(sz.x*sz.y*sz.z, 0.0), 
        vel_u_0(sz.x*sz.y*sz.z, 0.0), vel_v_0(sz.x*sz.y*sz.z, 0.0), vel_w_0(sz.x*sz.y*sz.z, 0.0), 
        vel_u_s(sz.x*sz.y*sz.z, 0.0), vel_v_s(sz.x*sz.y*sz.z, 0.0), vel_w_s(sz.x*sz.y*sz.z, 0.0), 
        den(sz.x*sz.y*sz.z, 0.0), den_0(sz.x*sz.y*sz.z, 0.0), den_s(sz.x*sz.y*sz.z, 0.0),
        temp(sz.x*sz.y*sz.z, 0.0) {
    make_ellipse();

    // randomly initialize velocities
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-1.0, 1.0);
    for(int i=0; i<size.x*size.y*size.z; i++) {
        vel_u[i] = dis(gen);
        vel_v[i] = dis(gen);
        vel_w[i] = dis(gen);
    }


    // up-jet of cloud
    for(int x=size.x*11/24; x<size.x*13/24; x++)
        for(int z=size.z*11/24; z<size.z*13/24; z++) {
            den_s[at(x, 1, z)] = 1.0;

            for(int y=1; y<size.y-2; y++) {
                vel_u_s[at(x, y, z)] = 1e6 * (x - size.x/2);
                vel_v_s[at(x, y, z)] = 1e8;
                vel_w_s[at(x, y, z)] = 1e6 * (z - size.z/2);
            }
        }

    // four side push
    // for(int x=size.x*11/24; x<size.x*13/24; x++)
    //     for(int z=size.z*11/24; z<size.z*13/24; z++) {
    //         for(int y=1; y<size.y-2; y++) {
    //             vel_u_s[at(x, y, z)] = -1e6 * (x - size.x/2);
    //             vel_v_s[at(x, y, z)] = 1e8;
    //             vel_w_s[at(x, y, z)] = -1e6 * (z - size.z/2);
    //         }
    //     }

    // for(int x=size.x*11/24; x<size.x*13/24; x++)
    //     for(int y=1; y<size.y-2; y++) {
    //     // for(int y=size.y*11/24; y<size.y*13/24; y++) {
    //         den_s[at(x, y, 1)] = 1.0;
    //         den_s[at(x, y, size.z-2)] = 1.0;

    //         vel_u_s[at(x, y, 1)] = 1e8;
    //         vel_u_s[at(x, y, size.z-2)] = -1e8;
    //     }
    // for(int z=size.z*11/24; z<size.z*13/24; z++)
    //     for(int y=1; y<size.y-2; y++) {
    //     // for(int y=size.y*11/24; y<size.y*13/24; y++) {
    //         den_s[at(1, y, z)] = 1.0;
    //         den_s[at(size.z-2, y, z)] = 1.0;

    //         vel_w_s[at(1, y, z)] = 1e8;
    //         vel_w_s[at(size.x-2, y, z)] = -1e8;
    //     }
}

void GasModel::update_tex3d(void) {
    for(int x=0; x<size.x; x++)
        for(int y=0; y<size.y; y++)
            for(int z=0; z<size.z; z++) {
                // if(vel_v[at(x, y, z)] > 0.001)
                //     std::cout << vel_u[at(x, y, z)] << " " 
                //                 << vel_v[at(x, y, z)] << " " 
                //                 << vel_w[at(x, y, z)] << std::endl;
                tex3d[pat(x, y, z)+0] = (int)(127.0 * glm::clamp(vel_u[at(x, y, z)]*2.0f, -1.0f, 1.0f)) + 127;
                tex3d[pat(x, y, z)+1] = (int)(127.0 * glm::clamp(vel_v[at(x, y, z)]*2.0f, -1.0f, 1.0f)) + 127;
                tex3d[pat(x, y, z)+2] = (int)(127.0 * glm::clamp(vel_w[at(x, y, z)]*2.0f, -1.0f, 1.0f)) + 127;
                // if(y <= 1 || den[at(x, y, z)] > 0.001)
                //     std::cout << x << ", " << y <<  ", " << z << ": " << den[at(x, y, z)] << std::endl;
                tex3d[pat(x, y, z)+3] = (int)(255 * glm::clamp(den[at(x, y, z)]*10000.0f, 0.0f, 1.0f));
                // tex3d[pat(x, y, z)+3] = 10;
           }
}

/********************************************************/
/** Navier-Stokes (v-rho) Gas Model Solver **************/

void GasModel::simulate_step(float dt) {
    static int cnt = 20;
    if(cnt-- < 0)
        for(int i=0; i<size.x*size.y*size.z; i++)
            den_s[i] = 0;

    // std::cout << "vel_step..." << std::endl;
    vel_step(vel_u, vel_u_0, vel_u_s, vel_v, vel_v_0, vel_v_s, vel_w, vel_w_0, vel_w_s, visc, dt);
    // std::cout << "den_step..." << std::endl;
    den_step(den, den_s, den_0, vel_u, vel_v, vel_w, diff, dt);
    // std::cout << "update_tex3d..." << std::endl;
    update_tex3d();
}
void GasModel::add_source(std::vector<float>& x, std::vector<float>& s, float dt) {
    int i;
    for(i=0; i<size.x*size.y*size.z; i++) {
        x[i] += dt * s[i];
    }
}
void GasModel::diffuse(int b, std::vector<float>& x, std::vector<float>& x0, float diff, float dt) {
    int i, j, k, l;
    float a = dt*diff*size.x*size.y*size.z;
    for(l=0; l<N_ITER; l++) {
        #pragma omp parallel num_threads(NUM_THREADS)
        #pragma omp for collapse(3)
        for(i=1; i<=size.x-2; i++)
            for(j=1; j<=size.y-2; j++)
                for(k=1; k<=size.z-2; k++) {
                    x[at(i, j, k)] = (x0[at(i, j, k)] 
                                        + a*(x[at(i-1, j, k)] + x[at(i+1, j, k)]
                                            +x[at(i, j-1, k)] + x[at(i, j+1, k)]
                                            +x[at(i, j, k-1)] + x[at(i, j, k+1)]))
                                    /(1 + 6*a);
                }
        set_bnd(b, x);
    }
}
void GasModel::advect(int b, std::vector<float>& d, std::vector<float>& d0,
            std::vector<float>& u, std::vector<float>& v, std::vector<float>& w,
            float dt) {
    int i, j, k;

    #pragma omp parallel num_threads(NUM_THREADS)
    #pragma omp for collapse(3)
    for(i=1; i<=size.x-2; i++)
        for(j=1; j<=size.y-2; j++)
            for(k=1; k<=size.z-2; k++) {
                int i0, j0, k0, i1, j1, k1;
                float x, y, z, s0, t0, r0, s1, t1, r1;
                x = i - dt*size.x*u[at(i,j,k)];
                y = j - dt*size.y*v[at(i,j,k)];
                z = k - dt*size.z*w[at(i,j,k)];
                if(x<0.5) x=0.5; if (x>size.x-1.5) x=size.x-1.5; i0=(int)x; i1=i0+1;
                if(y<0.5) y=0.5; if (y>size.y-1.5) y=size.y-1.5; j0=(int)y; j1=j0+1;
                if(z<0.5) z=0.5; if (z>size.z-1.5) z=size.z-1.5; k0=(int)z; k1=k0+1;
                s1 = x-i0; s0 = 1-s1;
                t1 = y-j0; t0 = 1-t1;
                r1 = z-k0; r0 = 1-r1;
                d[at(i,j,k)] = s0 * ( t0 * (r0*d0[at(i0,j0,k0)] + r1*d0[at(i0,j0,k1)])
                                    + t1 * (r0*d0[at(i0,j1,k0)] + r1*d0[at(i0,j1,k1)]) )
                             + s1 * ( t0 * (r0*d0[at(i1,j0,k0)] + r1*d0[at(i1,j0,k1)])
                                    + t1 * (r0*d0[at(i1,j1,k0)] + r1*d0[at(i1,j1,k1)]) );
            }
    set_bnd(b, d);
}
void GasModel::den_step(std::vector<float>& x, std::vector<float>& source, std::vector<float>& x0,
            std::vector<float>& u, std::vector<float>& v, std::vector<float>& w,
            float diff, float dt) {
    add_source(x, source, dt);
    x0.swap(x); diffuse(0, x, x0, diff, dt);
    x0.swap(x); advect(0, x, x0, u, v, w, dt);

}
void GasModel::vel_step(std::vector<float>& u, std::vector<float>& u0, std::vector<float>& u_s,
            std::vector<float>& v, std::vector<float>& v0, std::vector<float>& v_s,
            std::vector<float>& w, std::vector<float>& w0, std::vector<float>& w_s,
            float visc, float dt) {
    // std::cout << "\tadd_source..." << std::endl;
    add_source(u, u_s, dt); add_source(v, v_s, dt); add_source(w, w_s, dt);
    // std::cout << "\tdiffuse..." << std::endl;
    u0.swap(u); diffuse(1, u, u0, visc, dt);
    v0.swap(v); diffuse(2, v, v0, visc, dt);
    w0.swap(w); diffuse(3, w, w0, visc, dt);
    // std::cout << "\tproject1..." << std::endl;
    project(u, v, w, u0, v0); // u0 -> p, v0 -> div
    u0.swap(u); v0.swap(v); w0.swap(w);
    // std::cout << "\tadvect..." << std::endl;
    advect(1, u, u0, u0, v0, w0, dt);
    advect(2, v, v0, u0, v0, w0, dt);
    advect(3, w, w0, u0, v0, w0, dt);
    // std::cout << "\tproject2..." << std::endl;
    project(u, v, w, u0, v0); // u0 -> p, v0 -> div
} // vel_step ( N, u, u_prev, v, v_prev, w, w_prev, visc, dt );
void GasModel::project(std::vector<float>& u, std::vector<float>& v, std::vector<float>& w,
            std::vector<float>& p, std::vector<float>& div) {
    int i, j, k, l;
    float h;

    #pragma omp parallel num_threads(NUM_THREADS)
    #pragma omp for collapse(3)
    for(i=1; i<=size.x-2; i++)
        for(j=1; j<=size.y-2; j++)
            for(k=1; k<=size.z-2; k++) {
                div[at(i, j, k)] = -0.5 * (
                                    (1.0/size.x)*(u[at(i+1, j, k)] - u[at(i-1, j, k)])
                                +   (1.0/size.y)*(v[at(i, j+1, k)] - v[at(i, j-1, k)])
                                +   (1.0/size.z)*(w[at(i, j, k+1)] - w[at(i, j, k-1)]) );
                p[at(i, j, k)] = 0.0;
            }

    for(l=0; l<N_ITER; l++) {
        #pragma omp parallel num_threads(NUM_THREADS)
        #pragma omp for collapse(3)
        for(i=1; i<=size.x-2; i++)
            for(j=1; j<=size.y-2; j++)
                for(k=1; k<=size.z-2; k++)
                    p[at(i, j, k)] = (div[at(i, j, k)]
                                    + p[at(i-1, j, k)] + p[at(i+1, j, k)]
                                    + p[at(i, j-1, k)] + p[at(i, j+1, k)]
                                    + p[at(i, j, k-1)] + p[at(i, j, k+1)]) / 6.0;
        set_bnd(0, p);
    }

    #pragma omp parallel num_threads(NUM_THREADS)
    #pragma omp for collapse(3)
    for(i=1; i<=size.x-2; i++)
        for(j=1; j<=size.y-2; j++)
            for(k=1; k<=size.z-2; k++) {
                u[at(i, j, k)] -= 0.5 * size.x * (p[at(i+1, j, k)] - p[at(i-1, j, k)]);
                v[at(i, j, k)] -= 0.5 * size.y * (p[at(i, j+1, k)] - p[at(i, j-1, k)]);
                w[at(i, j, k)] -= 0.5 * size.z * (p[at(i, j, k+1)] - p[at(i, j, k-1)]);
            }
    set_bnd(1, u); set_bnd(2, v); set_bnd(3, w);
}
void GasModel::set_bnd(int b, std::vector<float>& x) {
    int i, j, k;

    if(bnd_type == 0) { // zero out
        // sides
        for(j=0; j<size.y; j++)
            for(k=0; k<size.z; k++) {
                x[at(0, j, k)]        = 0.0;
                x[at(size.x-1, j, k)] = 0.0;
        }
        for(i=0; i<size.x; i++)
            for(k=0; k<size.z; k++) {
                x[at(i, 0, k)]        = 0.0;
                x[at(i, size.y-1, k)] = 0.0;
        }
        for(i=0; i<size.x; i++)
            for(j=0; j<size.y; j++) {
                x[at(i, j, 0)]        = 0.0;
                x[at(i, j, size.z-1)] = 0.0;
        }
    }
    else if(bnd_type == 1) { // average, velo reflective
        // sides
        for(j=0; j<size.y; j++)
            for(k=0; k<size.z; k++) {
                x[at(0, j, k)]        = b==1 ? -x[at(1, j, k)]        : x[at(1, j, k)];
                x[at(size.x-1, j, k)] = b==1 ? -x[at(size.x-2, j, k)] : x[at(size.x-2, j, k)];
        }
        for(i=0; i<size.x; i++)
            for(k=0; k<size.z; k++) {
                x[at(i, 0, k)]        = b==2 ? -x[at(i, 1, k)]        : x[at(i, 1, k)];
                x[at(i, size.y-1, k)] = b==2 ? -x[at(i, size.y-2, k)] : x[at(i, size.y-2, k)];
        }
        for(i=0; i<size.x; i++)
            for(j=0; j<size.y; j++) {
                x[at(i, j, 0)]        = b==3 ? -x[at(i, j, 1)]        : x[at(i, j, 1)];
                x[at(i, j, size.z-1)] = b==3 ? -x[at(i, j, size.z-2)] : x[at(i, j, size.z-2)];
        }

        // borders
        for(i=0; i<size.x; i++) {
            x[at(i, 0, 0)] = (x[at(i, 1, 0)]
                            + x[at(i, 0, 1)]) / 2.0;
            x[at(i, 0, size.z-1)] = (x[at(i, 1, size.z-1)]
                                   + x[at(i, 0, size.z-2)]) / 2.0;
            x[at(i, size.y-1, 0)] = (x[at(i, size.y-2, 0)]
                                   + x[at(i, size.y-1, 1)]) / 2.0;
            x[at(i, size.y-1, size.z-1)] = (x[at(i, size.y-2, size.z-1)]
                                          + x[at(i, size.y-1, size.z-2)]) / 2.0;
        }
        for(j=0; j<size.y; j++) {
            x[at(0, j, 0)] = (x[at(1, j, 0)]
                            + x[at(0, j, 1)]) / 2.0;
            x[at(0, j, size.z-1)] = (x[at(1, j, size.z-1)]
                                   + x[at(0, j, size.z-2)]) / 2.0;
            x[at(size.x-1, j, 0)] = (x[at(size.x-2, j, 0)]
                                   + x[at(size.x-1, j, 1)]) / 2.0;
            x[at(size.x-1, j, size.z-1)] = (x[at(size.x-2, j, size.z-1)]
                                          + x[at(size.x-1, j, size.z-2)]) / 2.0;
        }
        for(k=0; k<size.z; k++) {
            x[at(0, 0, k)] = (x[at(1, 0, k)]
                            + x[at(0, 1, k)]) / 2.0;
            x[at(0, size.y-1, k)] = (x[at(1, size.y-1, k)]
                                   + x[at(0, size.y-2, k)]) / 2.0;
            x[at(size.x-1, 0, k)] = (x[at(size.x-2, 0, k)]
                                   + x[at(size.x-1, 1, k)]) / 2.0;
            x[at(size.x-1, size.y-1, k)] = (x[at(size.x-2, size.y-1, k)]
                                          + x[at(size.x-1, size.y-2, k)]) / 2.0;
        }


        // corners
        x[at(0, 0, 0)] = (x[at(1, 0, 0)]
                        + x[at(0, 1, 0)]
                        + x[at(0, 0, 1)]) / 3.0;
        x[at(size.x-1, 0, 0)] = (x[at(size.x-2, 0, 0)]
                               + x[at(size.x-1, 1, 0)]
                               + x[at(size.x-1, 0, 1)]) / 3.0;
        x[at(0, size.y-1, 0)] = (x[at(1, size.y-1, 0)]
                               + x[at(0, size.y-2, 0)]
                               + x[at(0, size.y-1, 1)]) / 3.0;
        x[at(0, 0, size.z-1)] = (x[at(1, 0, size.z-1)]
                               + x[at(0, 1, size.z-1)]
                               + x[at(0, 0, size.z-2)]) / 3.0;
        x[at(0, size.y-1, size.z-1)] = (x[at(1, size.y-1, size.z-1)]
                                      + x[at(0, size.y-2, size.z-1)]
                                      + x[at(0, size.y-1, size.z-2)]) / 3.0;
        x[at(size.x-1, 0, size.z-1)] = (x[at(size.x-2, 0, size.z-1)]
                                      + x[at(size.x-1, 1, size.z-1)]
                                      + x[at(size.x-1, 0, size.z-2)]) / 3.0;
        x[at(size.x-1, size.y-1, 0)] = (x[at(size.x-2, size.y-1, 0)]
                                      + x[at(size.x-1, size.y-2, 0)]
                                      + x[at(size.x-1, size.y-1, 1)]) / 3.0;
        x[at(size.x-1, size.y-1, size.z-1)] = (x[at(size.x-2, size.y-1, size.z-1)]
                                             + x[at(size.x-1, size.y-2, size.z-1)]
                                             + x[at(size.x-1, size.y-1, size.z-2)]) / 3.0;
    }
}

void GasModel::make_ellipse(void) {
    glm::vec3 center = glm::vec3((float)size.x, (float)size.y, size.z/2.0f)/2.0f;
    for(int x=0; x<size.x; x++)
        for(int y=0; y<size.y; y++)
            for(int z=0; z<size.z; z++) {
                tex3d[pat(x, y, z)+0] = 255;
                tex3d[pat(x, y, z)+1] = 255;
                tex3d[pat(x, y, z)+2] = 255;
                glm::vec3 delta = glm::vec3((float)x, (float)y, z/2.0f) - center;
                std::cout << glm::to_string(delta) << ", length= " << glm::length(delta) << std::endl;
                if(glm::length(delta) < size.x/4.0) {
                    tex3d[pat(x, y, z)+3] = 255;
                }
                else {
                    tex3d[pat(x, y, z)+3] = 0;
                }
            }
}