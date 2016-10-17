
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include "maths_funcs.h"

// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace std;

GLuint numVertices = 6;
GLuint gWorldLocation;
mat4 World = identity_mat4();
mat4 World1 = identity_mat4();

// Vertex Shader (for convenience, it is defined in the main here, but we will be using text files for shaders in future)
// Note: Input to this shader is the vertex positions that we specified for the triangle. 
// Note: gl_Position is a special built-in variable that is supposed to contain the vertex position (in X, Y, Z, W)
// Since our triangle vertices were specified as vec3, we just set W to 1.0.

static const char* pVS = "                                                    \n\
#version 330                                                                  \n\
                                                                              \n\
in vec3 vPosition;															  \n\
in vec4 vColor;																  \n\
out vec4 color;																 \n\
uniform mat4 gWorld;                                                                               \n\
                                                                              \n\
void main()                                                                     \n\
{                                                                                \n\
	gl_Position = gWorld * vec4(vPosition, 1.0);  \n\
	color = vColor;							\n\
}";

// Fragment Shader
// Note: no input in this shader, it just outputs the colour of all fragments, in this case set to red (format: R, G, B, A).
static const char* pFS = "                                              \n\
#version 330                                                            \n\
                                                                        \n\
in vec4 color;															\n\
out vec4 FragColor;                                                      \n\
                                                                          \n\
void main()                                                               \n\
{                                                                          \n\
	FragColor = color;									 \n\
}";


// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS
static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		fprintf(stderr, "Error creating shader type %d\n", ShaderType);
		exit(0);
	}
	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderText, NULL);
	// compile the shader and check for errors
	glCompileShader(ShaderObj);
	GLint success;
	// check for shader related errors using glGetShaderiv
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024];
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
		exit(1);
	}
	// Attach the compiled shader object to the program object
	glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders()
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
	GLuint shaderProgramID = glCreateProgram();
	if (shaderProgramID == 0) {
		fprintf(stderr, "Error creating shader program\n");
		exit(1);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	AddShader(shaderProgramID, pVS, GL_VERTEX_SHADER);
	AddShader(shaderProgramID, pFS, GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { 0 };


	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
		exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
		exit(1);
	}
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
	glUseProgram(shaderProgramID);
	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS
GLuint generateObjectBuffer(GLfloat vertices[], GLfloat colors[]) {
	// Genderate 1 generic buffer object, called VBO
	GLuint VBO;
	glGenBuffers(1, &VBO);
	// In OpenGL, we bind (make active) the handle to a target name and then execute commands on that target
	// Buffer will contain an array of vertices 
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// After binding, we now fill our object with data, everything in "Vertices" goes to the GPU
	glBufferData(GL_ARRAY_BUFFER, (numVertices * 7 * sizeof(GLfloat)), NULL, GL_STATIC_DRAW);
	// if you have more data besides vertices (e.g., vertex colours or normals), use glBufferSubData to tell the buffer when the vertices array ends and when the colors start
	glBufferSubData(GL_ARRAY_BUFFER, 0, numVertices * 3 * sizeof(GLfloat), vertices);
	glBufferSubData(GL_ARRAY_BUFFER, numVertices * 3 * sizeof(GLfloat), numVertices * 4 * sizeof(GLfloat), colors);
	return VBO;
}

void linkCurrentBuffertoShader(GLuint shaderProgramID) {
	// find the location of the variables that we will be using in the shader program
	GLuint positionID = glGetAttribLocation(shaderProgramID, "vPosition");
	GLuint colorID = glGetAttribLocation(shaderProgramID, "vColor");
	// Have to enable this
	glEnableVertexAttribArray(positionID);
	// Tell it where to find the position data in the currently active buffer (at index positionID)
	glVertexAttribPointer(positionID, 3, GL_FLOAT, GL_FALSE, 0, 0);
	// Similarly, for the color data.
	glEnableVertexAttribArray(colorID);
	glVertexAttribPointer(colorID, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(numVertices * 3 * sizeof(GLfloat)));
}
#pragma endregion VBO_FUNCTIONS

void display() {
	glClear(GL_COLOR_BUFFER_BIT);
	//print(World);
	glUniformMatrix4fv(gWorldLocation, 1, GL_FALSE, &World.m[0]);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	//print(World1);
	glUniformMatrix4fv(gWorldLocation, 1, GL_FALSE, &World1.m[0]);
	glDrawArrays(GL_TRIANGLES, 3, 6);
	glutSwapBuffers();
}

void init()
{
	// Create 3 vertices that make up a triangle that fits on the viewport 
	GLfloat vertices[] = { -1.0f, -0.5f, 0.0f,
		0.0f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		1.0f, -0.5f, 0.0f,
		0.0f, -0.5f, 0.0f,
		0.5f, 0.5f, 0.0f };
	// Create a color array that identfies the colors of each vertex (format R, G, B, A)
	GLfloat colors[] = { 1.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f };

	// Set up the shaders
	GLuint shaderProgramID = CompileShaders();
	// Put the vertices and colors into a vertex buffer object
	generateObjectBuffer(vertices, colors);

	// Link the current buffer to the shader
	linkCurrentBuffertoShader(shaderProgramID);
}

void keyPressed(unsigned char key, int x, int y) {
	/* stored like this:
	0 4 8  12
	1 5 9  13
	2 6 10 14
	3 7 11 15*/
	cout << key;
	if (key == 'w') {
		World.m[13] += 0.1f;
		World1.m[13] += -0.1f;
	}
	else if (key == 's') {
		World.m[13] += -0.1f;
		World1.m[13] += 0.1f;
	}
	else if (key == 'd') {
		World.m[12] += 0.1f;
		World1.m[12] += -0.1f;
	}
	else if (key == 'g') {
		World.m[14] += 0.1f;
	}
	else if (key == 'a') {
		World.m[12] += -0.1f;
		World1.m[12] += 0.1f;
	}
	else if (key == 'l') {
		World = rotate_z_deg(World, 5.0f);
		World = rotate_y_deg(World, 5.0f);
		World = rotate_x_deg(World, 5.0f);
	}
	else if (key == 'j') {
		World = rotate_z_deg(World, -5.0f);
		World = rotate_y_deg(World, -5.0f);
		World = rotate_x_deg(World, -5.0f);
	}
	else if (key == 'q') {
		World.m[0] *= 1.1f;
		World.m[5] *= 1.1f;
		World.m[10] *= 1.1f;
	}
	else if (key == 'e') {
		World.m[0] *= 0.9f;
		World.m[5] *= 0.9f;
		World.m[10] *= 0.9f;
	}
	else if (key == 'i') {
		World.m[0] *= 1.2f;
		World.m[5] *= 1.1f;
		World.m[10] *= 1.1f;
	}
	else if (key == 'k') {
		World.m[0] *= 0.8f;
		World.m[5] *= 0.9f;
		World.m[10] *= 0.9f;
	}
	else if (key == 'n') {
		World.m[12] += 0.1f;
		World.m[0] *= 1.2f;
		World.m[5] *= 1.1f;
		World.m[10] *= 1.1f;
		World = rotate_z_deg(World, 5.0f);
	}
	else if (key == 'm') {
		World.m[12] += -0.1f;
		World.m[0] *= 0.8f;
		World.m[5] *= 0.9f;
		World.m[10] *= 0.9f;
		World = rotate_z_deg(World, -5.0f);
	}
	glUniformMatrix4fv(gWorldLocation, 1, GL_FALSE, &World.m[0]);
	glutPostRedisplay();

}

int main(int argc, char** argv) {

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(800, 600);
	glutCreateWindow("Hello Triangle");
	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutKeyboardFunc(keyPressed);
	// A call to glewInit() must be done after glut is initialized!
	GLenum res = glewInit();
	// Check for any errors
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}
	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
	return 0;
}
