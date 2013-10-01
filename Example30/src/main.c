/**
 * OpenGL 4 - Example 30
 *
 * @author	Norbert Nopper norbert@nopper.tv
 *
 * Homepage: http://nopper.tv
 *
 * Copyright Norbert Nopper
 */

#include <stdio.h>

#include "GL/glus.h"

#define WIDTH 640

#define HEIGHT 480

#define DIRECTIONS_PADDING 1

// 2^5-1
#define STACK_DEPTH (2 * 2 * 2 * 2 * 2 - 1)

#define RAY_STACK_LENGTH (4 + 3 + 1)

#define NUM_SPHERES 6
#define NUM_LIGHTS 1

#define PADDING -321.123f

/**
 * The used shader program.
 */
static GLUSshaderprogram g_program;

/**
 * The location of the texture in the shader program.
 */
static GLint g_textureLocation;

/**
 * The empty VAO.
 */
static GLuint g_vao;

/**
 * The texture.
 */
static GLuint g_texture;

/**
 * Width of the image.
 */
static GLuint g_imageWidth = 640;

/**
 * Height of the image.
 */
static GLuint g_imageHeight = 480;

/**
 * Size of the work goups.
 */
static GLuint g_localSize = 16;

//

/**
 * The used compute shader program.
 */
static GLUSshaderprogram g_computeProgram;

//
// Shader Storage Buffer Objects
//

static GLuint g_directionSSBO;

static GLfloat g_directionBuffer[WIDTH * HEIGHT * (3 + DIRECTIONS_PADDING)];

//

static GLuint g_positionSSBO;

static GLfloat g_positionBuffer[WIDTH * HEIGHT * 4];

//

static GLuint g_rayStackSSBO;

static GLfloat g_rayStackBuffer[WIDTH * HEIGHT * RAY_STACK_LENGTH * STACK_DEPTH];

//
//
//

typedef struct _Material
{
	GLfloat emissiveColor[4];

	GLfloat diffuseColor[4];

	GLfloat specularColor[4];

	GLfloat shininess;

	GLfloat alpha;

	GLfloat reflectivity;

	GLfloat padding;

} Material;

typedef struct _Sphere
{
	GLfloat center[4];

	GLfloat radius;

	GLfloat padding[3];

	Material material;

} Sphere;

typedef struct _PointLight
{
	GLfloat position[4];

	GLfloat color[4];

} PointLight;

//

static GLuint g_spheresSSBO;

Sphere g_allSpheres[NUM_SPHERES] = {
		// Ground sphere
		{ { 0.0f, -10001.0f, -20.0f, 1.0f }, 10000.0f, {PADDING, PADDING, PADDING}, { { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.4f, 0.4f, 0.4f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f }, 0.0f, 1.0f, 0.0f, PADDING } },
		// Transparent sphere
		{ { 0.0f, 0.0f, -10.0f, 1.0f }, 1.0f, {PADDING, PADDING, PADDING}, { { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.8f, 0.8f, 0.8f, 1.0f }, { 0.8f, 0.8f, 0.8f, 1.0f }, 20.0f, 0.2f, 1.0f, PADDING } },
		// Reflective sphere
		{ { 1.0f, -0.75f, -7.0f, 1.0f }, 0.25f, {PADDING, PADDING, PADDING}, { { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.8f, 0.8f, 0.8f, 1.0f }, { 0.8f, 0.8f, 0.8f, 1.0f }, 20.0f, 1.0f, 0.8f, PADDING } },
		// Blue sphere
		{ { 2.0f, 1.0f, -16.0f, 1.0f }, 2.0f, {PADDING, PADDING, PADDING}, { { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.8f, 1.0f }, { 0.8f, 0.8f, 0.8f, 1.0f }, 20.0f, 1.0f, 0.2f, PADDING } },
		// Green sphere
		{ { -2.0f, 0.25f, -6.0f, 1.0f }, 1.25f, {PADDING, PADDING, PADDING}, { { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.8f, 0.0f, 1.0f }, { 0.8f, 0.8f, 0.8f, 1.0f }, 20.0f, 1.0f, 0.2f, PADDING } },
		// Red sphere
		{ { 3.0f, 0.0f, -8.0f, 1.0f }, 1.0f, {PADDING, PADDING, PADDING}, { { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.8f, 0.0f, 0.0f, 1.0f }, { 0.8f, 0.8f, 0.8f, 1.0f }, 20.0f, 1.0f, 0.2f, PADDING } }
};

//

static GLuint g_pointLightsSSBO;

PointLight g_allLights[NUM_LIGHTS] = {
		{{0.0f, 5.0f, -5.0f, 1.0f}, { 1.0f, 1.0f, 1.0f, 1.0f }}
};

//

static GLuint g_colorSSBO;

static GLfloat g_colorBuffer[WIDTH * HEIGHT * 4 * STACK_DEPTH];

GLUSboolean init(GLUSvoid)
{
	GLint i;

	GLUStextfile vertexSource;
	GLUStextfile fragmentSource;
	GLUStextfile computeSource;

	glusLoadTextFile("../Example30/shader/fullscreen.vert.glsl", &vertexSource);
	glusLoadTextFile("../Example30/shader/texture.frag.glsl", &fragmentSource);

	glusBuildProgramFromSource(&g_program, (const GLchar**)&vertexSource.text, 0, 0, 0, (const GLchar**)&fragmentSource.text);

	glusDestroyTextFile(&vertexSource);
	glusDestroyTextFile(&fragmentSource);

	glusLoadTextFile("../Example30/shader/raytrace.comp.glsl", &computeSource);

	glusBuildComputeProgramFromSource(&g_computeProgram, (const GLchar**)&computeSource.text);

	glusDestroyTextFile(&computeSource);

	//

	// Retrieve the uniform locations in the program.
	g_textureLocation = glGetUniformLocation(g_program.program, "u_texture");

	//

	// Generate and bind a texture.
	glGenTextures(1, &g_texture);
	glBindTexture(GL_TEXTURE_2D, g_texture);

	// Create an empty image.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, g_imageWidth, g_imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	// Setting the texture parameters.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindTexture(GL_TEXTURE_2D, 0);

	//
	//

	glUseProgram(g_program.program);

	glGenVertexArrays(1, &g_vao);
	glBindVertexArray(g_vao);

	//

	// Also bind created texture ...
	glBindTexture(GL_TEXTURE_2D, g_texture);

	// ... and bind this texture as an image, as we will write to it. see binding = 0 in shader.
	glBindImageTexture(0, g_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

	// ... and as this is texture number 0, bind the uniform to the program.
	glUniform1i(g_textureLocation, 0);

	//
	//

	glUseProgram(g_computeProgram.program);

	//

	printf("Preparing buffers ... ");

	// Generate the ray directions depending on FOV, width and height.
	if (!glusRaytracePerspectivef(g_directionBuffer, DIRECTIONS_PADDING, 30.0f, WIDTH, HEIGHT))
	{
		printf("failed!\n");

		printf("Error: Could not create direction buffer.\n");

		return GLUS_FALSE;
	}

	// Compute shader will use these textures just for input.
	glusRaytraceLookAtf(g_positionBuffer, g_directionBuffer, g_directionBuffer, DIRECTIONS_PADDING, WIDTH, HEIGHT, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f);

	for (i = 0; i < WIDTH * HEIGHT * RAY_STACK_LENGTH * STACK_DEPTH; i++)
	{
		g_rayStackBuffer[i] = 0.0f;
	}

	for (i = 0; i < WIDTH * HEIGHT * STACK_DEPTH; i++)
	{
		g_colorBuffer[i * 4 + 0] = 0.0f;
		g_colorBuffer[i * 4 + 1] = 0.0f;
		g_colorBuffer[i * 4 + 2] = 0.0f;
		g_colorBuffer[i * 4 + 3] = 1.0f;
	}

	printf("done!\n");

	//
	// Buffers with the initial ray position and direction.
	//

	glGenBuffers(1, &g_directionSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_directionSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, WIDTH * HEIGHT * (3 + DIRECTIONS_PADDING) * sizeof(GLfloat), g_directionBuffer, GL_STATIC_DRAW);
	// see binding = 1 in the shader
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, g_directionSSBO);

	//

	glGenBuffers(1, &g_positionSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_positionSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, WIDTH * HEIGHT * 4 * sizeof(GLfloat), g_positionBuffer, GL_STATIC_DRAW);
	// see binding = 2 in the shader
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, g_positionSSBO);

	//

	glGenBuffers(1, &g_rayStackSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_rayStackSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, WIDTH * HEIGHT * RAY_STACK_LENGTH * STACK_DEPTH * sizeof(GLfloat), g_rayStackBuffer, GL_STATIC_DRAW);
	// see binding = 3 in the shader
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, g_rayStackSSBO);

	//

	glGenBuffers(1, &g_spheresSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_spheresSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_SPHERES * sizeof(Sphere), g_allSpheres, GL_STATIC_DRAW);
	// see binding = 4 in the shader
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, g_spheresSSBO);

	//

	glGenBuffers(1, &g_pointLightsSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_pointLightsSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_LIGHTS * sizeof(PointLight), g_allSpheres, GL_STATIC_DRAW);
	// see binding = 5 in the shader
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, g_pointLightsSSBO);

	//

	glGenBuffers(1, &g_colorSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_colorSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, WIDTH * HEIGHT * 4 * STACK_DEPTH * sizeof(GLfloat), g_colorBuffer, GL_STATIC_DRAW);
	// see binding = 6 in the shader
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, g_colorSSBO);

	return GLUS_TRUE;
}

GLUSvoid reshape(GLUSint width, GLUSint height)
{
	glViewport(0, 0, width, height);
}

GLUSboolean update(GLUSfloat time)
{
	// Switch to the compute shader.
	glUseProgram(g_computeProgram.program);

	// Create threads depending on width, height and block size. In this case we have 1200 threads.
	glDispatchCompute(g_imageWidth / g_localSize, g_imageHeight / g_localSize, 1);

	// Switch back to the render program.
	glUseProgram(g_program.program);

	// Here we full screen the output of the compute shader.
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	return GLUS_TRUE;
}

GLUSvoid terminate(GLUSvoid)
{
	glBindTexture(GL_TEXTURE_2D, 0);

	if (g_texture)
	{
		glDeleteTextures(1, &g_texture);

		g_texture = 0;
	}

	//

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	if (g_directionSSBO)
	{
		glDeleteBuffers(1, &g_directionSSBO);

		g_directionSSBO = 0;
	}

	if (g_positionSSBO)
	{
		glDeleteBuffers(1, &g_positionSSBO);

		g_positionSSBO = 0;
	}

	if (g_rayStackSSBO)
	{
		glDeleteBuffers(1, &g_rayStackSSBO);

		g_rayStackSSBO = 0;
	}

	if (g_spheresSSBO)
	{
		glDeleteBuffers(1, &g_spheresSSBO);

		g_spheresSSBO = 0;
	}

	if (g_pointLightsSSBO)
	{
		glDeleteBuffers(1, &g_pointLightsSSBO);

		g_pointLightsSSBO = 0;
	}

	if (g_colorSSBO)
	{
		glDeleteBuffers(1, &g_colorSSBO);

		g_colorSSBO = 0;
	}

	//

	glBindVertexArray(0);

	if (g_vao)
	{
		glDeleteVertexArrays(1, &g_vao);

		g_vao = 0;
	}

	glUseProgram(0);

	glusDestroyProgram(&g_program);

	glusDestroyProgram(&g_computeProgram);
}

int main(int argc, char* argv[])
{
	glusInitFunc(init);

	glusReshapeFunc(reshape);

	glusUpdateFunc(update);

	glusTerminateFunc(terminate);

	glusPrepareContext(4, 3, GLUS_FORWARD_COMPATIBLE_BIT);

	// Again, makes programming for this example easier.
	glusPrepareNoResize(GLUS_TRUE);

	if (!glusCreateWindow("GLUS Example Window", WIDTH, HEIGHT, 0, 0, GLUS_FALSE))
	{
		printf("Could not create window!\n");
		return -1;
	}

	glusRun();

	return 0;
}
