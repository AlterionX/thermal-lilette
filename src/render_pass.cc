#include "render_pass.h"
#include "glfw_dispatch.h"
#include "texture_to_render.h"


#include <iostream>
#include <debuggl.h>
#include <map>
#include <unordered_map>
#include <memory>

namespace rpa {

    UType::UType(BASE_TYPE base, NUM_TYPE num, NUM_SUB_TYPE sub, int cnt)
    : base(base), datatype(num), sub(sub), cnt(cnt), cols(1) {}
    UType::UType(BASE_TYPE base, NUM_TYPE num, NUM_SUB_TYPE sub, int row, int col)
    : base(base), datatype(num), sub(sub), rows(row), cols(col) {}
    UType::UType(BASE_TYPE base, TEX_TYPE tex, int dimen)
    : base(base), textype(tex), dimen(dimen) {}
    UType::UType(BASE_TYPE base) : base(base) {}
    UType UType::fm(int size) { return fm(size, size); }
    UType UType::fm(int rs, int cs) { return UType(BASE_TYPE::NUM, NUM_TYPE::FLOAT, NUM_SUB_TYPE::MATRIX, rs, cs); }
    UType UType::fv(int size) { return UType(BASE_TYPE::NUM, NUM_TYPE::FLOAT, NUM_SUB_TYPE::VECTOR, size); }
    UType UType::fs(void) { return UType(BASE_TYPE::NUM, NUM_TYPE::FLOAT, NUM_SUB_TYPE::SINGLE, 1); }
    UType UType::im(int size) { return im(size, size); }
    UType UType::im(int rs, int cs) { return UType(BASE_TYPE::NUM, NUM_TYPE::INT, NUM_SUB_TYPE::MATRIX, rs, cs); }
    UType UType::iv(int size) { return UType(BASE_TYPE::NUM, NUM_TYPE::INT, NUM_SUB_TYPE::VECTOR, size); }
    UType UType::is(void) { return UType(BASE_TYPE::NUM, NUM_TYPE::INT, NUM_SUB_TYPE::SINGLE, 1); }
    UType UType::um(int size) { return um(size, size); }
    UType UType::um(int rs, int cs) { return UType(BASE_TYPE::NUM, NUM_TYPE::UINT, NUM_SUB_TYPE::MATRIX, rs, cs); }
    UType UType::uv(int size) { return UType(BASE_TYPE::NUM, NUM_TYPE::UINT, NUM_SUB_TYPE::VECTOR, size); }
    UType UType::us(void) { return UType(BASE_TYPE::NUM, NUM_TYPE::UINT, NUM_SUB_TYPE::SINGLE, 1); }

    UType UType::tx(int dimen) { return UType(BASE_TYPE::TEX, TEX_TYPE::TEX, dimen); }
    UType UType::ts(int dimen) { return UType(BASE_TYPE::TEX, TEX_TYPE::SAMPLER, dimen); }

    UType UType::null(void) { return UType(BASE_TYPE::NONE); }
    // UTypes
    size_t UType::Hasher::operator()(const UType& h) const {
        size_t bh = std::hash<int>{}(int(h.base));
        size_t bd = std::hash<int>{}(int(h.datatype));
        size_t bs = std::hash<int>{}(int(h.sub));
        size_t br = std::hash<int>{}(int(h.rows));
        size_t bc = std::hash<int>{}(int(h.cols));
        return ((bh << 7) ^ (bd << 14)) + bs + ((br >> 3) ^ (bc << 10));
    }
    bool UType::operator==(const UType& other) const {
        if (base == BASE_TYPE::NUM) {
            return datatype == other.datatype && sub == other.sub && cnt == other.cnt;
        } else if (base == BASE_TYPE::TEX) {
            return textype == other.textype && dimen == other.dimen;
        }
        return false; // NONE
    }
    std::unordered_map<UType, binder_t, UType::Hasher> UType::bind_map = []() {
        return std::unordered_map<UType, binder_t, UType::Hasher>{
            {UType::fm(4), [](int loc, int cnt, const void* data) {
                CHECK_GL_ERROR(glUniformMatrix4fv(loc, cnt, false, (const GLfloat*) data));
            }},
            {UType::fv(4), [](int loc, int cnt, const void* data) {
                CHECK_GL_ERROR(glUniform4fv(loc, cnt, (const GLfloat*) data));
            }},
            {UType::fv(3), [](int loc, int cnt, const void* data) {
                CHECK_GL_ERROR(glUniform3fv(loc, cnt, (const GLfloat*) data));
            }},
            {UType::fs(), [](int loc, int cnt, const void* data) {
                CHECK_GL_ERROR(glUniform1fv(loc, cnt, (const GLfloat*) data));
            }},
            {UType::is(), [](int loc, int cnt, const void* data) {
                CHECK_GL_ERROR(glUniform1iv(loc, cnt, (const GLint*) data));
            }},
            {UType::ts(2), [](int loc, int cnt, const void* data) {
                CHECK_GL_ERROR(glBindSampler(0, (GLuint) (long) data));
            }},
            {UType::tx(2), [](int loc, int cnt, const void* data) {
                CHECK_GL_ERROR(glUniform1i(loc, 0));
                CHECK_GL_ERROR(glActiveTexture(GL_TEXTURE0 + 0));
                CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, (long) data));
            }}
        };
    }();
    ShaderUniform::ShaderUniform(std::string name, UType type)
    : name(name), type(type) {}

    /*
     * For students:
     *
     * Although RenderPass simplifies the implementation of the reference code.
     * THE USE OF RENDERPASS CLASS IS TOTALLY OPTIONAL.
     * You can implement your system without even taking a look of this.
     */

    RenderInputMeta::RenderInputMeta() {}

    bool RenderInputMeta::isInteger() const {
        return element_type == GL_INT || element_type == GL_UNSIGNED_INT;
    }

    RenderInputMeta::RenderInputMeta(int _position,
                                     const std::string& _name,
                                     const void *_data,
                                     size_t _nelements,
                                     size_t _element_length,
                                     int _element_type)
        :position(_position), name(_name), data(_data),
        nelements(_nelements), element_length(_element_length),
        element_type(_element_type) {}

    RenderDataInput::RenderDataInput() {}


    RenderPass::RenderPass(int vao, // -1: create new VAO, otherwise use given VAO
                           const RenderDataInput& input,
                           const std::vector<const char*> shaders, // Order: VS, TCS, TES, GS, FS
                           const std::vector<std::shared_ptr<ShaderUniform>> uniforms,
                           const std::vector<const char*> output // Order: 0, 1, 2...
    ) : vao_(vao), input_(input), uniforms_(uniforms) {
        gdm::qd([&]GDM_OP{
            if (vao_ < 0) {
                CHECK_GL_ERROR(glGenVertexArrays(1, (GLuint*) &vao_));
            }
            CHECK_GL_ERROR(glBindVertexArray(vao_));

            // Program first
            vs_ = compileShader(shaders[0], GL_VERTEX_SHADER);
            tcs_ = compileShader(shaders[1], GL_TESS_CONTROL_SHADER);
            tes_ = compileShader(shaders[2], GL_TESS_EVALUATION_SHADER);
            gs_ = compileShader(shaders[3], GL_GEOMETRY_SHADER);
            fs_ = compileShader(shaders[4], GL_FRAGMENT_SHADER);

            CHECK_GL_ERROR(sp_ = glCreateProgram());
            glAttachShader(sp_, vs_);
            glAttachShader(sp_, fs_);
            if (shaders[1])
                glAttachShader(sp_, tcs_);
            if (shaders[2])
                glAttachShader(sp_, tes_);
            if (shaders[3])
                glAttachShader(sp_, gs_);
            // ... and then buffers
            size_t nbuffer = input.getNBuffers();
            if (input.hasIndex())
                nbuffer++;
            glbuffers_.resize(nbuffer);
            CHECK_GL_ERROR(glGenBuffers(nbuffer, glbuffers_.data()));
            for (int i = 0; i < input.getNBuffers(); i++) {
                auto meta = input.getBufferMeta(i);
                CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, glbuffers_[i]));
                CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
                                            meta.getElementSize() * meta.nelements,
                                            meta.data,
                                            GL_STATIC_DRAW));
                if (meta.isInteger()) {
                    CHECK_GL_ERROR(glVertexAttribIPointer(meta.position,
                                                          meta.element_length,
                                                          meta.element_type,
                                                          0, 0));
                } else {
                    CHECK_GL_ERROR(glVertexAttribPointer(meta.position,
                                                         meta.element_length,
                                                         meta.element_type,
                                                         GL_FALSE, 0, 0));
                }
                CHECK_GL_ERROR(glEnableVertexAttribArray(meta.position));
                // ... because we need program to bind location
                CHECK_GL_ERROR(glBindAttribLocation(sp_, meta.position, meta.name.c_str()));
            }
            // .. bind output position
            for (size_t i = 0; i < output.size(); i++) {
                CHECK_GL_ERROR(glBindFragDataLocation(sp_, i, output[i]));
            }
            // ... then we can link
            glLinkProgram(sp_);
            CHECK_GL_PROGRAM_ERROR(sp_);

            if (input.hasIndex()) {
                auto meta = input.getIndexMeta();
                CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                                            glbuffers_.back()
                ));
                CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                                            meta.getElementSize() * meta.nelements,
                                            meta.data, GL_STATIC_DRAW));
            }
            // after linking uniform locations can be determined
            unilocs_.resize(uniforms.size());
            for (size_t i = 0; i < uniforms.size(); i++) {
                CHECK_GL_ERROR(unilocs_[i] = glGetUniformLocation(sp_, uniforms[i]->name.c_str()));
            }
            if (input_.hasMaterial()) {
                createMaterialTexture();
                initMaterialUniform();
            }
        });
    }

    void RenderPass::initMaterialUniform() {
        material_uniforms_.clear();
        for (size_t i = 0; i < input_.getNMaterials(); i++) {
            auto& ma = input_.getMaterial(i);
            auto diffuse_data = [&ma](std::vector<glm::vec4>& cache){ cache[0] = ma->diffuse; };
            auto ambient_data = [&ma](std::vector<glm::vec4>& cache){ cache[0] = ma->ambient; };
            auto specular_data = [&ma](std::vector<glm::vec4>& cache){ cache[0] = ma->specular; };
            auto shininess_data = [&ma](std::vector<float>& cache){ cache[0] = ma->shininess; };
            int texid = matexids_[i];
            auto texture_data = [texid](std::vector<intptr_t>& cache){ cache[0] = texid; };
            int sampler2d = sampler2d_;
            auto sampler_data = [sampler2d](std::vector<intptr_t>& cache){ cache[0] = sampler2d; };

            auto diffuse = std::make_shared<CachedSU<glm::vec4, 1>>("diffuse", UType::fv(4), diffuse_data);
            auto ambient = std::make_shared<CachedSU<glm::vec4, 1>>("ambient", UType::fv(4), ambient_data);
            auto specular = std::make_shared<CachedSU<glm::vec4, 1>>("specular", UType::fv(4), specular_data);
            auto shininess = std::make_shared<CachedSU<float, 1>>("shininess", UType::fs(), shininess_data);
            auto texture = std::make_shared<CachedSU<intptr_t, 1>>("GL_TEXTURE_2D", UType::tx(2), texture_data);
            auto sampler = std::make_shared<CachedSU<intptr_t, 1>>("textureSampler", UType::ts(2), sampler_data);
            std::vector<std::shared_ptr<ShaderUniform>> munis = {diffuse, ambient, specular, shininess, texture, sampler};
            material_uniforms_.emplace_back(munis);
        }
        malocs_.clear();
        CHECK_GL_ERROR(malocs_.emplace_back(glGetUniformLocation(sp_, "diffuse")));
        CHECK_GL_ERROR(malocs_.emplace_back(glGetUniformLocation(sp_, "ambient")));
        CHECK_GL_ERROR(malocs_.emplace_back(glGetUniformLocation(sp_, "specular")));
        CHECK_GL_ERROR(malocs_.emplace_back(glGetUniformLocation(sp_, "shininess")));
        CHECK_GL_ERROR(malocs_.emplace_back(glGetUniformLocation(sp_, "textureSampler")));
        CHECK_GL_ERROR(malocs_.emplace_back(glGetUniformLocation(sp_, "textureSampler")));
        // std::cerr << "textureSampler location: " << malocs_.back() << std::endl;
    }

    /*
     * Create textures to gltextures_
     * and assign material specified textures to matexids_
     *
     * Different materials may share textures
     */
    void RenderPass::createMaterialTexture() {
        CHECK_GL_ERROR(glActiveTexture(GL_TEXTURE0 + 0));
        matexids_.clear();
        std::map<Image*, unsigned> tex2id;
        for (size_t i = 0; i < input_.getNMaterials(); i++) {
            auto& ma = input_.getMaterial(i);
        #if 0
            std::cerr << __func__ << " Material " << i << " has texture pointer " << ma->texture.get() << std::endl;
        #endif
            if (!ma->texture) {
                matexids_.emplace_back(0);
                continue;
            }
            // Do not create multiple texture for the same data.
            auto iter = tex2id.find(ma->texture.get());
            if (iter != tex2id.end()) {
                matexids_.emplace_back(iter->second);
                continue;
            }

            // Now create and upload texture data
            int w = ma->texture->width;
            int h = ma->texture->height;
            // TODO: enable stride
            // Translate RGB to RGBA for alignment
            std::vector<unsigned int> dummy(w*h);
            const unsigned char* bytes = ma->texture->bytes.data();
            for (int row = 0; row < h; row++) {
                for (int col = 0; col < w; col++) {
                    unsigned r = bytes[row*w * 3 + col * 3];
                    unsigned g = bytes[row*w * 3 + col * 3 + 1];
                    unsigned b = bytes[row*w * 3 + col * 3 + 1];
                    dummy[row*w + col] = r | (g << 8) | (b << 16) | (0xFF << 24);
                }
            }
            GLuint tex = 0;
            CHECK_GL_ERROR(glGenTextures(1, &tex));
            CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, tex));
            CHECK_GL_ERROR(glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8,
                                          w,
                                          h));
            CHECK_GL_ERROR(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h,
                                           GL_RGBA, GL_UNSIGNED_BYTE,
                                           dummy.data()));
            //CHECK_GL_ERROR(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));
            std::cerr << __func__ << " load data into texture " << tex <<
                " dim: " << w << " x " << h << std::endl;
            CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, 0));
            matexids_.emplace_back(tex);
            tex2id[ma->texture.get()] = tex;
        }
        CHECK_GL_ERROR(glGenSamplers(1, &sampler2d_));
        CHECK_GL_ERROR(glSamplerParameteri(sampler2d_, GL_TEXTURE_WRAP_S, GL_REPEAT));
        CHECK_GL_ERROR(glSamplerParameteri(sampler2d_, GL_TEXTURE_WRAP_T, GL_REPEAT));
        CHECK_GL_ERROR(glSamplerParameteri(sampler2d_, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        CHECK_GL_ERROR(glSamplerParameteri(sampler2d_, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    }

    RenderPass::~RenderPass() {
        // TODO: Free resources
    }

    void RenderPass::updateVBO(int position, const void* data, size_t size) {
        int bufferid = -1;
        for (int i = 0; i < input_.getNBuffers(); i++) {
            auto meta = input_.getBufferMeta(i);
            if (meta.position == position) {
                bufferid = i;
                break;
            }
        }
        if (bufferid < 0)
            throw __func__ + std::string(": error, can't find buffer with position ") + std::to_string(position);
        auto meta = input_.getBufferMeta(bufferid);
        CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, glbuffers_[bufferid]));
        CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
                                    size * meta.getElementSize(),
                                    data, GL_STATIC_DRAW));
    }
    void RenderPass::updateIndexVBO(const void* data, size_t size) {
        CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glbuffers_.back()));
        CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                                    size * 12,
                                    data, GL_STATIC_DRAW));
    }

    void RenderPass::setup() {
        // Switch to our object VAO.
        CHECK_GL_ERROR(glBindVertexArray(vao_));
        // Use our program.
        CHECK_GL_ERROR(glUseProgram(sp_));
        bindUniforms(uniforms_, unilocs_);
    }

    bool RenderPass::renderWithMaterial(int mid) {
        if (mid >= int(material_uniforms_.size()) || mid < 0)
            return false;
        const auto& mat = input_.getMaterial(mid);
        auto& matuni = material_uniforms_[mid];
        bindUniforms(matuni, malocs_);
        CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, mat->nfaces * 3,
                                      GL_UNSIGNED_INT,
                                      (const void*) (mat->offset * 3 * 4)) // Offset is in bytes
        );
        return true;
    }

    void RenderPass::bindUniforms(
        std::vector<std::shared_ptr<ShaderUniform>>& uniforms,
        const std::vector<unsigned>& unilocs
    ) {
        for (size_t i = 0; i < uniforms.size(); i++) {
            uniforms[i]->g_cache();
            CHECK_GL_ERROR(uniforms[i]->bind(unilocs[i]));
        }
    }

    unsigned RenderPass::compileShader(const char* source_ptr, int type) {
        if (!source_ptr)
            return 0;
        auto iter = shader_cache_.find(source_ptr);
        if (iter != shader_cache_.end()) {
            return iter->second;
        }
        GLuint ret = 0;
        CHECK_GL_ERROR(ret = glCreateShader(type));
        CHECK_GL_ERROR(glShaderSource(ret, 1, &source_ptr, nullptr));
        glCompileShader(ret);
        CHECK_GL_SHADER_ERROR(ret);
        shader_cache_[source_ptr] = ret;
        return ret;
    }

    void RenderDataInput::assign(int position,
                                 const std::string& name,
                                 const void *data,
                                 size_t nelements,
                                 size_t element_length,
                                 int element_type) {
        meta_.emplace_back(position, name, data, nelements, element_length, element_type);
    }

    void RenderDataInput::assignIndex(const void *data, size_t nelements, size_t element_length) {
        has_index_ = true;
        index_meta_ = {-1, "", data, nelements, element_length, GL_UNSIGNED_INT};
    }

    void RenderDataInput::useMaterials(const std::vector<std::shared_ptr<Material>>& ms) {
        materials_ = ms;
        for (const auto& ma : ms) {
            std::cerr << "Use Material from " << ma->offset << " size: " << ma->nfaces << std::endl;
        }
    }

    size_t RenderInputMeta::getElementSize() const {
        size_t element_size = 4;
        if (element_type == GL_FLOAT)
            element_size = 4;
        else if (element_type == GL_UNSIGNED_INT)
            element_size = 4;
        else if (element_type == GL_INT)
            element_size = 4;
        return element_size * element_length;
    }

    std::map<const char*, unsigned> RenderPass::shader_cache_;
}
