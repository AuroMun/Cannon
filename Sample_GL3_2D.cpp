#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
//#include <random>

#define airResistance 0.985
#define gravity 0.01
#define NUMBER_OF_LEVELS 4
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace std;

double xpos, ypos, xposNew, yposNew;
int isKeyboard = 0; //Set to 1 to use keyboard
int platformNumber = 4, obstacleNumber = 0;
double obstacleData[3*15];
double targetX = 1, targetY = 1;
double platformData[15*4]; //= {-3,-3,6,0.5, 3,-2,6,0.5, 4,6,5,0.5, -7,1.5,2,0.5};
//double obstacle[10*3];
float canX = -14; float canY = -7; float canR = 0.4;
int is_ball=0, level=1; //is_ball == 1 if there's a ball in the air
char levelString[] = "1.txt";



int random(int min, int max) //range : [min, max)
{
   static bool first = true;
   if ( first )
   {
      srand(time(NULL)); //seeding for the first time only!
      first = false;
   }
   return min + rand() % (max - min);
}


struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

GLuint programID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
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
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}


/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}


/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
            quit(window);
            break;
		default:
			break;
	}
}

/* Executed when a mouse button is pressed/released */


GLfloat fov = 2.498f;
/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
     is different from WindowSize */
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);



	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	/* glMatrixMode (GL_PROJECTION);
	   glLoadIdentity ();
	   gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
	// Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    //ov=1.0f;
     Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
    //Matrices.projection = glm::ortho(-16.0f, 16.0f, -9.0f, 9.0f, 0.1f, 500.0f);
}

VAO *triangle, *rectangle, *circle;

// Creates the triangle object used in this sample code
void createTriangle ()
{
  /* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

  /* Define vertex array as used in glBegin (GL_TRIANGLES) */
  static const GLfloat vertex_buffer_data [] = {
    0, 1,0, // vertex 0
    -1,-1,0, // vertex 1
    1,-1,0, // vertex 2
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 0
    0,1,0, // color 1
    0,0,1, // color 2
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  triangle = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_LINE);
}


VAO* createCircle(float r, float col1 = 1, float col2 = 0.843, float col3 = 0)
  {
    float rad = r;
    //GLfloat vertex_buffer_data[3*3] = {1, 1, 0, -1, 1, 0, -1, -1, 0,};
    GLfloat vertex_buffer_data[3*1000]; // = {1, 1, 0, -1, 1, 0, -1, -1, 0,};
    int i;
    for(i=0; i<1000; i+=3){
      if(i%6==3){
        vertex_buffer_data[i] = 0;
        vertex_buffer_data[i+1] = 0;
        vertex_buffer_data[i+2] = 0;
        continue;
      }
      vertex_buffer_data[i] = rad*sin(2.0*3.141592*i/100);
      vertex_buffer_data[i+1] = rad*cos(2.0*3.141592*i/100);
      vertex_buffer_data[i+2] = 0;
    }

     GLfloat color_buffer_data [3*1000];
     for(i=0; i<1000; i+=3){
      color_buffer_data[i] = col1;
      color_buffer_data[i+1] = col2;
      color_buffer_data[i+2] = col3;
    }

    // create3DObject creates and returns a handle to a VAO that can be used later
    return create3DObject(GL_TRIANGLES, 300, vertex_buffer_data, color_buffer_data, GL_FILL);
  }
//Create a rectangle object
VAO* createRectangle ( float width1, float height1)
{
  // GL3 accepts only Triangles. Quads are not supported
  VAO *recta;
  float x = 0; float y = 0; float width = width1; float height = height1;
  GLfloat vertex_buffer_data [] = {
    x,y,0, // vertex 1
    x,y+height,0, // vertex 2
    x+width, y+height,0, // vertex 3

    x+width, y+height,0, // vertex 3
    x+width, y,0, // vertex 4
    x,y,0  // vertex 1
  };

  GLfloat color_buffer_data [] = {
    1,0,0, // color 1
    1,0,0, // color 2
    1,0,0, // color 3

    1,0,0, // color 3
    1,0,0, // color 4
    1,0,0  // color 1
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  recta = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
  return recta;
}


glm::mat4 MVP;
glm::mat4 VP;

float camera_rotation_angle = 90;
float rectangle_rotation = 0;
float triangle_rotation = 0;
float translateX = 0;
float translateY = 0;

//Map class, updates happen here
class map{
  public:
    vector<VAO*> arr_rec;
    VAO* gun; float gunRotation;
    void mapInit(){
      gun = createRectangle( 2, 1 ); gunRotation = 0;
    }
    void update(){
      for(int i=0; i < arr_rec.size(); i++) { //rectangle
        Matrices.model = glm::mat4(1.0f);
        // Get mouse position
        //xpos*8/1080, ypos*8/1080

        glm::mat4 translateRectangle = glm::translate (glm::vec3(0, 0, 0));
        //cout<<xpos<<" "<<ypos<<endl;
        //cout<<(xpos-500)*8/1000<<" "<<(-ypos+500)*8/1000<<endl;

        Matrices.model *= translateRectangle;
        MVP = VP * Matrices.model;
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
        draw3DObject(arr_rec[i]);
      }
      Matrices.model = glm::mat4(1.0f);

      //Rotate about -3, -3
      glm::mat4 translateGun = glm::translate (glm::vec3(-14, -7, 0));        // glTranslatef
      float gunRotationAngle = atan2((yposNew+7),(xposNew+14)); //cout<<"Rot angle: "<<(yposNew+14)/(xposNew+7)<<endl;
      glm::mat4 rotateGun = glm::rotate((float)(gunRotationAngle), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
      glm::mat4 translateAgain =  glm::translate (glm::vec3(+14, +7, 0));        // glTranslatef

      //Translate to where it was
      glm::mat4 translateAgain1 =  glm::translate (glm::vec3(-14+1.3/2, -7.5, 0));        // glTranslatef

      Matrices.model *= (translateGun * rotateGun * translateAgain * translateAgain1);
      MVP = VP * Matrices.model;
      glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);;
      gunRotation++;
      draw3DObject(gun);
    }
}World;


class obj{
public:
  int is_circle, isPhysics;
  double xPos, yPos, width, height, radius;
  double xVel, yVel, xAcc, yAcc;
  VAO* toDraw;

  void objInit(double xPosNew,double yPosNew, double widthNew, double heightNew){
    xPos = xPosNew; yPos = yPosNew;
    width = widthNew; height = heightNew;
    toDraw = createRectangle(width, height);
  }
  void objInit(double xPosNew,double yPosNew,double radiusNew){
    xPos = xPosNew; yPos = yPosNew; radius = radiusNew;
    toDraw = createCircle(radius); isPhysics = 0;
    update();
  }
  void setColor(double col1, double col2, double col3){
    toDraw = createCircle(radius, col1, col2, col3);
    update();
  }
  void reset(double xPosNew, double yPosNew){
    xPos = xPosNew; yPos = yPosNew;
    xVel = yVel = xAcc = yAcc = 0;
    update();
  }
  void update(){
    //if(radius!=0.1)checkCollision(xPos, yPos, width, height);
    xVel += xAcc; yVel += yAcc;
    xPos += xVel; yPos += yVel;
    //if(xVel > 0.4)xVel=0.4; if(yVel>0.4)yVel=0.4;
    if(isPhysics==1) updatePhysics();
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translate = glm::translate (glm::vec3(xPos, yPos, 0));        // glTranslatef
    glm::mat4 rotate = glm::rotate((float)(0), glm::vec3(0,0,1));
    Matrices.model *= (translate * rotate);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);;
    draw3DObject(toDraw);
  }
  void updatePhysics(){
    xVel *= airResistance; yVel *= airResistance;
    yAcc = -gravity;
  }

}cannonball, cannon, targetA, targetInner[4], wall[4], platform[10], obstacle[10];



void checkCollision(obj &A, obj &B){ //B is a circle, A is a rectangle
  double xRect = A.xPos; double yRect = A.yPos; double width = A.width; double height = A.height;
  double x = B.xPos; double y = B.yPos; double yVel = B.yVel; double xVel = B.xVel;
  //cout<<x<<" "<<y<<" "<<yVel<<" "<<xVel<<endl;
  if(x > xRect && x < xRect+width && y > yRect+height && y+yVel-B.radius<yRect+height){
    B.yPos=yRect+height+B.radius;
    //cout<<B.yPos<<" "<<cannonball.yPos<<endl;
    B.yVel*=-0.9;
    B.xVel*=0.98;
  }
  else if(x > xRect && x < xRect+width && y < yRect&& y+yVel+B.radius>yRect){
    B.yPos=yRect-B.radius;
    B.yVel*=-0.9;
  }
  else if(y > yRect && y < yRect+height && x > xRect+width && x+xVel-B.radius<xRect+width){
    B.xPos = xRect+width+B.radius;
    B.xVel*=-1;
  }
  else if(y > yRect && y < yRect+height && x < xRect && x+xVel+B.radius>xRect){
    B.xPos = xRect-B.radius;
    B.xVel*=-1;
  }
}

int checkCollisionCircle(obj &firstBall, obj &secondBall){ //Circle //For now, B is target
  if (firstBall.xPos + firstBall.radius + secondBall.radius > secondBall.xPos
    && firstBall.xPos < secondBall.xPos + firstBall.radius + secondBall.radius
    && firstBall.yPos + firstBall.radius + secondBall.radius > secondBall.yPos
    && firstBall.yPos < secondBall.yPos + firstBall.radius + secondBall.radius)
  {
    //AABBs are overlapping
    if(secondBall.xVel < firstBall.xVel)
    secondBall.xVel += firstBall.xVel*0.5, firstBall.xVel /= 2;
    else
    firstBall.xVel += secondBall.xVel*0.5, secondBall.xVel /= 2;

    if(secondBall.yVel < firstBall.yVel)
    secondBall.yVel += firstBall.yVel*0.5, firstBall.yVel /= 2;
    else
    firstBall.yVel += secondBall.yVel*0.5, secondBall.yVel/=2;
    return 1;
  }
  return 0;
}

void makewalls(){
  wall[0].objInit(-16, -9, 32, 0.2);
  wall[1].objInit(-16, -9, 0.2, 18);
  wall[2].objInit(-16, 8.8, 32, 0.2);
  wall[3].objInit(15.8, -9, 0.2, 18);
  for(int i=0; i<platformNumber; i++){
    //cout<<platformData[i]<<endl;
    platform[i].objInit(platformData[4*i], platformData[4*i+1], platformData[4*i+2], platformData[4*i+3]);
    //if(i==1)platform[i].yVel=0.02;
  }
  for(int i=0; i<4; i++)wall[i].update();
  for(int i=0; i<platformNumber; i++)platform[i].update();
}

void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{

  if (action == GLFW_RELEASE) {
    switch (key) {
      case GLFW_KEY_SPACE:
        cannonball.isPhysics = 1;
        double a = sqrt((xposNew-canX)*(xposNew-canX) + (yposNew-canY)*(yposNew-canY));
        cannonball.reset(canX + (xposNew-canX)*0/a , canY + (yposNew-canY)*0/a);
        cannonball.xVel = (xposNew-canX)*1/20 ; cannonball.yVel = (yposNew-canY)*1/20;
      break;
    }
  }
  else if (action == GLFW_PRESS) {
    switch (key) {
      case GLFW_KEY_ESCAPE:
      quit(window);
      break;
      default:
      break;
    }
  }
}

void levelGen(){
  ifstream fin;
  levelString[0] = char(level)+'0';
  //cout<<levelString<<endl;
  fin.open(levelString);
  fin>>platformNumber; //cout<<platformNumber;
  for(int i=0; i<4*platformNumber; i++)fin>>platformData[i];
  fin>>targetX;
  fin>>targetY;
  makewalls();

  targetA.objInit(targetX, targetY, 0.8); targetA.reset(targetX, targetY);
  targetInner[0].objInit(targetX, targetY, 0.6); targetInner[0].setColor(1, 1, 0.878);
  targetInner[1].objInit(targetX, targetY, 0.5); targetInner[1].setColor(0.275, 0.510, 0.706);
  targetInner[2].objInit(targetX, targetY, 0.4); targetInner[2].setColor(0.902, 0.902, 0.980);
  targetInner[3].objInit(targetX, targetY, 0.3); targetInner[3].setColor(0.863, 0.078, 0.235);

}
float panX=0, panY=0; int panState=0;

//Divanshu, set keyboard controls as specified in the requirements.
//Press alt+~ to get to the other atom tab, I have 1.txt and 2.txt, levels - Modify and add more x.txt's.
//NUMBER_OF_LEVELS - #Defined. Change to modify the number of levels.

void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:

            if (action == GLFW_RELEASE){
                cannonball.isPhysics = 1;
                double a = sqrt((xposNew-canX)*(xposNew-canX) + (yposNew-canY)*(yposNew-canY));
                cannonball.reset(canX + (xposNew-canX)*0/a , canY + (yposNew-canY)*0/a);
                cannonball.xVel = (xposNew-canX)*1/20 ; cannonball.yVel = (yposNew-canY)*1/20;
              }
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            panState = 1;
            if (action == GLFW_RELEASE)panState = 0;
            break;
        default:
            break;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  fov -= yoffset/200;
  if(fov>2.498f)fov=2.498f;
}

/* Render the scene with openGL */
/* Edit this function according to your assignment */

void draw ()
{
  // clear the color and depth in the frame buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(0.9f, 0.9f, 0.98f, 0.0f);
  // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram (programID);

  // Eye - Location of camera. Don't change unless you are sure!!
  glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  glm::vec3 target (0, 0, 0);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up (0, 1, 0);

  // Compute Camera matrix (view)
  // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D

  //  Don't change unless you are sure!!
  if(panState==1)panX+=xposNew/100, panY+=yposNew/100;
  Matrices.view = glm::lookAt(glm::vec3(panX,panY,3), glm::vec3(panX,panY,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane
  int fbwidth=1280, fbheight=720;
  Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);
  // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  //  Don't change unless you are sure!!
  VP = Matrices.projection * Matrices.view;

  Matrices.model = glm::mat4(1.0f);

  // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
  // glPopMatrix ();

  World.update();

  //draw walls
  for(int i=0; i<4; i++){wall[i].update(); checkCollision(wall[i], cannonball);}
  for(int i=0; i<4; i++){wall[i].update(); checkCollision(wall[i], targetA);}
  for(int i=0; i<platformNumber; i++){platform[i].update(); checkCollision(platform[i], targetA);}
  for(int i=0; i<4; i++)for(int j=0; j<obstacleNumber; j++)checkCollision(wall[i], obstacle[j]);
  for(int i=0 ; i<platformNumber; i++)for(int j=0; j<obstacleNumber; j++)checkCollision(platform[i], obstacle[j]);
  for(int j=0; j<obstacleNumber; j++)if(checkCollisionCircle(cannonball, obstacle[j]));
  for(int i=0; i<platformNumber; i++){platform[i].update(); checkCollision(platform[i], cannonball);}
  for(int i=0; i<obstacleNumber; i++)for(int j=0; j<i; j++)checkCollisionCircle(obstacle[i], obstacle[j]);

  //for(int i=0; i<obstacleNumber; i++)for(int j=0; j<obstacleNumber; j++)checkCollisionCircle(obstacle[i], obstacle[j]);
  targetA.update();
  for(int i=0; i<4; i++)targetInner[i].update();

  //Draw cannon
  cannon.update();
  //Draw cannonball
  cannonball.update();

  for(int j=0; j<obstacleNumber; j++)
  obstacle[j].update();

  if(checkCollisionCircle(cannonball, targetA)){
    cannonball.isPhysics = 0; cannonball.reset(canX , canY);
    level++; if(level > NUMBER_OF_LEVELS)level = 1;
    levelGen();

    if (targetA.xVel>1) {
      targetA.xVel = 1;
    }
    if (targetA.yVel>1) {
      targetA.yVel = 1  ;
    }
  }
  glFlush();
}


/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
     is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling
    glfwSetScrollCallback(window, scroll_callback);

    /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
  World.mapInit();
  makewalls();
  cannonball.objInit(77, 77, canR); cannonball.setColor(1, 0.7, 0) ;cannonball.isPhysics = 1;
  for(int j=0; j<obstacleNumber; j++)obstacle[j].objInit(random(-6, 6), random(-6, 6), canR); //obstacle.isPhysics = 1;
  cannon.objInit(canX, canY, 1.4);


	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");
	reshapeWindow (window, width, height);
  // Background color of the scene
	glClearColor (0.3f, 0.3f, 0.3f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

    cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{
	int width = 1280;
	int height = 720;

    GLFWwindow* window = initGLFW(width, height);
    levelGen();
	   initGL (window, width, height);

    double last_update_time = glfwGetTime(), current_time;

    /* Draw in loop */
    while (!glfwWindowShouldClose(window)) {

        // OpenGL Draw commands


        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        if ((current_time - last_update_time) >= 0.01) { // atleast 0.5s elapsed since last frame
            // do something every 0.5 seconds ..
            draw();

            // Swap Frame Buffer in double buffering
            glfwSwapBuffers(window);

            glfwGetCursorPos(window, &xpos, &ypos);
            xposNew = (xpos-(1280/2))*32*fov/(2.498*1280); yposNew = (-ypos+(720/2))*fov*18/(2.498*720);
            //cout<<ypos<<" "<<xposNew<<" "<<yposNew<<" "<<(yposNew+7)/(xposNew+14)<<endl;
            // Poll for Keyboard and mouse events
            glfwPollEvents();
            last_update_time = current_time;
        }
    }

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
