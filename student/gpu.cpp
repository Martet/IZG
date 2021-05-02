/*!
 * @file
 * @brief This file contains implementation of gpu
 *
 * @author Tomáš Milet, imilet@fit.vutbr.cz
 */

#include <student/gpu.hpp>
#include <iostream>
#include <stdio.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

struct Triangle{
  OutVertex points[3];
};

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

void loadTriangle(Triangle &triangle, GPUContext &ctx, uint32_t tId){
  for(uint8_t i = 0; i < 3; i++){ // smyčka přes vrcholy trojúhelníku
    InVertex inVertex;
    inVertex.gl_VertexID = computeVertexID(ctx.vao, tId + i);
    vertexPuller(ctx.vao, inVertex);
    ctx.prg.vertexShader(triangle.points[i], inVertex, ctx.prg.uniforms);
  }
}

void perspectiveDivision(Triangle &triangle){
  for(uint8_t i = 0; i < 3; i++){
    triangle.points[i].gl_Position.x /= triangle.points[i].gl_Position.w;
    triangle.points[i].gl_Position.y /= triangle.points[i].gl_Position.w;
    triangle.points[i].gl_Position.z /= triangle.points[i].gl_Position.w;
  }
}

void viewportTransformation(Triangle &triangle, uint32_t w, uint32_t h){
  for(uint8_t i = 0; i < 3; i++){
    triangle.points[i].gl_Position.x = (triangle.points[i].gl_Position.x * 0.5 + 0.5) * w;
    triangle.points[i].gl_Position.y = (triangle.points[i].gl_Position.y * 0.5 + 0.5) * h;
  }
}

void rasterize(GPUContext &ctx, Triangle const &triangle){
  int maxX = MAX(triangle.points[0].gl_Position.x, MAX(triangle.points[1].gl_Position.x, triangle.points[2].gl_Position.x));
  int maxY = MAX(triangle.points[0].gl_Position.y, MAX(triangle.points[1].gl_Position.y, triangle.points[2].gl_Position.y));
  int minX = MIN(triangle.points[0].gl_Position.x, MIN(triangle.points[1].gl_Position.x, triangle.points[2].gl_Position.x));
  int minY = MIN(triangle.points[0].gl_Position.y, MIN(triangle.points[1].gl_Position.y, triangle.points[2].gl_Position.y));

  maxX = MIN(maxX, ctx.frame.width);
  maxY = MIN(maxY, ctx.frame.height);
  minX = MAX(minX, 0);
  minY = MAX(minY, 0);

  int deltaX1 = triangle.points[1].gl_Position.x - triangle.points[0].gl_Position.x;
  int deltaY1 = triangle.points[1].gl_Position.y - triangle.points[0].gl_Position.y;
  int deltaX2 = triangle.points[2].gl_Position.x - triangle.points[1].gl_Position.x;
  int deltaY2 = triangle.points[2].gl_Position.y - triangle.points[1].gl_Position.y;
  int deltaX3 = triangle.points[0].gl_Position.x - triangle.points[2].gl_Position.x;
  int deltaY3 = triangle.points[0].gl_Position.y - triangle.points[2].gl_Position.y;

  int E1 = (minY - triangle.points[0].gl_Position.y) * deltaX1 - (minX - triangle.points[0].gl_Position.x) * deltaY1;
  int E2 = (minY - triangle.points[1].gl_Position.y) * deltaX2 - (minX - triangle.points[1].gl_Position.x) * deltaY2;
  int E3 = (minY - triangle.points[2].gl_Position.y) * deltaX3 - (minX - triangle.points[2].gl_Position.x) * deltaY3;

  for(int y = minY; y < maxY; y++){
    int lastE1 = E1;
    int lastE2 = E2;
    int lastE3 = E3;
    for(int x = minX; x < maxX; x++){
      if(E1 > 0 && E2 > 0 && E3 > 0){
        InFragment inFragment;
        inFragment.gl_FragCoord.x = x + 0.5;
        inFragment.gl_FragCoord.y = y + 0.5;
        if(x < 5 || x > 95 || y < 5 || y > 95)
        printf("%f: %f %f\n", inFragment.gl_FragCoord.x + inFragment.gl_FragCoord.y, inFragment.gl_FragCoord.x, inFragment.gl_FragCoord.y);
        OutFragment outFragment;
        ctx.prg.fragmentShader(outFragment, inFragment, ctx.prg.uniforms);
      }
      
      E1 -= deltaY1;
      E2 -= deltaY2;
      E3 -= deltaY3;
    }
    E1 = lastE1 + deltaX1;
    E2 = lastE2 + deltaX2;
    E3 = lastE3 + deltaX3;
  }
}

//! [drawTrianglesImpl]
void drawTrianglesImpl(GPUContext &ctx, uint32_t nofVertices){
  for(uint32_t i = 0; i < nofVertices; i += 3){
    Triangle triangle;
    loadTriangle(triangle, ctx, i);
    perspectiveDivision(triangle);
    viewportTransformation(triangle, ctx.frame.width, ctx.frame.height);
    rasterize(ctx, triangle);
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

