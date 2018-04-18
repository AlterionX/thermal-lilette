#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <functional>
#include <material.h>

namespace rpa {
    typedef std::function<void(int, int, const void*)> binder_t;
    struct UType {
        enum class BASE_TYPE { NUM, TEX, NONE };
        enum class TEX_TYPE { TEX, SAMPLER };
        enum class NUM_TYPE { FLOAT, INT, UINT };
        enum class NUM_SUB_TYPE { MATRIX, VECTOR, SINGLE };

        UType(BASE_TYPE base, NUM_TYPE num, NUM_SUB_TYPE sub, int cnt);
        UType(BASE_TYPE base, NUM_TYPE num, NUM_SUB_TYPE sub, int row, int col);
        UType(BASE_TYPE base, TEX_TYPE tex, int dimen);
        UType(BASE_TYPE base);

        BASE_TYPE base;
        union {
            NUM_TYPE datatype;
            TEX_TYPE textype;
        };
        NUM_SUB_TYPE sub; // potential union
        union {
            int cnt;
            int rows;
            int dimen;
        };
        int cols;

        struct Hasher {
            size_t operator()(const UType& h) const;
        };
        bool operator==(const UType& other) const;
        static std::unordered_map<UType, binder_t, Hasher> bind_map;

        static UType fm(int size);
        static UType fm(int rs, int cs);
        static UType fv(int size);
        static UType fs(void);
        static UType im(int size);
        static UType im(int rs, int cs);
        static UType iv(int size);
        static UType is(void);
        static UType um(int size);
        static UType um(int rs, int cs);
        static UType uv(int size);
        static UType us(void);

        static UType tx(int dimen);
        static UType ts(int dimen);

        static UType null(void);
    };


    struct ShaderUniform {
        std::string name;
        UType type;
        ShaderUniform(std::string name, UType type);
        virtual void bind(const int& in) = 0;
        virtual void g_cache(void) = 0;
    };
    // Use template metaprogramming to allocate caches that would otherwise need to be stored manually
    template<typename U>
    struct PtrSU : public ShaderUniform {
        typedef std::function<void(std::shared_ptr<U>& cache)> cacher_t;

        cacher_t cdfn;
        std::shared_ptr<U> cache;
        bool custom_bfn;
        binder_t bfn;

        PtrSU(std::string name, UType type, cacher_t cdfn)
        : ShaderUniform(name, type), cdfn(cdfn), cache(nullptr), custom_bfn(false), bfn([](int, int, const void*){}) {}
        PtrSU(std::string name, binder_t bfn, cacher_t cdfn)
        : ShaderUniform(name, UType::null()), cdfn(cdfn), cache(nullptr), custom_bfn(true), bfn(bfn) {}

        virtual void g_cache(void) { cdfn(cache); }
        virtual void bind(const int& in) { custom_bfn ? bfn(in, 1, cache.get()) : UType::bind_map.find(type)->second(in, 1, cache.get()); }
    };
    template<typename U, size_t N>
    struct CachedSU : public ShaderUniform {
        typedef std::function<void(std::vector<U>& cache)> cacher_t;

        std::function<void(std::vector<U>& cache)> cdfn;
        std::vector<U> cache;
        bool custom_bfn;
        binder_t bfn;

        CachedSU(std::string name, UType type, cacher_t cdfn)
        : ShaderUniform(name, type), cdfn(cdfn), cache(N), custom_bfn(false) {}
        CachedSU(std::string name, binder_t bfn, cacher_t cdfn)
        : ShaderUniform(name, UType::null()), cdfn(cdfn), cache(N), custom_bfn(true), bfn([](int, int, const void*){}) {}

        virtual void g_cache(void) { cdfn(cache); }
        virtual void bind(const int& in) { custom_bfn ? bfn(in, 1, cache.data()) : UType::bind_map.find(type)->second(in, N, cache.data()); }
    };

    //TODO: figure out how frame buffers work, and use this to generalize all framebuffers
    struct FrameBuffer {};

    /* RenderInputMeta: describe one buffer used in some RenderPass */
    struct RenderInputMeta {
        int position = -1;
        std::string name;
        const void *data = nullptr;
        size_t nelements = 0;
        size_t element_length = 0;
        int element_type = 0;

        size_t getElementSize() const; // simple check: return 12 (3 * 4 bytes) for float3
        RenderInputMeta();
        RenderInputMeta(int _position,
                        const std::string& _name,
                        const void *_data,
                        size_t _nelements,
                        size_t _element_length,
                        int _element_type);
        bool isInteger() const;
    };

    /*
     * RenderDataInput: describe the complete set of buffers used in a RenderPass
     */
    class RenderDataInput {
    public:
        RenderDataInput();
        /*
         * assign: assign per-vertex attribute data
         *      position: glVertexAttribPointer position
         *      name: glBindAttribLocation name
         *      nelements: number of elements
         *      element_length: element dimension, e.g. for vec3 it's 3
         *      element_type: GL_FLOAT or GL_UNSIGNED_INT
         */
        void assign(
            int position,
            const std::string& name,
            const void *data,
            size_t nelements,
            size_t element_length,
            int element_type
        );
        /*
         * assign_index: assign the index buffer for vertices
         * This will bind the data to GL_ELEMENT_ARRAY_BUFFER
         * The element must be uvec3.
         */
        void assignIndex(const void *data, size_t nelements, size_t element_length);
        /*
         * useMaterials: assign materials to the input data
         */
        void useMaterials(const std::vector<std::shared_ptr<Material>>&);

        int getNBuffers() const { return int(meta_.size()); }
        RenderInputMeta getBufferMeta(int i) const { return meta_[i]; }
        bool hasIndex() const { return has_index_; }
        RenderInputMeta getIndexMeta() const { return index_meta_; }

        bool hasMaterial() const { return !materials_.empty(); }
        size_t getNMaterials() const { return materials_.size(); }
        const std::shared_ptr<Material>& getMaterial(size_t id) const { return materials_[id]; }
        std::shared_ptr<Material>& getMaterial(size_t id) { return materials_[id]; }
    private:
        std::vector<RenderInputMeta> meta_;
        std::vector<std::shared_ptr<Material>> materials_;
        RenderInputMeta index_meta_;
        bool has_index_ = false;
    };

    class RenderPass {
    public:
        /*
         * Constructor
         *      vao: the Vertex Array Object, pass -1 to create new
         *      input: RenderDataInput object
         *      shaders: array of shaders, leave the second as nullptr if no GS present
         *      uniforms: array of ShaderUniform objects
         *      output: the FS output variable name.
         * RenderPass does not support render-to-texture or multi-target
         * rendering for now (and you also don't need it).
         */
        RenderPass(
            int vao, // -1: create new VAO, otherwise use given VAO
            const RenderDataInput& input,
            const std::vector<const char*> shaders, // Order: VS, GS, FS
            const std::vector<std::shared_ptr<ShaderUniform>> uniforms,
            const std::vector<const char*> output // Order: 0, 1, 2...
        );
        ~RenderPass();

        unsigned getVAO() const { return unsigned(vao_); }
        void updateVBO(int position, const void* data, size_t nelement);
        void updateIndexVBO(const void* data, size_t nelement);
        void setup(void);
        /*
         * Note: here we don't have an unified render() function, because the
         * reference solution renders with different primitives
         *
         * However you can freely trianglize everything and add an
         * render() function
         */

         /*
          * renderWithMaterial: render a part of vertex buffer, after binding
          * corresponding uniforms for Phong shading.
          */
        bool renderWithMaterial(int i); // return false if material id is invalid
    private:
        void initMaterialUniform();
        void createMaterialTexture();

        int vao_;
        RenderDataInput input_;
        std::vector<std::shared_ptr<ShaderUniform>> uniforms_;
        std::vector<std::vector<std::shared_ptr<ShaderUniform>>> material_uniforms_;

        std::vector<unsigned> glbuffers_, unilocs_, malocs_;
        std::vector<unsigned> gltextures_, matexids_;
        unsigned sampler2d_;
        unsigned vs_ = 0, tcs_ = 0, tes_ = 0, gs_ = 0, fs_ = 0;
        unsigned sp_ = 0;

        static unsigned compileShader(const char*, int type);
        static std::map<const char*, unsigned> shader_cache_;

        static void bindUniforms(
            std::vector<std::shared_ptr<ShaderUniform>>& uniforms,
            const std::vector<unsigned>& unilocs
        );
    };

}

#endif
