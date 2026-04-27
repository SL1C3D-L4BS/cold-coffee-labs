// editor/render/editor_scene_pass.cpp — Wave 1C scene raster (unlit, depth).
//
// vk_mem_alloc includes vulkan.h; volk must load with prototypes disabled
// (matches editor_app.cpp and engine_render).
#define VK_NO_PROTOTYPES
#include <volk.h>
#include <vk_mem_alloc.h>

#include "editor/render/editor_scene_pass.hpp"

#include "editor/scene/components.hpp"
#include "engine/anim/animation_world.hpp"
#include "engine/anim/anim_types.hpp"
#include "engine/assets/asset_db.hpp"
#include "engine/assets/mesh_asset.hpp"
#include "engine/ecs/entity.hpp"
#include "engine/ecs/world.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

#include <array>
#include <algorithm>
#include <cstdio>
#include <span>
#include <utility>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

#ifndef GW_MOD_BUILD
#define GW_MOD_BUILD "."
#endif

namespace gw::editor::render {
namespace {

[[nodiscard]] std::vector<std::uint32_t> read_spv(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    if (!f) return {};
    const std::streamsize sz = f.tellg();
    f.seekg(0);
    if (sz < 0 || (sz % 4) != 0) return {};
    std::vector<std::uint32_t> out(static_cast<std::size_t>(sz) / 4u);
    f.read(reinterpret_cast<char*>(out.data()), sz);
    return out;
}

[[nodiscard]] VkShaderModule mod_from_spv(VkDevice dev, const std::vector<std::uint32_t>& code) {
    if (code.empty()) return VK_NULL_HANDLE;
    VkShaderModuleCreateInfo ci{};
    ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = code.size() * 4u;
    ci.pCode    = code.data();
    VkShaderModule m{};
    return (vkCreateShaderModule(dev, &ci, nullptr, &m) == VK_SUCCESS) ? m : VK_NULL_HANDLE;
}

struct FrameUbo {
    glm::mat4 view{1.f};
    glm::mat4 proj{1.f};
};

struct alignas(16) PushPc {
    glm::mat4 model{1.f};
    glm::vec4 color{1.f};
};

struct alignas(16) MeshDrawPc {
    glm::mat4     model{1.f};
    glm::vec4     color{1.f};
    glm::vec3     pos_scale{0.f};
    std::uint32_t first_index{0};
    glm::vec3     pos_bias{0.f};
    std::uint32_t index_count{0};
    std::uint32_t skin_flags{0};
    std::uint32_t joint_count{0};
    std::uint32_t palette_base{0};
};

static constexpr std::uint32_t kEditorSkinPaletteMaxMatrices = 512u;
static constexpr VkDeviceSize  kEditorSkinPaletteBytes =
    static_cast<VkDeviceSize>(static_cast<std::uint64_t>(kEditorSkinPaletteMaxMatrices) * sizeof(glm::mat4));

static_assert(sizeof(glm::mat4) == gw::assets::kInverseBindMat4Bytes);

// --------------------------------------------------------------------------
// 12 tris, non-indexed; positions only (24 unique verts expanded to 36*3)
// Y+ up, +Z forward, CCW.
// --------------------------------------------------------------------------
void fill_unit_cube_positions(float* p) {
    // 8 corners
    const float v[8][3] = {
        {-0.5f, -0.5f, -0.5f},
        {0.5f,  -0.5f, -0.5f},
        {0.5f,  0.5f,  -0.5f},
        {-0.5f, 0.5f,  -0.5f},
        {-0.5f, -0.5f, 0.5f },
        {0.5f,  -0.5f, 0.5f },
        {0.5f,  0.5f,  0.5f },
        {-0.5f, 0.5f,  0.5f },
    };
    // 2 tris per face, 6 faces (order: +Z, -Z, +X, -X, +Y, -Y)
    static const std::uint8_t t[12][3] = {
        {4,5,6},{4,6,7},
        {1,0,3},{1,3,2},
        {5,1,2},{5,2,6},
        {0,4,7},{0,7,3},
        {3,7,6},{3,6,2},
        {0,1,5},{0,5,4},
    };
    std::size_t o = 0;
    for (auto& f : t) {
        for (int k = 0; k < 3; ++k) {
            p[o + 0] = v[f[k]][0];
            p[o + 1] = v[f[k]][1];
            p[o + 2] = v[f[k]][2];
            o += 3;
        }
    }
}

static constexpr int kFif = 2; // match editor kFramesInFlight
static_assert(kFif == 2, "keep in sync with editor_app.cpp kFramesInFlight");

const glm::vec4 kBlockoutColor{0.90f, 0.40f, 0.12f, 1.f};
const glm::vec4 kPlaceholderColor{0.35f, 0.55f, 0.85f, 1.f};

} // namespace

struct EditorScenePass::Impl {
    VkDevice     device_    = VK_NULL_HANDLE;
    VmaAllocator alloc_     = nullptr;
    VkFormat     color_fmt_ = VK_FORMAT_UNDEFINED;
    VkFormat     depth_fmt_ = VK_FORMAT_UNDEFINED;

    VkShaderModule  vert_ = VK_NULL_HANDLE, frag_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout dsl_      = VK_NULL_HANDLE;
    VkDescriptorSetLayout dsl_mesh_ = VK_NULL_HANDLE;
    VkPipelineLayout  pll_  = VK_NULL_HANDLE;
    VkPipeline        p_    = VK_NULL_HANDLE;
    VkPipeline        p_mesh_   = VK_NULL_HANDLE;
    VkPipelineLayout  pll_mesh_  = VK_NULL_HANDLE;
    VkShaderModule    vert_mesh_  = VK_NULL_HANDLE;
    VkShaderModule    frag_mesh_  = VK_NULL_HANDLE;
    VkDescriptorPool  pool_  = VK_NULL_HANDLE;

    VkBuffer          vb_     = VK_NULL_HANDLE;
    VmaAllocation     vba_    = nullptr;
    VkDeviceSize     vb_nbytes_ = 0;

    std::array<VkBuffer,     kFif> ubo_{};
    std::array<VmaAllocation, kFif> ubao_{};
    std::array<void*, kFif>       map_{};

    std::array<VkDescriptorSet, kFif> dset_{};
    std::array<VkDescriptorSet, kFif> dset_mesh_{};

    std::array<VkBuffer, kFif>     skin_palette_buf_{};
    std::array<VmaAllocation, kFif> skin_palette_alloc_{};
    std::array<void*, kFif>         skin_palette_map_{};

    VkBuffer      skin_dummy_vb_   = VK_NULL_HANDLE;
    VmaAllocation skin_dummy_alloc_ = nullptr;

    bool make_vb() {
        float    verts[36 * 3];
        fill_unit_cube_positions(verts);
        vb_nbytes_ = sizeof(verts);
        VkBufferCreateInfo bci{};
        bci.sType         = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bci.size          = vb_nbytes_;
        bci.usage         = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bci.sharingMode  = VK_SHARING_MODE_EXCLUSIVE;
        VmaAllocationCreateInfo aci{};
        aci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                    VMA_ALLOCATION_CREATE_MAPPED_BIT;
        aci.usage  = VMA_MEMORY_USAGE_AUTO;
        VmaAllocationInfo ainfo{};
        if (vmaCreateBuffer(alloc_, &bci, &aci, &vb_, &vba_, &ainfo) != VK_SUCCESS) {
            return false;
        }
        if (ainfo.pMappedData) {
            std::memcpy(ainfo.pMappedData, verts, sizeof(verts));
        }
        return true;
    }

    void del_vb() {
        if (alloc_ && vb_) vmaDestroyBuffer(alloc_, vb_, vba_);
        vb_ = VK_NULL_HANDLE, vba_ = nullptr, vb_nbytes_ = 0;
    }
};

EditorScenePass::EditorScenePass() = default;

EditorScenePass::~EditorScenePass() { shutdown(); }

void EditorScenePass::shutdown() noexcept {
    if (!impl_) return;
    auto* t = impl_.get();
    if (t->device_) {
        vkDeviceWaitIdle(t->device_);
        t->del_vb();
        for (int i = 0; i < kFif; ++i) {
            if (t->ubao_[i] && t->alloc_) {
                // UBOs use VMA_ALLOCATION_CREATE_MAPPED_BIT + pMappedData — never vmaMapMemory.
                // vmaUnmapMemory is only for explicit maps; unmapping here trips VMA's assert.
                t->map_[i] = nullptr;
                vmaDestroyBuffer(t->alloc_, t->ubo_[i], t->ubao_[i]);
            }
            t->ubo_[i]  = VK_NULL_HANDLE;
            t->ubao_[i] = nullptr;
        }
        for (int i = 0; i < kFif; ++i) {
            if (t->skin_palette_buf_[i] && t->alloc_) {
                t->skin_palette_map_[i] = nullptr;
                vmaDestroyBuffer(t->alloc_, t->skin_palette_buf_[i], t->skin_palette_alloc_[i]);
            }
            t->skin_palette_buf_[i]   = VK_NULL_HANDLE;
            t->skin_palette_alloc_[i] = nullptr;
        }
        if (t->skin_dummy_vb_ && t->alloc_) {
            vmaDestroyBuffer(t->alloc_, t->skin_dummy_vb_, t->skin_dummy_alloc_);
        }
        t->skin_dummy_vb_   = VK_NULL_HANDLE;
        t->skin_dummy_alloc_ = nullptr;
        if (t->pool_) vkDestroyDescriptorPool(t->device_, t->pool_, nullptr);
        t->pool_ = VK_NULL_HANDLE;
        if (t->p_mesh_)  vkDestroyPipeline(t->device_, t->p_mesh_, nullptr);
        if (t->pll_mesh_) vkDestroyPipelineLayout(t->device_, t->pll_mesh_, nullptr);
        if (t->vert_mesh_) vkDestroyShaderModule(t->device_, t->vert_mesh_, nullptr);
        if (t->frag_mesh_) vkDestroyShaderModule(t->device_, t->frag_mesh_, nullptr);
        if (t->p_)       vkDestroyPipeline(t->device_, t->p_, nullptr);
        if (t->pll_)     vkDestroyPipelineLayout(t->device_, t->pll_, nullptr);
        if (t->dsl_mesh_) vkDestroyDescriptorSetLayout(t->device_, t->dsl_mesh_, nullptr);
        if (t->dsl_)     vkDestroyDescriptorSetLayout(t->device_, t->dsl_, nullptr);
        if (t->vert_)    vkDestroyShaderModule(t->device_, t->vert_, nullptr);
        if (t->frag_)    vkDestroyShaderModule(t->device_, t->frag_, nullptr);
        t->p_mesh_   = VK_NULL_HANDLE;
        t->pll_mesh_ = VK_NULL_HANDLE;
        t->vert_mesh_ = VK_NULL_HANDLE;
        t->frag_mesh_ = VK_NULL_HANDLE;
        t->p_     = VK_NULL_HANDLE;
        t->pll_       = VK_NULL_HANDLE;
        t->dsl_mesh_  = VK_NULL_HANDLE;
        t->dsl_       = VK_NULL_HANDLE;
        t->vert_  = VK_NULL_HANDLE;
        t->frag_  = VK_NULL_HANDLE;
    }
    t->device_    = VK_NULL_HANDLE;
    t->alloc_     = nullptr;
    t->color_fmt_ = VK_FORMAT_UNDEFINED;
    t->depth_fmt_ = VK_FORMAT_UNDEFINED;
    impl_.reset();
}

bool EditorScenePass::init_or_recreate(
    VkDevice d, VmaAllocator a, std::uint32_t color_u, std::uint32_t depth_u) {
    if (d == VK_NULL_HANDLE || !a) return false;
    const VkFormat color  = static_cast<VkFormat>(color_u);
    const VkFormat depth  = (depth_u == 0) ? VK_FORMAT_UNDEFINED : static_cast<VkFormat>(depth_u);
    if (color == VK_FORMAT_UNDEFINED) {
        return false;
    }

    if (!impl_) impl_ = std::make_unique<Impl>();
    auto* t = impl_.get();

    if (t->device_ == d && t->alloc_ == a && t->color_fmt_ == color && t->depth_fmt_ == depth
        && t->p_ != VK_NULL_HANDLE) {
        return true;
    }
    if (t->p_) shutdown();
    if (!impl_) impl_ = std::make_unique<Impl>();
    t = impl_.get();
    t->device_   = d;
    t->alloc_    = a;
    t->color_fmt_ = color;
    t->depth_fmt_ = depth;
    if (!t->make_vb()) {
        impl_.reset();
        return false;
    }

    const std::vector<std::uint32_t> vsb = read_spv(std::filesystem::path{GW_MOD_BUILD} / "shaders" / "editor" / "scene_unlit.vert.spv");
    const std::vector<std::uint32_t> fsp = read_spv(std::filesystem::path{GW_MOD_BUILD} / "shaders" / "editor" / "scene_unlit.frag.spv");
    t->vert_  = mod_from_spv(d, vsb);
    t->frag_  = mod_from_spv(d, fsp);
    if (!t->vert_ || !t->frag_) {
        impl_.reset();
        return false;
    }

    VkDescriptorSetLayoutBinding b{};
    b.binding         = 0;
    b.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    b.descriptorCount = 1;
    b.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo dl{};
    dl.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dl.bindingCount = 1;
    dl.pBindings   = &b;
    if (vkCreateDescriptorSetLayout(d, &dl, nullptr, &t->dsl_) != VK_SUCCESS) {
        impl_.reset();
        return false;
    }

    VkDescriptorSetLayoutBinding mesh_b[2]{};
    mesh_b[0].binding         = 0;
    mesh_b[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    mesh_b[0].descriptorCount = 1;
    mesh_b[0].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    mesh_b[1].binding         = 1;
    mesh_b[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    mesh_b[1].descriptorCount = 1;
    mesh_b[1].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
    VkDescriptorSetLayoutCreateInfo dl_mesh{};
    dl_mesh.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dl_mesh.bindingCount = 2;
    dl_mesh.pBindings    = mesh_b;
    if (vkCreateDescriptorSetLayout(d, &dl_mesh, nullptr, &t->dsl_mesh_) != VK_SUCCESS) {
        impl_.reset();
        return false;
    }

    const VkPushConstantRange pcr{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                 static_cast<std::uint32_t>(sizeof(PushPc))};

    VkPipelineLayoutCreateInfo pli{};
    pli.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pli.setLayoutCount         = 1;
    pli.pSetLayouts            = &t->dsl_;
    pli.pushConstantRangeCount  = 1;
    pli.pPushConstantRanges     = &pcr;
    if (vkCreatePipelineLayout(d, &pli, nullptr, &t->pll_) != VK_SUCCESS) {
        impl_.reset();
        return false;
    }

    for (int i = 0; i < kFif; ++i) {
        VkBufferCreateInfo ubo_bi{};
        ubo_bi.sType     = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        ubo_bi.size      = sizeof(FrameUbo);
        ubo_bi.usage     = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        ubo_bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VmaAllocationCreateInfo uaci{};
        uaci.flags     = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                        VMA_ALLOCATION_CREATE_MAPPED_BIT;
        uaci.usage     = VMA_MEMORY_USAGE_AUTO;
        VmaAllocationInfo ai{};
        if (vmaCreateBuffer(a, &ubo_bi, &uaci, &t->ubo_[i], &t->ubao_[i], &ai) != VK_SUCCESS) {
            impl_.reset();
            return false;
        }
        t->map_[i] = ai.pMappedData;
    }

    for (int i = 0; i < kFif; ++i) {
        VkBufferCreateInfo pbi{};
        pbi.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        pbi.size        = kEditorSkinPaletteBytes;
        pbi.usage       = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        pbi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VmaAllocationCreateInfo pai{};
        pai.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                    VMA_ALLOCATION_CREATE_MAPPED_BIT;
        pai.usage = VMA_MEMORY_USAGE_AUTO;
        VmaAllocationInfo paiout{};
        if (vmaCreateBuffer(a, &pbi, &pai, &t->skin_palette_buf_[i], &t->skin_palette_alloc_[i],
                &paiout) != VK_SUCCESS) {
            impl_.reset();
            return false;
        }
        t->skin_palette_map_[i] = paiout.pMappedData;
    }

    {
        VkBufferCreateInfo dbi{};
        dbi.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        dbi.size        = 12;
        dbi.usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        dbi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VmaAllocationCreateInfo daci{};
        daci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                     VMA_ALLOCATION_CREATE_MAPPED_BIT;
        daci.usage = VMA_MEMORY_USAGE_AUTO;
        VmaAllocationInfo dai{};
        if (vmaCreateBuffer(a, &dbi, &daci, &t->skin_dummy_vb_, &t->skin_dummy_alloc_, &dai) !=
            VK_SUCCESS) {
            impl_.reset();
            return false;
        }
        if (dai.pMappedData != nullptr) {
            const std::uint32_t dummy[3] = {0u, 0u, 255u};
            std::memcpy(dai.pMappedData, dummy, sizeof(dummy));
        }
    }

    VkDescriptorPoolSize pss[2]{};
    pss[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pss[0].descriptorCount = kFif * 2;
    pss[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pss[1].descriptorCount = kFif;
    VkDescriptorPoolCreateInfo dp{};
    dp.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dp.maxSets       = kFif * 2;
    dp.poolSizeCount = 2;
    dp.pPoolSizes    = pss;
    if (vkCreateDescriptorPool(d, &dp, nullptr, &t->pool_) != VK_SUCCESS) {
        impl_.reset();
        return false;
    }

    std::array<VkDescriptorSetLayout, kFif> dsls{};
    dsls.fill(t->dsl_);
    VkDescriptorSetAllocateInfo dsa{};
    dsa.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsa.descriptorPool     = t->pool_;
    dsa.descriptorSetCount = kFif;
    dsa.pSetLayouts        = dsls.data();
    if (vkAllocateDescriptorSets(d, &dsa, t->dset_.data()) != VK_SUCCESS) {
        impl_.reset();
        return false;
    }
    std::array<VkDescriptorSetLayout, kFif> dsls_mesh{};
    dsls_mesh.fill(t->dsl_mesh_);
    VkDescriptorSetAllocateInfo dsam{};
    dsam.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsam.descriptorPool     = t->pool_;
    dsam.descriptorSetCount = kFif;
    dsam.pSetLayouts        = dsls_mesh.data();
    if (vkAllocateDescriptorSets(d, &dsam, t->dset_mesh_.data()) != VK_SUCCESS) {
        impl_.reset();
        return false;
    }
    for (int i = 0; i < kFif; ++i) {
        VkDescriptorBufferInfo bi{};
        bi.buffer = t->ubo_[i];
        bi.offset  = 0;
        bi.range  = sizeof(FrameUbo);
        VkWriteDescriptorSet w{};
        w.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w.dstSet           = t->dset_[i];
        w.dstBinding       = 0;
        w.dstArrayElement  = 0;
        w.descriptorCount  = 1;
        w.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        w.pBufferInfo      = &bi;
        vkUpdateDescriptorSets(d, 1, &w, 0, nullptr);
    }
    for (int i = 0; i < kFif; ++i) {
        VkDescriptorBufferInfo bis[2]{};
        bis[0].buffer = t->ubo_[i];
        bis[0].offset = 0;
        bis[0].range  = sizeof(FrameUbo);
        bis[1].buffer = t->skin_palette_buf_[i];
        bis[1].offset = 0;
        bis[1].range  = kEditorSkinPaletteBytes;
        VkWriteDescriptorSet ws[2]{};
        ws[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        ws[0].dstSet          = t->dset_mesh_[i];
        ws[0].dstBinding      = 0;
        ws[0].dstArrayElement = 0;
        ws[0].descriptorCount = 1;
        ws[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ws[0].pBufferInfo     = &bis[0];
        ws[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        ws[1].dstSet          = t->dset_mesh_[i];
        ws[1].dstBinding      = 1;
        ws[1].dstArrayElement = 0;
        ws[1].descriptorCount = 1;
        ws[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        ws[1].pBufferInfo     = &bis[1];
        vkUpdateDescriptorSets(d, 2, ws, 0, nullptr);
    }

    VkVertexInputBindingDescription vbd{};
    vbd.binding   = 0;
    vbd.stride    = sizeof(float) * 3;
    vbd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    VkVertexInputAttributeDescription via{};
    via.location = 0;
    via.binding  = 0;
    via.format   = VK_FORMAT_R32G32B32_SFLOAT;
    via.offset   = 0;

    VkPipelineVertexInputStateCreateInfo vii{};
    vii.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vii.vertexBindingDescriptionCount  = 1;
    vii.pVertexBindingDescriptions   = &vbd;
    vii.vertexAttributeDescriptionCount = 1;
    vii.pVertexAttributeDescriptions  = &via;

    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo vpst{};
    vpst.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vpst.viewportCount  = 1;
    vpst.pViewports     = nullptr; // dynamic
    vpst.scissorCount  = 1;
    vpst.pScissors     = nullptr;

    VkPipelineRasterizationStateCreateInfo rast{};
    rast.sType     = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rast.cullMode  = VK_CULL_MODE_NONE; // 1C: keep visible without winding fights
    rast.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rast.lineWidth = 1.f;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo ds{};
    ds.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable  = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp  = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState cba{};
    cba.colorWriteMask = 0xF; // all channels
    VkPipelineColorBlendStateCreateInfo cbs{};
    cbs.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbs.attachmentCount = 1;
    cbs.pAttachments  = &cba;

    const VkDynamicState     ds_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dyn_st{};
    dyn_st.sType            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn_st.dynamicStateCount  = 2;
    dyn_st.pDynamicStates    = ds_states;

    VkFormat use_depth = depth;
    if (use_depth == VK_FORMAT_UNDEFINED) {
        use_depth = VK_FORMAT_D32_SFLOAT;
    }

    VkPipelineRenderingCreateInfo pri{};
    pri.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pri.colorAttachmentCount  = 1;
    pri.pColorAttachmentFormats = &color;
    pri.depthAttachmentFormat  = use_depth;
    if (use_depth == VK_FORMAT_D32_SFLOAT_S8_UINT || use_depth == VK_FORMAT_D24_UNORM_S8_UINT) {
        pri.stencilAttachmentFormat = use_depth;
    } else {
        pri.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
    }

    const VkShaderModule  mods[2] = {t->vert_, t->frag_};
    const char*         names[2]  = {"main", "main"};
    VkPipelineShaderStageCreateInfo sh[2]{};
    for (int s = 0; s < 2; ++s) {
        sh[s].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        sh[s].stage = s == 0 ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
        sh[s].module  = mods[s];
        sh[s].pName   = names[s];
    }

    VkGraphicsPipelineCreateInfo gpi{};
    gpi.sType    = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gpi.pNext   = &pri;
    gpi.stageCount = 2;
    gpi.pStages  = sh;
    gpi.pVertexInputState   = &vii;
    gpi.pInputAssemblyState  = &ia;
    gpi.pViewportState       = &vpst;
    gpi.pRasterizationState  = &rast;
    gpi.pMultisampleState    = &ms;
    gpi.pDepthStencilState   = &ds;
    gpi.pColorBlendState     = &cbs;
    gpi.pDynamicState        = &dyn_st;
    gpi.layout               = t->pll_;
    gpi.subpass              = 0;

    if (vkCreateGraphicsPipelines(d, VK_NULL_HANDLE, 1, &gpi, nullptr, &t->p_) != VK_SUCCESS) {
        impl_.reset();
        return false;
    }

    // Cooked `.gwmesh` path (quantized stream0 + GPU index buffer).
    const std::vector<std::uint32_t> vs_mesh = read_spv(
        std::filesystem::path{GW_MOD_BUILD} / "shaders" / "editor" / "mesh_scene_unlit.vert.spv");
    const std::vector<std::uint32_t> fs_mesh = read_spv(
        std::filesystem::path{GW_MOD_BUILD} / "shaders" / "editor" / "mesh_scene_unlit.frag.spv");
    t->vert_mesh_ = mod_from_spv(d, vs_mesh);
    t->frag_mesh_ = mod_from_spv(d, fs_mesh);
    if (t->vert_mesh_ && t->frag_mesh_) {
        const VkPushConstantRange pcr_mesh{
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
            static_cast<std::uint32_t>(sizeof(MeshDrawPc))};
        VkPipelineLayoutCreateInfo pl_mesh{};
        pl_mesh.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pl_mesh.setLayoutCount         = 1;
        pl_mesh.pSetLayouts            = &t->dsl_mesh_;
        pl_mesh.pushConstantRangeCount = 1;
        pl_mesh.pPushConstantRanges    = &pcr_mesh;
        if (vkCreatePipelineLayout(d, &pl_mesh, nullptr, &t->pll_mesh_) == VK_SUCCESS) {
            VkVertexInputBindingDescription vb_mesh[2]{};
            vb_mesh[0].binding   = 0;
            vb_mesh[0].stride    = 8;
            vb_mesh[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            vb_mesh[1].binding   = 1;
            vb_mesh[1].stride    = 12;
            vb_mesh[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            VkVertexInputAttributeDescription at_mesh[3]{};
            at_mesh[0].location = 0;
            at_mesh[0].binding  = 0;
            at_mesh[0].format   = VK_FORMAT_R16G16B16A16_SINT;
            at_mesh[0].offset   = 0;
            at_mesh[1].location = 1;
            at_mesh[1].binding  = 1;
            at_mesh[1].format   = VK_FORMAT_R32G32_UINT;
            at_mesh[1].offset   = 0;
            at_mesh[2].location = 2;
            at_mesh[2].binding  = 1;
            at_mesh[2].format   = VK_FORMAT_R32_UINT;
            at_mesh[2].offset   = 8;
            VkPipelineVertexInputStateCreateInfo vii_mesh{};
            vii_mesh.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vii_mesh.vertexBindingDescriptionCount   = 2;
            vii_mesh.pVertexBindingDescriptions      = vb_mesh;
            vii_mesh.vertexAttributeDescriptionCount = 3;
            vii_mesh.pVertexAttributeDescriptions    = at_mesh;

            const VkShaderModule mesh_mods[2] = {t->vert_mesh_, t->frag_mesh_};
            VkPipelineShaderStageCreateInfo sh_mesh[2]{};
            for (int s = 0; s < 2; ++s) {
                sh_mesh[s].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                sh_mesh[s].stage  = s == 0 ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
                sh_mesh[s].module = mesh_mods[s];
                sh_mesh[s].pName  = "main";
            }
            VkGraphicsPipelineCreateInfo gpi_mesh{};
            gpi_mesh.sType                 = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            gpi_mesh.pNext                 = &pri;
            gpi_mesh.stageCount            = 2;
            gpi_mesh.pStages               = sh_mesh;
            gpi_mesh.pVertexInputState     = &vii_mesh;
            gpi_mesh.pInputAssemblyState   = &ia;
            gpi_mesh.pViewportState        = &vpst;
            gpi_mesh.pRasterizationState   = &rast;
            gpi_mesh.pMultisampleState     = &ms;
            gpi_mesh.pDepthStencilState    = &ds;
            gpi_mesh.pColorBlendState      = &cbs;
            gpi_mesh.pDynamicState         = &dyn_st;
            gpi_mesh.layout                = t->pll_mesh_;
            gpi_mesh.subpass               = 0;
            if (vkCreateGraphicsPipelines(d, VK_NULL_HANDLE, 1, &gpi_mesh, nullptr, &t->p_mesh_) !=
                VK_SUCCESS) {
                vkDestroyPipelineLayout(d, t->pll_mesh_, nullptr);
                t->pll_mesh_ = VK_NULL_HANDLE;
                vkDestroyShaderModule(d, t->vert_mesh_, nullptr);
                t->vert_mesh_ = VK_NULL_HANDLE;
                vkDestroyShaderModule(d, t->frag_mesh_, nullptr);
                t->frag_mesh_ = VK_NULL_HANDLE;
            }
        } else {
            vkDestroyShaderModule(d, t->vert_mesh_, nullptr);
            t->vert_mesh_ = VK_NULL_HANDLE;
            vkDestroyShaderModule(d, t->frag_mesh_, nullptr);
            t->frag_mesh_ = VK_NULL_HANDLE;
        }
    }
    return true;
}

void EditorScenePass::record(const RecordFrame& f) noexcept {
    (void)f.device;
    (void)f.allocator;
    if (f.world == nullptr || !impl_ || !impl_->p_ || !impl_->vb_
        || f.max_frames_in_flight < 1u || f.extent_w == 0u || f.extent_h == 0u) {
        return;
    }
    gw::ecs::World& world = *f.world;
    const int       fi    = static_cast<int>(std::min(
        std::min(f.frame_index, static_cast<std::uint32_t>(kFif - 1U)),
        f.max_frames_in_flight - 1U));
    auto* t = impl_.get();
    if (t->map_[fi] == nullptr) {
        return;
    }
    {
        auto* u  = static_cast<FrameUbo*>(t->map_[fi]);
        u->view = f.view;
        u->proj = f.proj;
    }

    const VkCommandBuffer cmd = f.cmd;
    const VkRect2D        sc{{0, 0}, {f.extent_w, f.extent_h}};
    const VkViewport      vp{0.f,
                        0.f,
                        static_cast<float>(f.extent_w),
                        static_cast<float>(f.extent_h),
                        0.f,
                        1.f};

    vkCmdSetViewport(cmd, 0, 1, &vp);
    vkCmdSetScissor(cmd, 0, 1, &sc);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, t->p_);
    const VkBuffer   vb  = t->vb_;
    const VkDeviceSize off = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &vb, &off);
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, t->pll_, 0, 1, &t->dset_[static_cast<std::size_t>(fi)], 0,
        nullptr);

    const VkShaderStageFlags pflags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    const std::uint32_t  psz = static_cast<std::uint32_t>(sizeof(PushPc));

    std::size_t drawn  = 0;
    const std::size_t kMaxDraws = 512u;

    using WMC = gw::editor::scene::WorldMatrixComponent;
    using BPC = gw::editor::scene::BlockoutPrimitiveComponent;
    using VSC = gw::editor::scene::VisibilityComponent;

    world.for_each<WMC, BPC>([&](const gw::ecs::Entity e, const WMC& wmc, const BPC& /*b*/) {
        if (drawn >= kMaxDraws) {
            return;
        }
        if (const VSC* vis = world.get_component<VSC>(e)) {
            if (!vis->visible) {
                return;
            }
        }
        PushPc            pc{};
        // glm dmat4 → mat4: cast column-wise
        pc.model = glm::mat4(wmc.world);
        pc.color  = kBlockoutColor;
        vkCmdPushConstants(cmd, t->pll_, pflags, 0, psz, &pc);
        vkCmdDraw(cmd, 36, 1, 0, 0);
        ++drawn;
    });

    if (drawn == 0) {
        PushPc pc{};
        pc.model  = glm::mat4(1.f);
        pc.color = kPlaceholderColor;
        vkCmdPushConstants(cmd, t->pll_, pflags, 0, psz, &pc);
        vkCmdDraw(cmd, 36, 1, 0, 0);
    }

    if (f.asset_db && t->p_mesh_ && t->pll_mesh_ && t->skin_dummy_vb_) {
        using WMC = gw::editor::scene::WorldMatrixComponent;
        using EMC = gw::editor::scene::EditorCookedMeshComponent;
        using VSC = gw::editor::scene::VisibilityComponent;

        constexpr VkShaderStageFlags mflags =
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        const std::uint32_t msz = static_cast<std::uint32_t>(sizeof(MeshDrawPc));

        std::uint32_t palette_cursor = 0;

        world.for_each<WMC, EMC>([&](const gw::ecs::Entity e, const WMC& wmc, EMC& emc) {
            if (const VSC* vis = world.get_component<VSC>(e)) {
                if (!vis->visible) {
                    return;
                }
            }
            if (emc.cooked_vfs_path[0] == '\0') {
                return;
            }
            if (emc.mesh_handle_bits == 0) {
                const gw::assets::AssetPath ap(emc.cooked_vfs_path.data());
                auto                         loaded = f.asset_db->load_sync<gw::assets::MeshAsset>(ap);
                if (!loaded.has_value()) {
                    return;
                }
                emc.mesh_handle_bits = loaded.value().bits;
            }
            const auto* mesh = f.asset_db->get(
                gw::assets::MeshHandle{gw::assets::AssetHandle{emc.mesh_handle_bits}});
            if (!mesh || !mesh->ready() || mesh->index() == VK_NULL_HANDLE ||
                mesh->stream0() == VK_NULL_HANDLE || mesh->submeshes().empty()) {
                return;
            }
            const auto& sub = mesh->submeshes()[0];
            const auto& lod = sub.lods[0];
            if (lod.index_count == 0) {
                return;
            }

            const std::uint32_t jc = mesh->joint_count();
            const std::size_t     ib_bytes_expected =
                static_cast<std::size_t>(jc) * gw::assets::kInverseBindMat4Bytes;
            const bool            use_skin =
                mesh->skinned() && mesh->stream2() != VK_NULL_HANDLE && jc > 0u &&
                mesh->inverse_bind_matrices_bytes().size() == ib_bytes_expected;
            if (use_skin && (palette_cursor + jc > kEditorSkinPaletteMaxMatrices)) {
                return;
            }

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, t->p_mesh_);
            const VkBuffer     vb0 = mesh->stream0();
            const VkDeviceSize voff0 =
                static_cast<VkDeviceSize>(sub.vertex_start) * 8u;
            VkBuffer           vb1 = t->skin_dummy_vb_;
            VkDeviceSize       voff1 = 0;
            if (use_skin) {
                vb1 = mesh->stream2();
                voff1 = static_cast<VkDeviceSize>(mesh->stream2_skin_packed_vertex_byte_offset()) +
                        static_cast<VkDeviceSize>(sub.vertex_start) * 12u;
            }
            const VkBuffer     vbs[2]   = {vb0, vb1};
            const VkDeviceSize offs[2] = {voff0, voff1};
            vkCmdBindVertexBuffers(cmd, 0, 2, vbs, offs);
            vkCmdBindIndexBuffer(cmd, mesh->index(), lod.index_offset,
                static_cast<VkIndexType>(std::to_underlying(mesh->index_type())));
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, t->pll_mesh_, 0, 1,
                &t->dset_mesh_[static_cast<std::size_t>(fi)], 0, nullptr);

            MeshDrawPc mpc{};
            mpc.model = glm::mat4(wmc.world);
            mpc.color = glm::vec4(0.15f, 0.78f, 0.48f, 1.f);
            const auto& hdr = mesh->header();
            mpc.pos_scale =
                glm::vec3(hdr.pos_scale[0], hdr.pos_scale[1], hdr.pos_scale[2]);
            mpc.pos_bias = glm::vec3(hdr.pos_bias[0], hdr.pos_bias[1], hdr.pos_bias[2]);
            const std::uint32_t ib_stride =
                (mesh->index_type() == gw::assets::MeshIndexElementType::Uint16) ? 2u : 4u;
            mpc.first_index = lod.index_offset / ib_stride;
            mpc.index_count = lod.index_count;
            if (use_skin) {
                mpc.skin_flags   = 1u;
                mpc.joint_count  = jc;
                mpc.palette_base = palette_cursor;
                auto* const palette_base_ptr =
                    static_cast<glm::mat4*>(t->skin_palette_map_[fi]) + static_cast<std::ptrdiff_t>(palette_cursor);
                const std::span<glm::mat4> palette_span(palette_base_ptr, jc);
                const gw::anim::InstanceHandle anim_h{emc.anim_instance_id};
                bool ok_palette = false;
                if (f.anim_world != nullptr && anim_h.valid()) {
                    thread_local std::vector<glm::mat4> ib_scratch;
                    if (ib_scratch.size() < static_cast<std::size_t>(jc)) {
                        ib_scratch.resize(static_cast<std::size_t>(jc));
                    }
                    std::memcpy(ib_scratch.data(), mesh->inverse_bind_matrices_bytes().data(),
                        ib_bytes_expected);
                    ok_palette = f.anim_world->build_skin_matrix_palette(
                        anim_h,
                        std::span<const glm::mat4>(ib_scratch.data(), static_cast<std::size_t>(jc)),
                        palette_span);
                }
                if (!ok_palette) {
                    for (std::uint32_t j = 0; j < jc; ++j) {
                        palette_base_ptr[j] = glm::mat4(1.f);
                    }
                }
                palette_cursor += jc;
            } else {
                mpc.skin_flags   = 0u;
                mpc.joint_count  = 0u;
                mpc.palette_base = 0u;
            }
            vkCmdPushConstants(cmd, t->pll_mesh_, mflags, 0, msz, &mpc);
            vkCmdDrawIndexed(cmd, mpc.index_count, 1, mpc.first_index,
                static_cast<std::int32_t>(sub.vertex_start), 0);
        });
    }
}

} // namespace gw::editor::render
