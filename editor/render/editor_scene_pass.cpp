// editor/render/editor_scene_pass.cpp — Wave 1C scene raster (unlit, depth).
//
// vk_mem_alloc includes vulkan.h; volk must load with prototypes disabled
// (matches editor_app.cpp and engine_render).
#define VK_NO_PROTOTYPES
#include <volk.h>
#include <vk_mem_alloc.h>

#include "editor/render/editor_scene_pass.hpp"

#include "editor/scene/components.hpp"
#include "engine/ecs/entity.hpp"
#include "engine/ecs/world.hpp"

#define GLM_FORCE_RADIANS
#include <glm/mat4x4.hpp>

#include <array>
#include <algorithm>
#include <cstdio>
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
    VkDescriptorSetLayout dsl_  = VK_NULL_HANDLE;
    VkPipelineLayout  pll_  = VK_NULL_HANDLE;
    VkPipeline        p_    = VK_NULL_HANDLE;
    VkDescriptorPool  pool_  = VK_NULL_HANDLE;

    VkBuffer          vb_     = VK_NULL_HANDLE;
    VmaAllocation     vba_    = nullptr;
    VkDeviceSize     vb_nbytes_ = 0;

    std::array<VkBuffer,     kFif> ubo_{};
    std::array<VmaAllocation, kFif> ubao_{};
    std::array<void*, kFif>       map_{};

    std::array<VkDescriptorSet, kFif> dset_{};

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
        if (t->pool_) vkDestroyDescriptorPool(t->device_, t->pool_, nullptr);
        t->pool_ = VK_NULL_HANDLE;
        if (t->p_)       vkDestroyPipeline(t->device_, t->p_, nullptr);
        if (t->pll_)     vkDestroyPipelineLayout(t->device_, t->pll_, nullptr);
        if (t->dsl_)     vkDestroyDescriptorSetLayout(t->device_, t->dsl_, nullptr);
        if (t->vert_)    vkDestroyShaderModule(t->device_, t->vert_, nullptr);
        if (t->frag_)    vkDestroyShaderModule(t->device_, t->frag_, nullptr);
        t->p_     = VK_NULL_HANDLE;
        t->pll_   = VK_NULL_HANDLE;
        t->dsl_   = VK_NULL_HANDLE;
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

    VkDescriptorPoolSize ps{};
    ps.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ps.descriptorCount = kFif;
    VkDescriptorPoolCreateInfo dp{};
    dp.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dp.maxSets      = kFif;
    dp.poolSizeCount = 1;
    dp.pPoolSizes   = &ps;
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
    return true;
}

void EditorScenePass::record(
    VkCommandBuffer  cmd, VkDevice device, VmaAllocator alloc, std::uint32_t extent_w,
    std::uint32_t   extent_h, const glm::mat4& view, const glm::mat4& proj,
    gw::ecs::World& world, std::uint32_t frame_index, std::uint32_t max_frames) noexcept {
    (void)device;
    (void)alloc;
    if (!impl_ || !impl_->p_ || !impl_->vb_ || max_frames < 1u || extent_w == 0u
        || extent_h == 0u) {
        return;
    }
    const int fi = static_cast<int>(std::min(
        std::min(frame_index, static_cast<std::uint32_t>(kFif - 1U)), max_frames - 1U));
    auto* t   = impl_.get();
    if (t->map_[fi] == nullptr) {
        return;
    }
    {
        auto* u  = static_cast<FrameUbo*>(t->map_[fi]);
        u->view = view;
        u->proj = proj;
    }

    const VkRect2D sc{{0, 0}, {extent_w, extent_h}};
    const VkViewport vp{
        0.f, 0.f, static_cast<float>(extent_w), static_cast<float>(extent_h), 0.f, 1.f};

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
}

} // namespace gw::editor::render
