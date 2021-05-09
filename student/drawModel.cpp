/*!
 * @file
 * @brief This file contains functions for model rendering
 *
 * @author Martin Zmitko, xzmitk01@stud.fit.vutbr.cz
 */
#include <student/drawModel.hpp>
#include <student/gpu.hpp>
#include <stdio.h>
#include <iostream>
#include "../tests/testCommon.hpp"

void drawNode(GPUContext &ctx, Node const &node, Model const &model, glm::mat4 mat){
  if(node.mesh >= 0){ // ma tento node mesh?
    Mesh const &mesh = model.meshes[node.mesh];
    ctx.vao.indexType = mesh.indexType;
    ctx.vao.indexBuffer = mesh.indices;
    ctx.prg.uniforms.uniform[1].m4 = mat;
    ctx.prg.uniforms.uniform[2].m4 = glm::transpose(glm::inverse(mat));
    ctx.prg.uniforms.uniform[5].v4 = mesh.diffuseColor;
    ctx.vao.vertexAttrib[0] = mesh.position;
    ctx.vao.vertexAttrib[1] = mesh.normal;
    ctx.vao.vertexAttrib[2] = mesh.texCoord;
    if(mesh.diffuseTexture >= 0){
      ctx.prg.uniforms.textures[0] = model.textures[mesh.diffuseTexture];
      ctx.prg.uniforms.uniform[6].v1 = 1.f;
    } else {
      ctx.prg.uniforms.uniform[6].v1 = 0.f;
      ctx.prg.uniforms.textures[0] = Texture{};
    }

    drawTriangles(ctx, mesh.nofIndices);
  }

  for(Node n : node.children){
    drawNode(ctx, n, model, mat * n.modelMatrix);
  }
}

/**
 * @brief This function renders a model
 *
 * @param ctx GPUContext
 * @param model model structure
 * @param proj projection matrix
 * @param view view matrix
 * @param light light position
 * @param camera camera position (unused)
 */
//! [drawModel]
void drawModel(GPUContext &ctx, Model const &model, glm::mat4 const &proj, glm::mat4 const &view, glm::vec3 const& light, glm::vec3 const &camera){
  (void)camera;
  ctx.prg.fragmentShader = drawModel_fragmentShader;
  ctx.prg.vertexShader = drawModel_vertexShader;
  ctx.prg.vs2fs[0] = AttributeType::VEC3;
  ctx.prg.vs2fs[1] = AttributeType::VEC3;
  ctx.prg.vs2fs[2] = AttributeType::VEC2;
  ctx.prg.uniforms.uniform[0].m4 = proj * view;
  ctx.prg.uniforms.uniform[3].v3 = light;

  for(uint32_t i = 0; i < model.roots.size(); i++)
    drawNode(ctx, model.roots[i], model, model.roots[i].modelMatrix);
}
//! [drawModel]

/**
 * @brief This function represents vertex shader of texture rendering method.
 *
 * @param outVertex output vertex
 * @param inVertex input vertex
 * @param uniforms uniform variables
 */
//! [drawModel_vs]
void drawModel_vertexShader(OutVertex&outVertex,InVertex const&inVertex,Uniforms const&uniforms){
  outVertex.attributes[0].v3 = uniforms.uniform[1].m4 * glm::vec4(inVertex.attributes[0].v3, 1.f);
  outVertex.attributes[1].v3 = uniforms.uniform[2].m4 * glm::vec4(inVertex.attributes[1].v3, 0.f);
  outVertex.attributes[2].v2 = inVertex.attributes[2].v2;
  outVertex.gl_Position = uniforms.uniform[0].m4 * uniforms.uniform[1].m4 * glm::vec4(inVertex.attributes[0].v3, 1.f);
}
//! [drawModel_vs]

/**
 * @brief This functionrepresents fragment shader of texture rendering method.
 *
 * @param outFragment output fragment
 * @param inFragment input fragment
 * @param uniforms uniform variables
 */
//! [drawModel_fs]
void drawModel_fragmentShader(OutFragment&outFragment,InFragment const&inFragment,Uniforms const&uniforms){
  glm::vec4 tex = uniforms.uniform[6].v1 > 0.f ? read_texture(uniforms.textures[0], inFragment.attributes[2].v2) : uniforms.uniform[5].v4;
  glm::vec3 aL = tex * 0.2f;
  glm::vec3 dL, L = uniforms.uniform[3].v3, N = inFragment.attributes[1].v3;
  dL = tex * glm::clamp(glm::dot(L - inFragment.attributes[0].v3, glm::normalize(N)), 0.f, 1.f);
  outFragment.gl_FragColor = glm::vec4(aL + dL, tex.a);
}
//! [drawModel_fs]

