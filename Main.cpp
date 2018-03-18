#include <windows.h>
#include <vector>
#include <GL/glew.h>

#include <GL/freeglut.h>

#include <GL/gl.h>
#include <GL/glext.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#include "InitShader.h"
#include "LoadMesh.h"
#include "LoadTexture.h"
#include "imgui_impl_glut.h"
#include "VideoMux.h"
#include "DebugCallback.h"
#include "Quad.h"
#include "VBO_Surf.h"

//names of the shader files to load
static const std::string vertex_shader("template_vs.glsl");
static const std::string fragment_shader("template_fs.glsl");

const int screenWidth(1280), screenHeight(720);

GLuint shader_program = -1;
GLuint texture_id = -1; //Texture map for fish
GLuint fboTexture = -1;
GLuint pickTexture = -1;

GLuint fbo_T = -1;
GLuint renderBuffer = -1;

GLuint Quad_vao = -1;

//names of the mesh and texture files to load
static const std::string mesh_name = "Amago0.obj";
static const std::string texture_name = "AmagoT.bmp";

MeshData mesh_data;
float time_sec = 0.0f;
float angle = 0.0f;
bool recording = false;
bool isBoundary = false;

GLuint pixel_Index = 99;

const int pass_loc = 1;
const int tex_loc = 2;
const int fishtex_loc = 3;

const int MAXFISH(9);
GLubyte bufferColor[4] = { static_cast <GLubyte>(20)};

std::vector <glm::mat4> offset(MAXFISH);
GLuint transformOffset;



//Draw the user interface using ImGui
void draw_gui()
{
   ImGui_ImplGlut_NewFrame();

   const int filename_len = 256;
   static char video_filename[filename_len] = "capture.mp4";

   ImGui::InputText("Video filename", video_filename, filename_len);
   ImGui::SameLine();
   if (recording == false)
   {
      if (ImGui::Button("Start Recording"))
      {
         const int w = glutGet(GLUT_WINDOW_WIDTH);
         const int h = glutGet(GLUT_WINDOW_HEIGHT);
         recording = true;
         start_encoding(video_filename, w, h); //Uses ffmpeg
      }
      
   }
   else
   {
      if (ImGui::Button("Stop Recording"))
      {
         recording = false;
         finish_encoding(); //Uses ffmpeg
      }
   }

   if (ImGui::Checkbox("Show Boundary", &isBoundary))
   {
	   int boundary_loc = 5;
	   if (isBoundary)
	   {
		   glUniform1i(boundary_loc,1);
	   }
	   else
	   {
		   glUniform1i(boundary_loc, 0);
	   }

   }
   //create a slider to change the angle variables
   ImGui::SliderFloat("View angle", &angle, -3.141592f, +3.141592f);

   ImGui::Image((void*)fboTexture, ImVec2(128.0f, 128.0f), ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
   ImGui::Image((void*)pickTexture, ImVec2(128.0f, 128.0f), ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));

   static bool show = false;
   //ImGui::ShowTestWindow(&show);
   //ImGui::ShowTestWindow();
   ImGui::Render();
 }

// glut display callback function.
// This function gets called every time the scene gets redisplayed 
void display()
{
   //clear the screen
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glUseProgram(shader_program);

   const int w = screenWidth;
   const int h = screenHeight;
   const float aspect_ratio = float(w) / float(h);

   glm::mat4 M = glm::rotate(angle, glm::vec3(0.0f, 1.0f, 0.0f))*glm::scale(glm::vec3(mesh_data.mScaleFactor));
   glm::mat4 V = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
   glm::mat4 P = glm::perspective(3.141592f / 4.0f, aspect_ratio, 0.1f, 100.0f);
   
   int PV_loc = 0;
   glm::mat4 PV = P*V;
   glUniformMatrix4fv(PV_loc, 1, false, glm::value_ptr(PV));
   int M_loc = 6;
   glUniformMatrix4fv(M_loc, 1, false, glm::value_ptr(M));

   pixel_Index = static_cast <GLuint> (bufferColor[0]);
   const int index_loc = 7;
   glUniform1i(index_loc, pixel_Index);

 
   ///////////////////////////////////////////////////
   // Begin pass 1: render scene to texture.
   ///////////////////////////////////////////////////
   
   glUniform1i(pass_loc,1);
   glBindFramebuffer(GL_FRAMEBUFFER,fbo_T);

   GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1 };
   glDrawBuffers(2, drawBuffers);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, texture_id);
   glUniform1i(fishtex_loc, 0);


   glViewport(0, 0, screenWidth, screenHeight);
   glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   

   //DrawSurfaceVao();
   glEnable(GL_CULL_FACE);
   glCullFace(GL_BACK);


   glEnable(GL_BLEND);
   
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glBindVertexArray(mesh_data.mVao);
   glDrawElementsInstanced(GL_TRIANGLES, mesh_data.mNumIndices, GL_UNSIGNED_INT, 0, MAXFISH);
   glDisable(GL_BLEND);
   glDisable(GL_CULL_FACE);
	
   ///////////////////////////////////////////////////
   // Begin pass 2: render textured quad to screen
   ///////////////////////////////////////////////////
   glUniform1i(pass_loc, 2);
   glBindFramebuffer(GL_FRAMEBUFFER,0);

   glDrawBuffer(GL_BACK);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, fboTexture);
   //glBindTexture(GL_TEXTURE_2D, texture_id);
   glUniform1i(tex_loc, 0); 

   //glViewport(0,0, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
   glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
   

   draw_quad_vao(Quad_vao);


   draw_gui();

   if (recording == true)
   {
      glFinish();

      glReadBuffer(GL_BACK);
      read_frame_to_encode(&rgb, &pixels, w, h);
      encode_frame(rgb);
   }

   glutSwapBuffers();
}

// glut idle callback.
//This function gets called between frames
void idle()
{
	glutPostRedisplay();

   const int time_ms = glutGet(GLUT_ELAPSED_TIME);
   time_sec = 0.001f*time_ms;

   GLuint time_loc = 4;
   glUniform1f(time_loc, time_sec);

   

}

void reload_shader()
{
   GLuint new_shader = InitShader(vertex_shader.c_str(), fragment_shader.c_str());

   if(new_shader == -1) // loading failed
   {
      glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
   }
   else
   {
      glClearColor(0.35f, 0.35f, 0.35f, 0.0f);

      if(shader_program != -1)
      {
         glDeleteProgram(shader_program);
      }
      shader_program = new_shader;

      if(mesh_data.mVao != -1)
      {
         BufferIndexedVerts(mesh_data);
      }
   }
}

// Display info about the OpenGL implementation provided by the graphics driver.
void printGlInfo()
{
   std::cout << "Vendor: "       << glGetString(GL_VENDOR)                    << std::endl;
   std::cout << "Renderer: "     << glGetString(GL_RENDERER)                  << std::endl;
   std::cout << "Version: "      << glGetString(GL_VERSION)                   << std::endl;
   std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION)  << std::endl;
}

void initOpenGl()
{
   //Initialize glew so that new OpenGL function names can be used
   glewInit();
   //RegisterCallback();

   glEnable(GL_DEPTH_TEST);

   reload_shader();

   int fishID = 0;
   for (int i = 0; i < MAXFISH/3; i++)
   {
	   for (int j = 0; j< MAXFISH/3; j++)
	   {
		   offset[fishID] = glm::translate(glm::vec3(1.2*i - 1.0f , 1.2*j - 1.0f, 0.0f));
		   fishID++;
	   }
   }

   //Load a mesh and a texture
   mesh_data = LoadMesh(mesh_name); //Helper function: Uses Open Asset Import library.
   texture_id = LoadTexture(texture_name.c_str()); //Helper function: Uses FreeImage library
	
   CreateSurfaceVao();
   Quad_vao = create_quad_vao();

   glGenTextures(1,&fboTexture);
   glBindTexture(GL_TEXTURE_2D,fboTexture);
   glTexImage2D(GL_TEXTURE_2D,0,GL_RGB, screenWidth, screenHeight,0, GL_RGB, GL_UNSIGNED_BYTE,0);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glBindTexture(GL_TEXTURE_2D, 0);

   glGenTextures(1, &pickTexture);
   glBindTexture(GL_TEXTURE_2D,pickTexture);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glBindTexture(GL_TEXTURE_2D, 0);

   glGenRenderbuffers(1, &renderBuffer);
   glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);
   glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screenWidth, screenHeight);

   glGenFramebuffers(1, &fbo_T);
   glBindFramebuffer(GL_FRAMEBUFFER, fbo_T);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTexture, 0);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, pickTexture, 0);
   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderBuffer);

   glBindFramebuffer(GL_FRAMEBUFFER,0);

   glBindVertexArray(mesh_data.mVao);
   glGenBuffers(1, &transformOffset);
   glBindBuffer(GL_ARRAY_BUFFER, transformOffset);
   glBufferData(GL_ARRAY_BUFFER, MAXFISH * sizeof(glm::mat4), &offset[0], GL_STATIC_DRAW);
   GLuint offset_loc = 3;
   for (int i = 0; i<4; i++)
   {
	   glVertexAttribPointer(offset_loc + i, 4, GL_FLOAT, false, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)*i));
	   glEnableVertexAttribArray(offset_loc + i);
	   glVertexAttribDivisor(offset_loc + i, 1);
   }
   glBindVertexArray(0);
}

// glut callbacks need to send keyboard and mouse events to imgui
void keyboard(unsigned char key, int x, int y)
{
   ImGui_ImplGlut_KeyCallback(key);
   std::cout << "key : " << key << ", x: " << x << ", y: " << y << std::endl;

   switch(key)
   {
      case 'r':
      case 'R':
         reload_shader();     
      break;
   }
}

void keyboard_up(unsigned char key, int x, int y)
{
   ImGui_ImplGlut_KeyUpCallback(key);
}

void special_up(int key, int x, int y)
{
   ImGui_ImplGlut_SpecialUpCallback(key);
}

void passive(int x, int y)
{
   ImGui_ImplGlut_PassiveMouseMotionCallback(x,y);
}

void special(int key, int x, int y)
{
   ImGui_ImplGlut_SpecialCallback(key);
}

void motion(int x, int y)
{
   ImGui_ImplGlut_MouseMotionCallback(x, y);
}

void mouse(int button, int state, int x, int y)
{
   ImGui_ImplGlut_MouseButtonCallback(button, state);
   if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
   {
	   glBindFramebuffer(GL_FRAMEBUFFER, fbo_T);
	   glReadBuffer(GL_COLOR_ATTACHMENT1);
	   glPixelStorei(GL_PACK_ALIGNMENT, 1);
	   glReadPixels(x, screenHeight - y - 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, bufferColor);
	   glBindFramebuffer(GL_FRAMEBUFFER, 0);
	   if (x>0)
	   {
		   std::cout << static_cast <GLuint> (bufferColor[1]) << std::endl;
	   }
   }
   
   
}

void changeView(int w, int h)
{
	glViewport(0, 0, w, h);
}

int main (int argc, char **argv)
{
	#if _DEBUG
		glutInitContextFlags(GLUT_DEBUG);
	#endif
		glutInitContextVersion(4, 3);
   //Configure initial window state using freeglut
   glutInit(&argc, argv); 
   glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
   glutInitWindowPosition (5, 5);
   glutInitWindowSize (screenWidth, screenHeight);
   int win = glutCreateWindow ("FBO Template");

   printGlInfo();

   //Register callback functions with glut. 
   glutDisplayFunc(display); 
   glutKeyboardFunc(keyboard);
   glutSpecialFunc(special);
   glutKeyboardUpFunc(keyboard_up);
   glutSpecialUpFunc(special_up);
   glutMouseFunc(mouse);
   glutMotionFunc(motion);
   glutPassiveMotionFunc(motion);

   glutIdleFunc(idle);
   glutReshapeFunc(changeView);

   initOpenGl();
   ImGui_ImplGlut_Init(); // initialize the imgui system

   //Enter the glut event loop.
   glutMainLoop();
   glutDestroyWindow(win);
   return 0;		
}


