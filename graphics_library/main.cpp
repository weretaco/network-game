#include <cstdio>
#include <cstdlib>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>

#include <GL/glew.h>
#include <GL/glfw.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

using namespace std;

/*

Rendering Pipeline

All models are defined
All vertices in each model are in model space

Apply the model transformation to each model (based on model position, rotation, and scaling)
This results in all vertices being in world space

Apply the view transformation (based on camera position and rotation)
Vertices are in camera coordinates

Apply the projection matrix (camera lens angle, clipping planes) to turn coordinates into screen coordinates

*/

GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path);

int main() {
   // Initialise GLFW
   if( !glfwInit() )
   {
      fprintf( stderr, "Failed to initialize GLFW\n" );
      return -1;
   }

   glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 4);
   glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
   glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 3);
   glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

   // Open a window and create its OpenGL context
   if( !glfwOpenWindow( 1024, 768, 0,0,0,0, 32,0, GLFW_WINDOW ) )
   {
      fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
      glfwTerminate();
      return -1;
   }

   // Initialize GLEW
   glewExperimental = true; // Needed for core profile
   if (glewInit() != GLEW_OK) {
      fprintf(stderr, "Failed to initialize GLEW\n");
      return -1;
   }

   glfwSetWindowTitle( "Tutorial 02" );

   // Ensure we can capture the escape key being pressed below
   glfwEnable( GLFW_STICKY_KEYS );

   // Dark blue background
   glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

   // Enable depth test
   glEnable(GL_DEPTH_TEST);
   // Accept fragment if it closer to the camera than the former one
   glDepthFunc(GL_LESS);

   GLuint VertexArrayID;
   glGenVertexArrays(1, &VertexArrayID);
   glBindVertexArray(VertexArrayID);

   // Create and compile our GLSL program from the shaders
   GLuint programID = LoadShaders( "SimpleTransform.vertexshader", "SingleColor.fragmentshader" );

   // Get a handle for our "MVP" uniform
   GLuint MatrixID = glGetUniformLocation(programID, "MVP");

   // Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
   glm::mat4 Projection = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
   // Or, for an ortho camera :
   //glm::mat4 Projection = glm::ortho(-10.0f,10.0f,-10.0f,10.0f,0.0f,100.0f); // In world coordinates
        
   // Camera matrix
   glm::mat4 View = glm::lookAt(
      glm::vec3(4,3,3), // Camera is at (4,3,3), in World Space
      glm::vec3(0,0,0), // and looks at the origin
      glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
   );

   // Model matrix : an identity matrix (model will be at the origin)
   glm::mat4 Model = glm::mat4(1.0f);
   // Our ModelViewProjection : multiplication of our 3 matrices
   glm::mat4 MVP = Projection * View * Model; // Remember, matrix multiplication is the other way around

   // Our vertices. Tree consecutive floats give a 3D vertex; Three consecutive vertices give a triangle.
   // A cube has 6 faces with 2 triangles each, so this makes 6*2=12 triangles, and 12*3 vertices
   static const GLfloat g_vertex_buffer_data[] = {
      -1.0f,-1.0f,-1.0f, // triangle 1 : begin
      -1.0f,-1.0f, 1.0f,
      -1.0f, 1.0f, 1.0f, // triangle 1 : end
      1.0f, 1.0f,-1.0f, // triangle 2 : begin
      -1.0f,-1.0f,-1.0f,
      -1.0f, 1.0f,-1.0f, // triangle 2 : end
      1.0f,-1.0f, 1.0f,
      -1.0f,-1.0f,-1.0f,
      1.0f,-1.0f,-1.0f,
      1.0f, 1.0f,-1.0f,
      1.0f,-1.0f,-1.0f,
      -1.0f,-1.0f,-1.0f,
      -1.0f,-1.0f,-1.0f,
      -1.0f, 1.0f, 1.0f,
      -1.0f, 1.0f,-1.0f,
      1.0f,-1.0f, 1.0f,
      -1.0f,-1.0f, 1.0f,
      -1.0f,-1.0f,-1.0f,
      -1.0f, 1.0f, 1.0f,
      -1.0f,-1.0f, 1.0f,
      1.0f,-1.0f, 1.0f,
      1.0f, 1.0f, 1.0f,
      1.0f,-1.0f,-1.0f,
      1.0f, 1.0f,-1.0f,
      1.0f,-1.0f,-1.0f,
      1.0f, 1.0f, 1.0f,
      1.0f,-1.0f, 1.0f,
      1.0f, 1.0f, 1.0f,
      1.0f, 1.0f,-1.0f,
      -1.0f, 1.0f,-1.0f,
      1.0f, 1.0f, 1.0f,
      -1.0f, 1.0f,-1.0f,
      -1.0f, 1.0f, 1.0f,
      1.0f, 1.0f, 1.0f,
      -1.0f, 1.0f, 1.0f,
      1.0f,-1.0f, 1.0f
   };

   GLuint vertexbuffer;
   glGenBuffers(1, &vertexbuffer);
   glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

   // One color for each vertex. They were generated randomly.
   static const GLfloat g_color_buffer_data[] = {
      0.583f,  0.771f,  0.014f,
      0.609f,  0.115f,  0.436f,
      0.327f,  0.483f,  0.844f,
      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.310f,  0.747f,  0.185f,
      0.597f,  0.770f,  0.761f,
      0.559f,  0.436f,  0.730f,
      0.359f,  0.583f,  0.152f,
      0.483f,  0.596f,  0.789f,
      0.559f,  0.861f,  0.639f,
      0.195f,  0.548f,  0.859f,
      0.014f,  0.184f,  0.576f,
      0.771f,  0.328f,  0.970f,
      0.406f,  0.615f,  0.116f,
      0.676f,  0.977f,  0.133f,
      0.971f,  0.572f,  0.833f,
      0.140f,  0.616f,  0.489f,
      0.997f,  0.513f,  0.064f,
      0.945f,  0.719f,  0.592f,
      0.543f,  0.021f,  0.978f,
      0.279f,  0.317f,  0.505f,
      0.167f,  0.620f,  0.077f,
      0.347f,  0.857f,  0.137f,
      0.055f,  0.953f,  0.042f,
      0.714f,  0.505f,  0.345f,
      0.783f,  0.290f,  0.734f,
      0.722f,  0.645f,  0.174f,
      0.302f,  0.455f,  0.848f,
      0.225f,  0.587f,  0.040f,
      0.517f,  0.713f,  0.338f,
      0.053f,  0.959f,  0.120f,
      0.393f,  0.621f,  0.362f,
      0.673f,  0.211f,  0.457f,
      0.820f,  0.883f,  0.371f,
      0.982f,  0.099f,  0.879f
   };

   GLuint colorbuffer;
   glGenBuffers(1, &colorbuffer);
   glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);

   do{

      // Clear the screen
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      // Use our shader
      glUseProgram(programID);

      // Send our transformation to the currently bound shader, 
      // in the "MVP" uniform
      glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

      // 1rst attribute buffer : vertices
      glEnableVertexAttribArray(0);
      glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
      glVertexAttribPointer(
         0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
         3,                  // size
         GL_FLOAT,           // type
         GL_FALSE,           // normalized?
         0,                  // stride
         (void*)0            // array buffer offset
      );

      // 2nd attribute buffer : colors
      glEnableVertexAttribArray(1);
      glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
      glVertexAttribPointer(
         1,                   // attribute. No particular reason for 1, but must match the layout in the shader.
         3,                   // size
         GL_FLOAT,            // type
         GL_FALSE,            // normalized?
         0,                   // stride
         (void*)0             // array buffer offset
      );

      // Draw the triangle !
      glDrawArrays(GL_TRIANGLES, 0, 12*3);

      glDisableVertexAttribArray(0);

      // Swap buffers
      glfwSwapBuffers();

   } // Check if the ESC key was pressed or the window was closed
   while( glfwGetKey( GLFW_KEY_ESC ) != GLFW_PRESS &&
      glfwGetWindowParam( GLFW_OPENED ) );

   // Cleanup VBO
   glDeleteBuffers(1, &vertexbuffer);
   glDeleteProgram(programID);
   glDeleteVertexArrays(1, &VertexArrayID);

   // Close OpenGL window and terminate GLFW
   glfwTerminate();

   return 0;
}

GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {
 
   // Create the shaders
   GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
   GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

   // Read the Vertex Shader code from the file
   string VertexShaderCode;
   ifstream VertexShaderStream(vertex_file_path, ios::in);
   if(VertexShaderStream.is_open()){
      string Line = "";
      while(getline(VertexShaderStream, Line))
            VertexShaderCode += "\n" + Line;
      VertexShaderStream.close();
   }else {
      printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
      getchar();
      return 0;
   }

   // Read the Fragment Shader code from the file
   string FragmentShaderCode;
   ifstream FragmentShaderStream(fragment_file_path, ios::in);
   if(FragmentShaderStream.is_open()) {
      string Line = "";
      while(getline(FragmentShaderStream, Line))
         FragmentShaderCode += "\n" + Line;
      FragmentShaderStream.close();
   }



   GLint Result = GL_FALSE;
   int InfoLogLength;



   // Compile Vertex Shader
   printf("Compiling shader : %s\n", vertex_file_path);
   char const * VertexSourcePointer = VertexShaderCode.c_str();
   glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
   glCompileShader(VertexShaderID);

   // Check Vertex Shader
   glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
   glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
   if ( InfoLogLength > 0 ){
      vector<char> VertexShaderErrorMessage(InfoLogLength+1);
      glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
      printf("%s\n", &VertexShaderErrorMessage[0]);
   }



   // Compile Fragment Shader
   printf("Compiling shader : %s\n", fragment_file_path);
   char const * FragmentSourcePointer = FragmentShaderCode.c_str();
   glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
   glCompileShader(FragmentShaderID);

   // Check Fragment Shader
   glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
   glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
   if ( InfoLogLength > 0 ){
      vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
      glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
      printf("%s\n", &FragmentShaderErrorMessage[0]);
   }



   // Link the program
   printf("Linking program\n");
   GLuint ProgramID = glCreateProgram();
   glAttachShader(ProgramID, VertexShaderID);
   glAttachShader(ProgramID, FragmentShaderID);
   glLinkProgram(ProgramID);

   // Check the program
   glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
   glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
   if ( InfoLogLength > 0 ){
      vector<char> ProgramErrorMessage(InfoLogLength+1);
      glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
      printf("%s\n", &ProgramErrorMessage[0]);
   }

   glDeleteShader(VertexShaderID);
   glDeleteShader(FragmentShaderID);

   return ProgramID;
}