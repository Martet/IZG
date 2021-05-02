/*!
 * @file
 * @brief This file contains implementation of gpu
 *
 * @author Tomáš Milet, imilet@fit.vutbr.cz
 */

#include <student/gpu.hpp>
#include <iostream>

uint32_t computeVertexID(VertexArray const&vao,uint32_t shaderInvocation){
  if(!vao.indexBuffer)return shaderInvocation;

  switch(vao.indexType){
    case IndexType::UINT32:
      return ((uint32_t*)vao.indexBuffer)[shaderInvocation];
    case IndexType::UINT16:
      return ((uint16_t*)vao.indexBuffer)[shaderInvocation];
    case IndexType::UINT8:
      return ((uint8_t*)vao.indexBuffer)[shaderInvocation];
  }

  return shaderInvocation;
}

void vertexPuller(VertexArray const &vao, InVertex &vert){
  for(uint32_t i = 0; i < maxAttributes; i++){
    switch(vao.vertexAttrib[i].type){
      case AttributeType::EMPTY:
        break;
      case AttributeType::FLOAT:
        vert.attributes[i].v1 = *(float*)((uint8_t*)vao.vertexAttrib[i].bufferData + vao.vertexAttrib[i].offset + vao.vertexAttrib[i].stride * vert.gl_VertexID);
        break;
      case AttributeType::VEC2:
        vert.attributes[i].v2 = *(glm::vec2*)((uint8_t*)vao.vertexAttrib[i].bufferData + vao.vertexAttrib[i].offset + vao.vertexAttrib[i].stride * vert.gl_VertexID);
        break;
      case AttributeType::VEC3:
        vert.attributes[i].v3 = *(glm::vec3*)((uint8_t*)vao.vertexAttrib[i].bufferData + vao.vertexAttrib[i].offset + vao.vertexAttrib[i].stride * vert.gl_VertexID);
        break;
      case AttributeType::VEC4:
        vert.attributes[i].v4 = *(glm::vec4*)((uint8_t*)vao.vertexAttrib[i].bufferData + vao.vertexAttrib[i].offset + vao.vertexAttrib[i].stride * vert.gl_VertexID);
        break;
    }
  }
}


//! [drawTrianglesImpl]
void drawTrianglesImpl(GPUContext &ctx,uint32_t nofVertices){
  for(int i = 0; i < nofVertices; i++){
    InVertex inVertex;
    OutVertex outVertex;
    inVertex.gl_VertexID = computeVertexID(ctx.vao, i);
    vertexPuller(ctx.vao, inVertex);
    ctx.prg.vertexShader(outVertex, inVertex, ctx.prg.uniforms);
  }
}
//! [drawTrianglesImpl]

/**
 * @brief This function reads color from texture.
 *
 * @param texture texture
 * @param uv uv coordinates
 *
 * @return color 4 floats
 */
glm::vec4 read_texture(Texture const&texture,glm::vec2 uv){
  if(!texture.data)return glm::vec4(0.f);
  auto uv1 = glm::fract(uv);
  auto uv2 = uv1*glm::vec2(texture.width-1,texture.height-1)+0.5f;
  auto pix = glm::uvec2(uv2);
  //auto t   = glm::fract(uv2);
  glm::vec4 color = glm::vec4(0.f,0.f,0.f,1.f);
  for(uint32_t c=0;c<texture.channels;++c)
    color[c] = texture.data[(pix.y*texture.width+pix.x)*texture.channels+c]/255.f;
  return color;
}

/**
 * @brief This function clears framebuffer.
 *
 * @param ctx GPUContext
 * @param r red channel
 * @param g green channel
 * @param b blue channel
 * @param a alpha channel
 */
void clear(GPUContext&ctx,float r,float g,float b,float a){
  auto&frame = ctx.frame;
  auto const nofPixels = frame.width * frame.height;
  for(size_t i=0;i<nofPixels;++i){
    frame.depth[i] = 10e10f;
    frame.color[i*4+0] = static_cast<uint8_t>(glm::min(r*255.f,255.f));
    frame.color[i*4+1] = static_cast<uint8_t>(glm::min(g*255.f,255.f));
    frame.color[i*4+2] = static_cast<uint8_t>(glm::min(b*255.f,255.f));
    frame.color[i*4+3] = static_cast<uint8_t>(glm::min(a*255.f,255.f));
  }
}

