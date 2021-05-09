/*!
 * @file
 * @brief This file contains functions for model rendering
 *
 * @author Martin Zmitko, xzmitk01@stud.fit.vutbr.cz
 */
#include <student/drawModel.hpp>
#include <student/gpu.hpp>
#include <stdio.h>

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
  (void)outVertex;
  (void)inVertex;
  (void)uniforms;
  /// \todo Tato funkce reprezentujte vertex shader.<br>
  /// Vaším úkolem je správně trasnformovat vrcholy modelu.
  /// Bližší informace jsou uvedeny na hlavní stránce dokumentace.
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
  (void)outFragment;
  (void)inFragment;
  (void)uniforms;
  /// \todo Tato funkce reprezentujte fragment shader.<br>
  /// Vaším úkolem je správně obarvit fragmenty a osvětlit je pomocí lambertova osvětlovacího modelu.
  /// Bližší informace jsou uvedeny na hlavní stránce dokumentace.
}
//! [drawModel_fs]

