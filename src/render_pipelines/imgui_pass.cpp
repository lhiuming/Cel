#include "imgui_pass.h"

#include "editor/imgui_global.h"

namespace rei {

namespace imgui {

// clang-format off
static const char* vertex_shader_text = " \
cbuffer vertexBuffer : register(b0) \
{\
  float4x4 ProjectionMatrix; \
};\
struct VS_INPUT\
{\
  float2 pos : POSITION;\
  float4 col : COLOR0;\
  float2 uv  : TEXCOORD0;\
};\
\
struct PS_INPUT\
{\
  float4 pos : SV_POSITION;\
  float4 col : COLOR0;\
  float2 uv  : TEXCOORD0;\
};\
\
PS_INPUT VS(VS_INPUT input)\
{\
  PS_INPUT output;\
  output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\
  output.col = input.col;\
  output.uv  = input.uv;\
  return output;\
}";

constexpr static const char* pixel_shader_text = " \
struct PS_INPUT\
{\
  float4 pos : SV_POSITION;\
  float4 col : COLOR0;\
  float2 uv  : TEXCOORD0;\
};\
SamplerState sampler0 : register(s0);\
Texture2D texture0 : register(t0);\
\
float4 PS(PS_INPUT input) : SV_Target\
{\
  float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \
  return out_col; \
}";
// clang-format on

struct ImGuiShaderDesc : RasterizationShaderMetaInfo {
  ImGuiShaderDesc() {
    VertexInputDesc pos
      = {"POSITION", 0, ResourceFormat::R32G32Float, ImGUI::RenderData::pos_offset()};
    VertexInputDesc uv
      = {"TEXCOORD", 0, ResourceFormat::R32G32Float, ImGUI::RenderData::uv_offset()};
    VertexInputDesc color
      = {"COLOR", 0, ResourceFormat::R8G8B8A8Unorm, ImGUI::RenderData::color_offset()};
    this->vertex_input_desc = {pos, uv, color};
    ShaderParameter space0 {};
    space0.const_buffers = {ConstantBuffer()};
    space0.shader_resources = {ShaderResource()};
    space0.static_samplers = {StaticSampler()};
    this->signature.param_table = {space0};
    this->is_depth_stencil_disabled = true;
    this->front_clockwise = true; // imgui winding
    this->merge.init_as_alpha_blending();
  }
};

} // namespace imgui

ImGuiPass::ImGuiPass(std::weak_ptr<Renderer> renderer) : m_renderer(renderer) {
  REI_ASSERT(!m_renderer.expired());

  // Init ImGUI //
  std::shared_ptr<Renderer> r = m_renderer.lock();

  // Create ui shader
  m_imgui_shader = r->create_shader(
    imgui::vertex_shader_text, imgui::pixel_shader_text, imgui::ImGuiShaderDesc());
  // Create default font texture
  {
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    TextureDesc desc
      = TextureDesc::simple_2d(size_t(width), size_t(height), ResourceFormat::R8G8B8A8Unorm);
    m_imgui_fonts_texture
      = r->create_texture(desc, ResourceState::CopyDestination, L"ImGUI default fonts");
    REI_ASSERT(m_imgui_fonts_texture);
    r->upload_texture(m_imgui_fonts_texture, pixels);
    r->transition(m_imgui_fonts_texture, ResourceState::PixelShaderResource);
    io.Fonts->TexID = 0;
  }
  // const buffer
  {
    ConstBufferLayout lo {};
    lo[0] = ShaderDataType::Float4x4;
    m_imgui_constbuffer = r->create_const_buffer(lo, 1, L"ImGui Const Buffer");
  }
  // shader argument
  {
    ShaderArgumentValue val {};
    val.const_buffers = {m_imgui_constbuffer};
    val.const_buffer_offsets = {0};
    val.shader_resources = {m_imgui_fonts_texture};
    m_fonts_arg = r->create_shader_argument(val);
  }
}

bool debug_showDemo = true;

void ImGuiPass::run(const Parameters& params) {
  auto r = m_renderer.lock();

  // TODO retrive new render data and draw them
  //   -- need to check and expand index-buffer and vertex buffer
  //   -- then update the data to buffer
  //   -- update const buffer (not really need to update; just set it up in creation?)
  //   -- then bind all kinds of states and draw
  //   -- finnaly, convert the imgui commands to render commands
  // ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_pd3dCommandList);
  const ImGUI::RenderData data = g_ImGUI.prepare_render_data(debug_showDemo);

  // update buffer size
  {
    const LowLevelGeometryData total_index = LowLevelGeometryData::size_only(
      data.total_index_count(), ImGUI::RenderData::index_bytesize());
    const LowLevelGeometryData total_vertex = LowLevelGeometryData::size_only(
      data.total_vertex_count(), ImGUI::RenderData::vertec_bytesize());
    if (m_imgui_vertex_buffer == c_empty_handle) {
      // create buffers
      LowLevelGeometryDesc desc {total_index, total_vertex};
      desc.flags.dynamic = true;
      desc.flags.include_blas = false;
      auto buffers = r->create_geometry(desc);
      m_imgui_index_buffer = buffers.index_buffer;
      m_imgui_vertex_buffer = buffers.vertex_buffer;
    } else {
      // update size
      r->update_geometry(m_imgui_index_buffer, total_index);
      r->update_geometry(m_imgui_vertex_buffer, total_vertex);
    }
  }

  // update buffer content
  {
    // const buffer
    {
      Rectf vp = data.viewport();
      float L = vp.x;
      float R = vp.x + vp.width;
      float T = vp.y;
      float B = vp.y + vp.height;
      Mat4 mvp = {
        {2.0f / (R - L), 0.0f, 0.0f, 0.0f},
        {0.0f, 2.0f / (T - B), 0.0f, 0.0f},
        {0.0f, 0.0f, 0.5f, 0.0f},
        {(R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f},
      };
      r->update_const_buffer(m_imgui_constbuffer, 0, 0, mvp);
    }

    // upload geometry buffer data
    LowLevelGeometryData index {};
    index.element_bytesize = ImGUI::RenderData::index_bytesize();
    LowLevelGeometryData vertex {};
    vertex.element_bytesize = ImGUI::RenderData::vertec_bytesize();
    size_t index_count = 0, vertex_count = 0;
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
      const ImDrawList* cmd_list = draw_data->CmdLists[n];
      index.addr = cmd_list->IdxBuffer.Data;
      index.element_count = cmd_list->IdxBuffer.Size;
      r->update_geometry(m_imgui_index_buffer, index, index_count);
      vertex.addr = cmd_list->VtxBuffer.Data;
      vertex.element_count = cmd_list->VtxBuffer.Size;
      r->update_geometry(m_imgui_vertex_buffer, vertex, vertex_count);
      index_count += cmd_list->IdxBuffer.Size;
      vertex_count += cmd_list->VtxBuffer.Size;
    }
  }
  // draw
  auto cmd_list = r->prepare();
  cmd_list->transition(params.render_target, ResourceState::RenderTarget);
  {
    RenderPassCommand raster {};
    raster.render_targets = {params.render_target};
    raster.depth_stencil = c_empty_handle;
    raster.clear_rt = false;
    raster.clear_ds = false;
    raster.viewport = RenderViewaport::full(draw_data->DisplaySize.x, draw_data->DisplaySize.y);
    raster.area = {};
    cmd_list->begin_render_pass(raster);

    size_t global_vtx_offset = 0;
    size_t global_idx_offset = 0;
    ImVec2 clip_off = draw_data->DisplayPos;
    DrawCommand draw {};
    draw.index_buffer = m_imgui_index_buffer;
    draw.vertex_buffer = m_imgui_vertex_buffer;
    draw.shader = m_imgui_shader;
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
      const ImDrawList* im_cmd_list = draw_data->CmdLists[n];
      for (int cmd_i = 0; cmd_i < im_cmd_list->CmdBuffer.Size; cmd_i++) {
        const ImDrawCmd* pcmd = &im_cmd_list->CmdBuffer[cmd_i];
        if (pcmd->UserCallback != NULL) {
          // User callback, registered via ImDrawList::AddCallback()
          // (ImDrawCallback_ResetRenderState is a special callback value used by the user to
          // request the renderer to reset render state.)
          if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
            REI_WARNING("Ignored callback");
          else
            pcmd->UserCallback(im_cmd_list, pcmd);
        } else {
          // Apply Scissor, Bind texture, Draw
          draw.override_area
            = {int(pcmd->ClipRect.x - clip_off.x), int(pcmd->ClipRect.y - clip_off.y),
              int(pcmd->ClipRect.z - pcmd->ClipRect.x), int(pcmd->ClipRect.w - pcmd->ClipRect.y)};
          draw.index_count = pcmd->ElemCount;
          draw.index_offset = pcmd->IdxOffset + global_idx_offset;
          draw.vertex_offset = pcmd->VtxOffset + global_vtx_offset;
          if (pcmd->TextureId == 0) {
            draw.arguments = {m_fonts_arg};
          } else {
            REI_WARNING("Unknow texture");
          }
          // ctx->RSSetScissorRects(1, &r);
          cmd_list->draw(draw);
        }
      }
      global_idx_offset += im_cmd_list->IdxBuffer.Size;
      global_vtx_offset += im_cmd_list->VtxBuffer.Size;
    }
    cmd_list->end_render_pass();
  } // end draw
}

} // namespace rei
