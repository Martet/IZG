/*!
 * @file
 * @brief This file contains implementation of gpu
 *
 * @author Martin Zmitko, xzmitk01@stud.fit.vutbr.cz
 */

#include <student/gpu.hpp>
#include <iostream>
#include <stdio.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define AREA(v0, v1, v2) ((v0.x) * (v1.y) + (v1.x) * (v2.y) + (v2.x) * (v0.y) - (v1.x) * (v0.y) - (v2.x) * (v1.y) - (v0.x) * (v2.y)) / 2

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

void getBarCords(Triangle &triangle, InFragment &fragment){
  float area = AREA(triangle.points[0].gl_Position, triangle.points[1].gl_Position, triangle.points[2].gl_Position);
  float bar0 = AREA(triangle.points[1].gl_Position, triangle.points[2].gl_Position, fragment.gl_FragCoord) / area;
  float bar1 = AREA(triangle.points[2].gl_Position, triangle.points[0].gl_Position, fragment.gl_FragCoord) / area;
  float bar2 = 1 - bar0 - bar1;
  fragment.gl_FragCoord.z = triangle.points[0].gl_Position.z * bar0 + triangle.points[1].gl_Position.z * bar1 + triangle.points[2].gl_Position.z * bar2;
}

void getPerspectiveBarAttrs(Triangle &triangle, InFragment &fragment, Program &prg){
  float area = AREA(triangle.points[0].gl_Position, triangle.points[1].gl_Position, triangle.points[2].gl_Position);
  float bar0 = AREA(triangle.points[1].gl_Position, triangle.points[2].gl_Position, fragment.gl_FragCoord) / area;
  float bar1 = AREA(triangle.points[2].gl_Position, triangle.points[0].gl_Position, fragment.gl_FragCoord) / area;
  float bar2 = 1 - bar0 - bar1;
  float s = bar0 / triangle.points[0].gl_Position.w + bar1 / triangle.points[1].gl_Position.w + bar2 / triangle.points[2].gl_Position.w;
  bar0 /= triangle.points[0].gl_Position.w * s;
  bar1 /= triangle.points[1].gl_Position.w * s;
  bar2 /= triangle.points[2].gl_Position.w * s;

  for(uint32_t i = 0; i < maxAttributes; i++){
    if(prg.vs2fs[i] != AttributeType::EMPTY){
      fragment.attributes[i].v3.r = triangle.points[0].attributes[i].v3.r * bar0 + triangle.points[1].attributes[i].v3.r * bar1 + triangle.points[2].attributes[i].v3.r * bar2;
      fragment.attributes[i].v3.g = triangle.points[0].attributes[i].v3.g * bar0 + triangle.points[1].attributes[i].v3.g * bar1 + triangle.points[2].attributes[i].v3.g * bar2;
      fragment.attributes[i].v3.b = triangle.points[0].attributes[i].v3.b * bar0 + triangle.points[1].attributes[i].v3.b * bar1 + triangle.points[2].attributes[i].v3.b * bar2;
    }
  }
}

void rasterize(GPUContext &ctx, Triangle &triangle){
  int32_t maxX = MAX(triangle.points[0].gl_Position.x, MAX(triangle.points[1].gl_Position.x, triangle.points[2].gl_Position.x));
  int32_t maxY = MAX(triangle.points[0].gl_Position.y, MAX(triangle.points[1].gl_Position.y, triangle.points[2].gl_Position.y));
  int32_t minX = MIN(triangle.points[0].gl_Position.x, MIN(triangle.points[1].gl_Position.x, triangle.points[2].gl_Position.x));
  int32_t minY = MIN(triangle.points[0].gl_Position.y, MIN(triangle.points[1].gl_Position.y, triangle.points[2].gl_Position.y));

  maxX = MIN(maxX, ctx.frame.width);
  maxY = MIN(maxY, ctx.frame.height);
  minX = MAX(minX, 0);
  minY = MAX(minY, 0);

  int32_t deltaX1 = triangle.points[1].gl_Position.x - triangle.points[0].gl_Position.x;
  int32_t deltaY1 = triangle.points[1].gl_Position.y - triangle.points[0].gl_Position.y;
  int32_t deltaX2 = triangle.points[2].gl_Position.x - triangle.points[1].gl_Position.x;
  int32_t deltaY2 = triangle.points[2].gl_Position.y - triangle.points[1].gl_Position.y;
  int32_t deltaX3 = triangle.points[0].gl_Position.x - triangle.points[2].gl_Position.x;
  int32_t deltaY3 = triangle.points[0].gl_Position.y - triangle.points[2].gl_Position.y;

  float E1 = (minY + 0.5 - triangle.points[0].gl_Position.y) * deltaX1 - (minX + 0.5 - triangle.points[0].gl_Position.x) * deltaY1;
  float E2 = (minY + 0.5 - triangle.points[1].gl_Position.y) * deltaX2 - (minX + 0.5 - triangle.points[1].gl_Position.x) * deltaY2;
  float E3 = (minY + 0.5 - triangle.points[2].gl_Position.y) * deltaX3 - (minX + 0.5 - triangle.points[2].gl_Position.x) * deltaY3;

  for(uint32_t y = minY; y < maxY; y++){
    int32_t lastE1 = E1;
    int32_t lastE2 = E2;
    int32_t lastE3 = E3;
    for(uint32_t x = minX; x < maxX; x++){
      if(E1 >= 0 && E2 >= 0 && E3 >= 0){
        InFragment inFragment;
        inFragment.gl_FragCoord.x = x + 0.5;
        inFragment.gl_FragCoord.y = y + 0.5;
        getBarCords(triangle, inFragment);
        getPerspectiveBarAttrs(triangle, inFragment, ctx.prg);
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

