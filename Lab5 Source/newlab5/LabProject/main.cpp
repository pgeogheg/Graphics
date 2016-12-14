
//Some Windows Headers (For Time, IO, etc.)
#include "text.h"
#include <time.h>
#include <windows.h>
#include <mmsystem.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include "maths_funcs.h"
#include <SOIL.h>

// Assimp includes

#include <assimp/cimport.h> // C importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations
#include <stdio.h>
#include <math.h>
#include <vector> // STL dynamic memory.

const int MODEL_COUNT = 5;
int prevMouseX = 0;
int prevMouseY = 0;
int playerScore = 0;
float scoreCombo = 1.0;
int score_id = 0;
int timer_id = 0;
int scoreboard_id = 0;
int cursor_id = 0;
float playertime = 50.0;
float elapsed = 0.0;
bool playingGame = true;
bool gameLoseSeen = false;

int scoreBoard[3] = { 0, 0, 0 };

void enterIntoScoreboard (int score) {
	int i;
	for (i = 2; i >= 0; i--) {
		if (score > scoreBoard[i]) {
			if (i == 2) {
				scoreBoard[i] = score;
			}
			else {
				scoreBoard[i + 1] = scoreBoard[i];
				scoreBoard[i] = score;
			}
		}
		else {
			break;
		}
	}
}

float miniRotation = 0.0;

const char* atlas_image = "../freemono.png";
const char* atlas_meta = "../freemono.meta";

/*----------------------------------------------------------------------------
MESH TO LOAD
----------------------------------------------------------------------------*/
// this mesh is a dae file format but you should be able to use any other format too, obj is typically what is used
// put the mesh in your project directory, or provide a filepath for it here
char* MESH_NAMES[] = { "../playership.obj", "../invader.obj", "../arena.obj", "../bullet.obj", "../mothership.obj" };
/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

std::vector<float>* g_vp = new std::vector<float>[MODEL_COUNT];
std::vector<float>* g_vn = new std::vector<float>[MODEL_COUNT];
std::vector<float>* g_vt = new std::vector<float>[MODEL_COUNT];

int g_point_count[MODEL_COUNT] = { 0, 0, 0, 0, 0 };
GLuint vaos[MODEL_COUNT] = { 0, 0, 0, 0, 0 };

// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace std;
GLuint shaderProgramID;


unsigned int mesh_vao = 0;
int width = 800;
int height = 600;

GLuint loc1, loc2, loc3;
GLfloat rotate_y = 0.0f;

mat4 cameraTranslate = translate(identity_mat4(), vec3(0.0, 0.0, 0.0));
mat4 cameraRotate = rotate_x_deg(identity_mat4(), -30);
float cameraAlign = 0.0;

//----------------------
// INVADER SETTINGS
//----------------------
// !! Do NOT change these !!
class Invader {
public:
	float xPos = 0.0;
	float yPos = 0.0;
	bool active = true;
	int direction = 1;
};

Invader newInv(float x, float y) {
	Invader i;
	i.xPos = x;
	i.yPos = y;
	i.active = true;
	i.direction = 1;
	return i;
}

// You can change these
float invaderStartHeight = 4.5;
int invaderRow = 3;
int invaderColumn = 9;
float invaderSpeed = 0.02;
float invaderAcc = 0.005;
int invaderCount = 10;
int invaderTimer = 0;

Invader invaders[3][9];

void initInvaders() {
	int i, j;
	for (i = 0; i < invaderRow; i++) {
		for (j = 0; j < invaderColumn; j++) {
			int gap = (i % 2) * 20;
			invaders[i][j] = newInv(10 + (40 * j) + gap, (invaderStartHeight - (0.5 * i)));
		}
	}
}

void invadersMove() {
	int i, j;
	for (i = 0; i < invaderRow; i++) {
		for (j = 0; j < invaderColumn; j++) {
			Invader in = invaders[i][j];
			Invader newI;
			newI.active = in.active;
			newI.direction = in.direction;
			newI.xPos = in.xPos;
			newI.yPos = in.yPos;
			if (in.xPos >= 350) {
				newI.yPos = in.yPos - 0.2;
				newI.direction = in.direction = -1;
				PlaySound("../fastinvader1.wav", NULL, SND_ASYNC | SND_FILENAME);
			}
			if (in.xPos <= 5.0) {
				newI.yPos = in.yPos - 0.2;
				newI.direction = in.direction = 1;
				PlaySound("../fastinvader1.wav", NULL, SND_ASYNC | SND_FILENAME);
			}
			newI.xPos = in.xPos + (in.direction * invaderSpeed);
			invaders[i][j] = newI;
		}
	}
}

//----------------------
// MOTHERSHIP SETTINGS
//----------------------
bool mothershipActive = false;
float mothershipX = 0.0;
float mothershipY = 5.5;
float mothershipSpeed = 0.03;
mat4 mthModel = identity_mat4();

void spawnMothership() {
	if (!mothershipActive) {
		int chance = rand() % 10000;
		if (chance < 1) {
			PlaySound("../ufo_lowpitch.wav", NULL, SND_ASYNC | SND_FILENAME);
			mothershipActive = true;
			mothershipX = 350.0;
		}
	}
}

//----------------------
// BULLET SETTINGS
//----------------------
class Bullet {
public:
	float xPos = 0.0;  
	float yPos = 0.0;
	bool active = false;
	mat4 bModel = identity_mat4();
};

int maxNumberOfBullets = 1;
Bullet b1;
Bullet ammo[1] = {
	b1
};

//----------------------
// TEXTURE SETTINGS
//----------------------
GLuint tex;
GLuint textures_buffer[MODEL_COUNT];
char* TEX_NAMES[] = { "../playerShipT.png", "../invaderT.png", "../arenaT.jpg", "../img_test.bmp", "../mothershipT.jpg" };

void loadTextures() {
	int width, height;
	int i;
	for (i = 0; i < MODEL_COUNT; i++) {
		glGenTextures(1, &textures_buffer[i]);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textures_buffer[i]);
		glUniform1i(glGetUniformLocation(shaderProgramID, "ourTexture"), 0);
		unsigned char* image = SOIL_load_image(TEX_NAMES[i], &width, &height, 0, SOIL_LOAD_RGB);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		SOIL_free_image_data(image);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		printf("Loaded texture: %s Width: %i Height %i\n", TEX_NAMES[i], width, height);
	}
}

void fireBullet() {
	int i;
	for (i = 0; i < maxNumberOfBullets; i++) {
		if (!ammo[i].active) {
			ammo[i].active = true;
			ammo[i].xPos = cameraAlign;
			ammo[i].yPos = 0;
			PlaySound("../shoot.wav", NULL, SND_ASYNC | SND_FILENAME);
			return;
		}
	}
}

bool hitDetection(Bullet b) {
	int i, j;
	for (i = 0; i < invaderRow; i++) {
		for (j = 0; j < invaderColumn; j++) {
			float invaderY = invaders[i][j].yPos - 1.7;
			float invaderX = invaders[i][j].xPos;
			if (invaders[i][j].active &&
				invaderX < b.xPos + 4.0 &&
				invaderX > b.xPos - 4.0 &&
				invaderY < b.yPos + 0.5 &&
				invaderY > b.yPos - 0.5) {
				cout << "COLLISION! Hit alien i: " << i << " j: " << j << "\n";
				invaders[i][j].active = false;
				b.active = false;
				b.xPos = 0;
				b.yPos = 0;
				playerScore += (100 * scoreCombo);
				scoreCombo += 0.1;
				invaderSpeed += invaderAcc;
				PlaySound("../invaderkilled.wav", NULL, SND_ASYNC | SND_FILENAME );
				return true;
			}
		}
	}
	//Sleep(10);
	return false;
}

bool mothershipHitDetect(Bullet b) {
	if (mothershipActive &&
		mothershipX < b.xPos + 10.0 &&
		mothershipX > b.xPos - 10.0 &&
		mothershipY < b.yPos + 2.5 &&
		mothershipY > b.yPos - 2.5) {
		cout << "MOTHERSHIP HIT!\n";
		mothershipActive = false;
		b.active = false;
		b.xPos = 0;
		b.yPos = 0;
		playerScore += (1000 * scoreCombo);
		scoreCombo += 0.5;
		PlaySound("../invaderkilled.wav", NULL, SND_ASYNC | SND_FILENAME);
		return true;
	}
	return false;
}

Bullet bulletLogic(Bullet b) {
	Bullet newBull;
	if (b.yPos > 5) {
		newBull.active = false;
		newBull.xPos = 0;
		newBull.yPos = 0;
		scoreCombo = 1.0;
	} 
	else if (hitDetection(b) || mothershipHitDetect(b)) {
		newBull.active = false;
		newBull.xPos = 0;
		newBull.yPos = 0;
	}
	else {
		newBull.xPos = b.xPos;
		newBull.yPos = b.yPos + 0.01;
		newBull.active = true;
	}
	return newBull;
}

void gameLose() {
	if (!gameLoseSeen) {
		PlaySound("../explosion.wav", NULL, SND_ASYNC | SND_FILENAME);
		cout << "Game Over!\n";
		playingGame = false;
		enterIntoScoreboard(playerScore);
		char newTxt[256];
		sprintf(newTxt, "");
		update_text(score_id, newTxt);
		update_text(timer_id, newTxt);
		update_text(cursor_id, newTxt);

		sprintf(newTxt, "1. %i\n2. %i\n3. %i\n", scoreBoard[0], scoreBoard[1], scoreBoard[2]);
		update_text(scoreboard_id, newTxt);
		draw_texts();
		gameLoseSeen = true;
	}
	
}

void winLoseCheck() {
	int i, j;
	for (i = 0; i < invaderRow; i++) {
		for (j = 0; j < invaderColumn; j++) {
			float invaderY = invaders[i][j].yPos;
			if (invaders[i][j].active && invaders[i][j].yPos < 0) {
				gameLose();
			}
		}
	}
}

void loadFont() {
	if (!init_text_rendering(atlas_image, atlas_meta, width, height)) {
		fprintf(stderr, "ERROR init text rendering\n");
		exit(1);
	}

	score_id = add_text(
		"Score: 0 (x1.0)",
		-0.9f, -0.7f, 35.0f, 0.5f, 0.5f, 1.0f, 1.0f);

	timer_id = add_text(
		"120",
		0.5f, -0.7f, 35.0f, 0.5f, 0.5f, 1.0f, 1.0f);

	scoreboard_id = add_text(
		"",
		0.0f, 0.0f, 35.0f, 0.5f, 0.5f, 1.0f, 1.0f);

	cursor_id = add_text(
		"| |",
		-0.033f, 0.0f, 35.0f, 0.5f, 0.5f, 1.0f, 1.0f);
}

#pragma region MESH LOADING
/*----------------------------------------------------------------------------
                   MESH LOADING FUNCTION
  ----------------------------------------------------------------------------*/

bool load_mesh (const char* file_name, int count) {
  const aiScene* scene = aiImportFile (file_name, aiProcess_Triangulate); // TRIANGLES!
        fprintf (stderr, "ERROR: reading mesh %s\n", file_name);
  if (!scene) {
    fprintf (stderr, "ERROR: reading mesh %s\n", file_name);
    return false;
  }
  printf ("  %i animations\n", scene->mNumAnimations);
  printf ("  %i cameras\n", scene->mNumCameras);
  printf ("  %i lights\n", scene->mNumLights);
  printf ("  %i materials\n", scene->mNumMaterials);
  printf ("  %i meshes\n", scene->mNumMeshes);
  printf ("  %i textures\n", scene->mNumTextures);
  g_point_count[count] = 0;
  for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
    const aiMesh* mesh = scene->mMeshes[m_i];
    printf ("    %i vertices in mesh\n", mesh->mNumVertices);
    g_point_count[count] += mesh->mNumVertices;
    for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
      if (mesh->HasPositions ()) {
        const aiVector3D* vp = &(mesh->mVertices[v_i]);
        //printf ("      vp %i (%f,%f,%f)\n", v_i, vp->x, vp->y, vp->z);
        g_vp[count].push_back (vp->x);
        g_vp[count].push_back (vp->y);
        g_vp[count].push_back (vp->z);
      }
      if (mesh->HasNormals ()) {
        const aiVector3D* vn = &(mesh->mNormals[v_i]);
        //printf ("      vn %i (%f,%f,%f)\n", v_i, vn->x, vn->y, vn->z);
        g_vn[count].push_back (vn->x);
        g_vn[count].push_back (vn->y);
        g_vn[count].push_back (vn->z);
      }
      if (mesh->HasTextureCoords (0)) {
        const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
        printf ("      vt %i (%f,%f)\n", v_i, vt->x, vt->y);
        g_vt[count].push_back (vt->x);
        g_vt[count].push_back (vt->y);
      }
      if (mesh->HasTangentsAndBitangents ()) {
        // NB: could store/print tangents here
      }
    }
  }
  
  aiReleaseImport (scene);
  return true;
}

#pragma endregion MESH LOADING

// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS

// Create a NULL-terminated string by reading the provided file
char* readShaderSource(const char* shaderFile) {   
    FILE* fp = fopen(shaderFile, "rb"); //!->Why does binary flag "RB" work and not "R"... wierd msvc thing?

    if ( fp == NULL ) { return NULL; }

    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);

    fseek(fp, 0L, SEEK_SET);
    char* buf = new char[size + 1];
    fread(buf, 1, size, fp);
    buf[size] = '\0';

    fclose(fp);

    return buf;
}


static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
    GLuint ShaderObj = glCreateShader(ShaderType);

    if (ShaderObj == 0) {
        fprintf(stderr, "Error creating shader type %d\n", ShaderType);
        exit(0);
    }
	const char* pShaderSource = readShaderSource( pShaderText);

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
    glCompileShader(ShaderObj);
    GLint success;
	// check for shader related errors using glGetShaderiv
    glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
		Sleep(10000000);
		exit(1);
    }
	// Attach the compiled shader object to the program object
    glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders()
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
    shaderProgramID = glCreateProgram();
    if (shaderProgramID == 0) {
        fprintf(stderr, "Error creating shader program\n");
		Sleep(10000000);
        exit(1);
    }

	// Create two shader objects, one for the vertex, and one for the fragment shader
    AddShader(shaderProgramID, "../Shaders/simpleVertexShader.txt", GL_VERTEX_SHADER);
    AddShader(shaderProgramID, "../Shaders/fShader.txt", GL_FRAGMENT_SHADER);

    GLint Success = 0;
    GLchar ErrorLog[1024] = { 0 };
	// After compiling all shader objects and attaching them to the program, we can finally link it
    glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
		Sleep(10000000);
        exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
    glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
		Sleep(10000000);
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

void generateObjectBufferMesh() {
/*----------------------------------------------------------------------------
                   LOAD MESH HERE AND COPY INTO BUFFERS
  ----------------------------------------------------------------------------*/

	//Note: you may get an error "vector subscript out of range" if you are using this code for a mesh that doesnt have positions and normals
	//Might be an idea to do a check for that before generating and binding the buffer.
	int i = 0;
	for (i = 0; i < MODEL_COUNT; i++) {
		load_mesh(MESH_NAMES[i], i);
		unsigned int vp_vbo = i;
		loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
		loc2 = glGetAttribLocation(shaderProgramID, "vertex_normal");
		loc3 = glGetAttribLocation(shaderProgramID, "vertex_texture");

		glGenBuffers(1, &vp_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
		glBufferData(GL_ARRAY_BUFFER, g_point_count[i] * 3 * sizeof(float), &g_vp[i][0], GL_STATIC_DRAW);
		unsigned int vn_vbo = i;
		glGenBuffers(1, &vn_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
		glBufferData(GL_ARRAY_BUFFER, g_point_count[i] * 3 * sizeof(float), &g_vn[i][0], GL_STATIC_DRAW);

		//	This is for texture coordinates which you don't currently need, so I have commented it out
		unsigned int vt_vbo = 0;
		glGenBuffers (1, &vt_vbo);
		glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
		glBufferData (GL_ARRAY_BUFFER, g_point_count[i] * 2 * sizeof (float), &g_vt[i][0], GL_STATIC_DRAW);

		glGenVertexArrays(1, &(vaos[i]));
		glBindVertexArray(vaos[i]);

		glEnableVertexAttribArray(loc1);
		glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
		glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(loc2);
		glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
		glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		//	This is for texture coordinates which you don't currently need, so I have commented it out
		glEnableVertexAttribArray (loc3);
		glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
		glVertexAttribPointer (loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		printf("g_point: %i , %i \n", i, g_point_count[i]);
	}
	
}


#pragma endregion VBO_FUNCTIONS


void display(){
	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable (GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc (GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor (0.5f, 0.5f, 0.5f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram (shaderProgramID);


	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation (shaderProgramID, "model");
	int view_mat_location = glGetUniformLocation (shaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation (shaderProgramID, "proj");
	

	// Root of the Hierarchy
	mat4 view = cameraRotate * cameraTranslate;
	mat4 persp_proj = perspective(45.0, (float)width/(float)height, 0.1, 100.0);
	mat4 model = identity_mat4 ();

	// update uniforms & draw
	glUniformMatrix4fv (proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv (view_mat_location, 1, GL_FALSE, view.m);
	glUniformMatrix4fv (matrix_location, 1, GL_FALSE, model.m);

	// Draw SHIP
	glEnable(GL_TEXTURE_2D);
	glGenerateMipmap(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures_buffer[0]);
	glBindVertexArray(vaos[0]);
	model = rotate_y_deg(model, -90);
	model = rotate_x_deg(model, 30);
	model = scale(model, vec3(0.1, 0.1, 0.1));
	model = translate(model, vec3(0.0, 0.25, -2.0));
	model = rotate_y_deg(model, cameraAlign);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, model.m);
	glDrawArrays(GL_TRIANGLES, 0, g_point_count[0]);
	
	// Draw INVADERS
	glBindTexture(GL_TEXTURE_2D, textures_buffer[1]);
	if (playingGame) invadersMove();
	glBindVertexArray(vaos[1]);
	int i, j;
	for (i = 0; i < invaderRow; i++) {
		for (j = 0; j < invaderColumn; j++) {
			if (invaders[i][j].active) {
				mat4 v_model = identity_mat4();
				v_model = rotate_x_deg(v_model, 120);
				v_model = scale(v_model, vec3(0.05, 0.05, 0.05));
				v_model = translate(v_model, vec3(0.0, invaders[i][j].yPos, -5.0));
				v_model = rotate_y_deg(v_model, invaders[i][j].xPos);
				glUniformMatrix4fv(matrix_location, 1, GL_FALSE, v_model.m);
				glDrawArrays(GL_TRIANGLES, 0, g_point_count[1]);
			}
		}
	}


	// Draw ARENA
	glBindTexture(GL_TEXTURE_2D, textures_buffer[2]);
	glBindVertexArray(vaos[2]);
	model = translate(identity_mat4(), vec3(0.0, -0.5, 0.0));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, model.m);
	glDrawArrays(GL_TRIANGLES, 0, g_point_count[2]);

	// Draw BULLETS
	glBindTexture(GL_TEXTURE_2D, textures_buffer[3]);
	glBindVertexArray(vaos[3]);
	
	for (i = 0; i < maxNumberOfBullets && ammo[i].active; i++) {
		ammo[i] = bulletLogic(ammo[i]);
		//cout << "Bullet x: " << ammo[i].xPos << "\n";
		mat4 bModel = ammo[i].bModel;
		bModel = scale(bModel, vec3(0.2, 0.2, 0.2));
		bModel = translate(bModel, vec3(0.0, 2.0 + (ammo[i].yPos), -5.0));
		bModel = rotate_y_deg(bModel, ammo[i].xPos);
		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, bModel.m);
		glDrawArrays(GL_TRIANGLES, 0, g_point_count[3]);
	}

	// Draw MOTHERSHIP
	glBindTexture(GL_TEXTURE_2D, textures_buffer[4]);
	if (playingGame) spawnMothership();
	glBindVertexArray(vaos[4]);
	if (mothershipActive) {
		mthModel = scale(identity_mat4(), vec3(0.1, 0.1, 0.1));
		mthModel = rotate_x_deg(mthModel, 90);
		mthModel = translate(mthModel, vec3(0.0, mothershipY, -5.0));
		mthModel = rotate_y_deg(mthModel, mothershipX);

		if (playingGame) mothershipX -= mothershipSpeed;
		if (mothershipX < 10) {
			mothershipActive = false;
		}


		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, mthModel.m);
		glDrawArrays(GL_TRIANGLES, 0, g_point_count[4]);

		if (playingGame) miniRotation += 0.5;

		glBindVertexArray(vaos[1]);
		mat4 miniModel1 = identity_mat4();
		miniModel1 = scale(miniModel1, vec3(0.2, 0.2, 0.2));
		miniModel1 = translate(miniModel1, vec3(12.0, 0.0, 0.0));
		miniModel1 = rotate_y_deg(miniModel1, miniRotation);
		miniModel1 = mthModel * miniModel1;

		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, miniModel1.m);
		glDrawArrays(GL_TRIANGLES, 0, g_point_count[1]);

		mat4 miniModel2 = identity_mat4();
		miniModel2 = scale(miniModel2, vec3(0.2, 0.2, 0.2));
		miniModel2 = translate(miniModel2, vec3(12.0, 0.0, 0.0));
		miniModel2 = rotate_y_deg(miniModel2, miniRotation + 120);
		miniModel2 = mthModel * miniModel2;

		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, miniModel2.m);
		glDrawArrays(GL_TRIANGLES, 0, g_point_count[1]);

		mat4 miniModel3 = identity_mat4();
		miniModel3 = scale(miniModel3, vec3(0.2, 0.2, 0.2));
		miniModel3 = translate(miniModel3, vec3(12.0, 0.0, 0.0));
		miniModel3 = rotate_y_deg(miniModel3, miniRotation + 240);
		miniModel3 = mthModel * miniModel3;

		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, miniModel3.m);
		glDrawArrays(GL_TRIANGLES, 0, g_point_count[1]);
	}
	
	// Draw TEXT
	char newTxt[256];
	sprintf(newTxt, "Score: %d (x%.2f)\n", playerScore, scoreCombo);
	update_text(score_id, newTxt);

	if (playingGame) elapsed += 1.0 / 60.0;
	sprintf(newTxt, "%.2f\n", playertime - elapsed);
	update_text(timer_id, newTxt);

	draw_texts();

	if ((playertime - elapsed) <= 0) {
		elapsed = playertime;
		gameLose();
	}

	// Draw SKYBOX


    glutSwapBuffers();
}


void updateScene() {	

	// Placeholder code, if you want to work with framerate
	// Wait until at least 16ms passed since start of last frame (Effectively caps framerate at ~60fps)
	static DWORD  last_time = 0;
	DWORD  curr_time = timeGetTime();
	float  delta = (curr_time - last_time) * 0.001f;
	if (delta > 0.03f)
		delta = 0.03f;
	last_time = curr_time;

	// rotate the model slowly around the y axis
	rotate_y+=0.2f;
	// Draw the next frame
	glutPostRedisplay();
}


void init()
{
	initInvaders();
	// Set up the shaders
	GLuint shaderProgramID = CompileShaders();
	// load mesh into a vertex buffer array
	generateObjectBufferMesh();
	
	
}

// Placeholder code for the keypress
void keypress(unsigned char key, int x, int y) {
	if(key=='d' && playingGame){
		cameraRotate = rotate_x_deg(cameraRotate, 30);
		cameraRotate = rotate_y_deg(cameraRotate, 1.0);
		cameraRotate = rotate_x_deg(cameraRotate, -30);
		cameraAlign -= 1.0;
		if (cameraAlign == -1) {
			cameraAlign = 359.0;
		}
	}
	else if (key == 'a' && playingGame) {
		cameraRotate = rotate_x_deg(cameraRotate, 30);
		cameraRotate = rotate_y_deg(cameraRotate, -1.0);
		cameraRotate = rotate_x_deg(cameraRotate, -30);
		cameraAlign += 1.0;
		if (cameraAlign == 360.0) {
			cameraAlign = 0.0;
		}
	}
	else if (key == 'w' && playingGame) {
		cout << "FIRE at: ";
		fireBullet();
	}
	else if (key == 'r') {
		cout << "RESTART";
		playerScore = 0;
		scoreCombo = 1.0;
		elapsed = 0.0;
		playingGame = true;
		initInvaders();
		mothershipActive = false;
		char newTxt[256];
		sprintf(newTxt, "");
		update_text(scoreboard_id, newTxt);
		sprintf(newTxt, "| |");
		update_text(cursor_id, newTxt);
		gameLoseSeen = false;
		Sleep(1000);
	}
}

void mouseMove(int x, int y) {
	/*
	if (x < prevMouseX) {
		cameraRotate = rotate_y_deg(cameraRotate, 1.0);
		prevMouseX = x;
	}
	else if (x > prevMouseX) {
		cameraRotate = rotate_y_deg(cameraRotate, -1.0);
		prevMouseX = x;
	}

	if (y < prevMouseY) {
		cameraRotate = rotate_x_deg(cameraRotate, -1.0);
		prevMouseY = y;
	}
	else if (y > prevMouseY) {
		cameraRotate = rotate_x_deg(cameraRotate, 1.0);
		prevMouseY = y;
	}
	*/
}

int main(int argc, char** argv){
	srand(time(NULL));

	// Set up the window
	glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(width, height);
    glutCreateWindow("Hello Triangle");

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	glutKeyboardFunc(keypress);
	glutPassiveMotionFunc(mouseMove);

	// A call to glewInit() must be done after glut is initialized!
    GLenum res = glewInit();
	// Check for any errors
    if (res != GLEW_OK) {
      fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
      return 1;
    }
	// Set up your objects and shaders
	init();
	loadFont();
	loadTextures();
	// Begin infinite event loop
	glutMainLoop();
    return 0;
}











