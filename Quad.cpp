#include "Quad.h"

GLuint create_quad_vao()
{
   GLuint vao, vbo;
   //quad is in z=0 plane, and goes from -1.0 to +1.0 in x,y directions.
   const float quad_verts[] = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f };

   //generate vao id to hold the mapping from attrib variables in shader to memory locations in vbo
   glGenVertexArrays(1, &vao);
   //binding vao means that bindbuffer, enablevertexattribarray and vertexattribpointer 
   // state will be remembered by vao
   glBindVertexArray(vao);

   glGenBuffers(1, &vbo); // Generate vbo to hold vertex attributes for triangle
   glBindBuffer(GL_ARRAY_BUFFER, vbo); //specify the buffer where vertex attribute data is stored
   //upload from main memory to gpu memory
   glBufferData(GL_ARRAY_BUFFER, sizeof(quad_verts), &quad_verts[0], GL_STATIC_DRAW);

   //get a reference to an attrib variable name in a shader
   const GLint pos_loc = 0;
   glEnableVertexAttribArray(pos_loc); //enable this attribute

   //tell opengl how to get the attribute values out of the vbo (stride and offset)
   glVertexAttribPointer(pos_loc, 3, GL_FLOAT, false, 0, 0);
   glBindVertexArray(0); //unbind the vao

   return vao;
}

void draw_quad_vao(GLuint vao)
{
   glBindVertexArray(vao);
   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}