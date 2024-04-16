//When using this as a template, be sure to make these changes in the new project: 
//1. In Debugging properties set the Environment to PATH=%PATH%;$(SolutionDir)\lib;
//2. Change window_title below
//3. Copy assets (mesh and texture) to new project directory

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "DebugCallback.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "Scene.h"
#include "Uniforms.h"
#include "InitShader.h"    //Functions for loading shaders from text files
#include "LoadMesh.h"      //Functions for creating OpenGL buffers from mesh files
#include "LoadTexture.h"   //Functions for creating OpenGL textures from image files
#include "VideoRecorder.h"      //Functions for saving videos

const int Scene::InitWindowWidth = 1024;
const int Scene::InitWindowHeight = 1024;

static const std::string vertex_shader("template_vs.glsl");
static const std::string fragment_shader("template_fs.glsl");
GLuint shader_program = -1;

static const std::string mesh_name = "suzanne.obj";
//static const std::string texture_name = "AmagoT.bmp";

//GLuint texture_id = -1; //Texture map for mesh
MeshData mesh_data;

float angle = 0.0f;
float yangle = 0.0f;
float scale = 1.0f;
static float m=5.0;
static float n=5.0;
static glm::vec4 ka = glm::vec4(1.0);
static glm::vec4 kd = glm::vec4(0.4);
static glm::vec4 ks = glm::vec4(0.6);
static bool full= false;
static bool fresnal = false;
static bool distribution = false;
static bool geometric_attenuation = false;
static bool trow_reitz = false;
static bool Ashikhmin_Shirley = false;
static bool strauss = false;
static bool lommel = false;
static bool iso = false;
static bool aniso = false;
static int o;
static int lighting_models;
static float nu = 0.1;
static float nv = 0.1;
static int  spec = 0;
bool recording = false;

namespace Scene
{
   namespace Camera
   {
      glm::mat4 P;
      glm::mat4 V;

      float Aspect = 1.0f;
      float NearZ = 0.1f;
      float FarZ = 100.0f;
      float Fov = glm::pi<float>() / 4.0f;

      void UpdateP()
      {
         P = glm::perspective(Fov, Aspect, NearZ, FarZ);
      }
   }
}


// This function gets called every time the scene gets redisplayed
void Scene::Display(GLFWwindow* window)
{
   //Clear the screen to the color previously specified in the glClearColor(...) call.
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   Camera::V = glm::lookAt(glm::vec3(Uniforms::SceneData.eye_w), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
   Uniforms::SceneData.PV = Camera::P * Camera::V;
   Uniforms::BufferSceneData();

   glUseProgram(shader_program);
   //Note that we don't need to set the value of a uniform here. The value is set with the "binding" in the layout qualifier
   //glBindTextureUnit(0, texture_id);

   //Set uniforms
   glm::mat4 M = glm::rotate(angle, glm::vec3(0.0f, 1.0f, 0.0f));
   glm::mat4 s = glm::scale(glm::vec3(scale * mesh_data.mScaleFactor));
  glm::mat4 k = glm::rotate(yangle, glm::vec3(1.0f, 0.0f, 0.0f));
  
  glm::mat4 final_model = M * k * s;

 

   glUniformMatrix4fv(Uniforms::UniformLocs::M, 1, false, glm::value_ptr(final_model));
   
   //homework 3------
   glUniform1f(Uniforms::UniformLocs::m, m);
   glUniform1f(Uniforms::UniformLocs::n, n);
   glUniform4fv(Uniforms::UniformLocs::ka,1, glm::value_ptr(ka));
   glUniform4fv(Uniforms::UniformLocs::kd,1, glm::value_ptr(kd));
   glUniform4fv(Uniforms::UniformLocs::ks, 1, glm::value_ptr(ks));
   glUniform1i(Uniforms::UniformLocs::o, o);
   glUniform1i(Uniforms::UniformLocs::lighting_models, lighting_models);
   glUniform1f(Uniforms::UniformLocs::nu, nu);
   glUniform1f(Uniforms::UniformLocs::nv, nv);
   glUniform1i(Uniforms::UniformLocs::spec, spec);

   //homework 3------


   glBindVertexArray(mesh_data.mVao);
  // glDrawElements(GL_TRIANGLES, mesh_data.mSubmesh[0].mNumIndices, GL_UNSIGNED_INT, 0);
   //For meshes with multiple submeshes use mesh_data.DrawMesh(); 
   mesh_data.DrawMesh();

   DrawGui(window);

   if (recording == true)
   {
      glFinish();
      glReadBuffer(GL_BACK);
      int w, h;
      glfwGetFramebufferSize(window, &w, &h);
      VideoRecorder::EncodeBuffer(GL_BACK);
   }

   /* Swap front and back buffers */
   glfwSwapBuffers(window);
}

void Scene::DrawGui(GLFWwindow* window)
{
   //Begin ImGui Frame
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();

   //Draw Gui
   ImGui::Begin("Debug window");
   if (ImGui::Button("Quit"))
   {
      glfwSetWindowShouldClose(window, GLFW_TRUE);
   }

   const int filename_len = 256;
   static char video_filename[filename_len] = "capture.mp4";
   static bool show_imgui_demo = false;

   if (recording == false)
   {
      if (ImGui::Button("Start Recording"))
      {
         int w, h;
         glfwGetFramebufferSize(window, &w, &h);
         recording = true;
         const int fps = 60;
         const int bitrate = 4000000;
         VideoRecorder::Start(video_filename, w, h, fps, bitrate); //Uses ffmpeg
      }
   }
   else
   {
      if (ImGui::Button("Stop Recording"))
      {
         recording = false;
         VideoRecorder::Stop(); //Uses ffmpeg
      }
   }
   ImGui::SameLine();
   ImGui::InputText("Video filename", video_filename, filename_len);


   ImGui::SliderFloat("View angle", &angle, -glm::pi<float>(), +glm::pi<float>());
   ImGui::SliderFloat("View angle y direction", &yangle , -glm::pi<float>(), +glm::pi<float>());
   ImGui::SliderFloat("Scale", &scale, -10.0f, +10.0f);
   ImGui::SliderFloat("m for roughness", &m,-10.0f,+10.0f);
   ImGui::SliderFloat("n for fresnal term", &n, -10.0f, +10.0f);
   ImGui::SliderFloat("nu for Ashikhmin-Shirley ", &nu, 0.1f, +10000.0f);
   ImGui::SliderFloat("nv for Ashikhmin-Shirley", &nv, 0.1f, +10000.0f);


  
   ImGui::ColorEdit4("Material Specular Color", glm::value_ptr(ks));
   ImGui::ColorEdit4("Material Ambient Color", glm::value_ptr(ka));
   ImGui::ColorEdit4("Material Diffuse Color", glm::value_ptr(kd));

  // if (ImGui::RadioButton("full lighting model",  full)) {
  //     o = 0;
  //     full = true;
  //     fresnal = false;
  //     distribution = false;
  //     geometric_attenuation = false;
  //}
  /* if (ImGui::RadioButton("fresnal only lighting model", fresnal) ){
       o =1;
       full = false;
       fresnal = true;
       distribution = false;
       geometric_attenuation = false;
   }
   if (ImGui::RadioButton("Distribution  factor only lighting model", distribution) ){
       o = 2;
       full = false;
       fresnal = false;
       distribution = true;
       geometric_attenuation = false;
   }
   if (ImGui::RadioButton("Geometric attenuation only lighting model", geometric_attenuation) ){
       o = 3;
       full = false;
       fresnal = false;
       distribution = false;
       geometric_attenuation = true;
   }*/
   if (ImGui::RadioButton("trow-reitz lighting model", trow_reitz)) {
       lighting_models = 0;
       trow_reitz = true;
       Ashikhmin_Shirley = false;
       strauss = false;
       lommel = false;
   }

   if (ImGui::RadioButton("Ashikhmin-Shirley", Ashikhmin_Shirley)) {
       lighting_models = 1;
       Ashikhmin_Shirley = true;
       trow_reitz = false;
       strauss = false;
       lommel = false;
   }

   if (ImGui::RadioButton("Strauss Lighting model", strauss)) {
       lighting_models = 2;
       Ashikhmin_Shirley = false;
       trow_reitz = false;
       strauss = true;
       lommel = false;
   }
   if (ImGui::RadioButton("Lommel-seelinger lighting model", lommel)) {
       lighting_models = 3;
       Ashikhmin_Shirley = false;
       trow_reitz = false;
       strauss = false;
       lommel = true;
   }

   //if (ImGui::RadioButton("iso metric specular for Ashikhmin-Shirley ", iso)) {
   //    spec = 0;
   //   iso = true;
   //    aniso = false;
   //}   
   //if (ImGui::RadioButton("aniiso metric specular for Ashikhmin-Shirley ", aniso)) {
   //    spec = 1;
   //    aniso = true;
   //   iso = false;
   //    
   //}


   if (ImGui::Button("Show ImGui Demo Window"))
   {
      show_imgui_demo = true;
   }
   ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

   ImGui::End();

   if (show_imgui_demo == true)
   {
      ImGui::ShowDemoWindow(&show_imgui_demo);
   }

   //End ImGui Frame
   ImGui::Render();
   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Scene::Idle()
{
   float time_sec = static_cast<float>(glfwGetTime());

   //Pass time_sec value to the shaders
   glUniform1f(Uniforms::UniformLocs::time, time_sec);
}

void Scene::ReloadShader()
{
   GLuint new_shader = InitShader(vertex_shader.c_str(), fragment_shader.c_str());

   if (new_shader == -1) // loading failed
   {
      DebugBreak(); //alert user by breaking and showing debugger
      glClearColor(1.0f, 0.0f, 1.0f, 0.0f); //change clear color if shader can't be compiled
   }
   else
   {
      glClearColor(0.35f, 0.35f, 0.35f, 0.0f);

      if (shader_program != -1)
      {
         glDeleteProgram(shader_program);
      }
      shader_program = new_shader;
   }
}

//Initialize OpenGL state. This function only gets called once.
void Scene::Init()
{
   glewInit();
   RegisterDebugCallback();
   //Print out information about the OpenGL version supported by the graphics driver.	
   std::ostringstream oss;
   oss << "GL_VENDOR: " << glGetString(GL_VENDOR) << std::endl;
   oss << "GL_RENDERER: " << glGetString(GL_RENDERER) << std::endl;
   oss << "GL_VERSION: " << glGetString(GL_VERSION) << std::endl;
   oss << "GL_SHADING_LANGUAGE_VERSION: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

   int max_uniform_block_size = 0;
   glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &max_uniform_block_size);
   oss << "GL_MAX_UNIFORM_BLOCK_SIZE: " << max_uniform_block_size << std::endl;

   int max_storage_block_size = 0;
   glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &max_storage_block_size);
   oss << "GL_MAX_SHADER_STORAGE_BLOCK_SIZE: " << max_storage_block_size << std::endl;

   int max_texture_size = 0;
   glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
   oss << "GL_MAX_TEXTURE_SIZE: " << max_texture_size << std::endl;

   int max_3d_texture_size = 0;
   glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max_3d_texture_size);
   oss << "GL_MAX_3D_TEXTURE_SIZE: " << max_3d_texture_size << std::endl;

   //Output to console and file
   std::cout << oss.str();

   std::fstream info_file("info.txt", std::ios::out | std::ios::trunc);
   info_file << oss.str();
   info_file.close();
   glEnable(GL_DEPTH_TEST);

   ReloadShader();
   mesh_data = LoadMesh(mesh_name);
  // texture_id = LoadTexture(texture_name);

   Camera::UpdateP();
   Uniforms::Init();
}