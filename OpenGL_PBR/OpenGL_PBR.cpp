#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION //Effectively turns header file into a cpp file
#include <STB/stb_image.h>
#include <assimp/config.h>
#include <map>

#include "Shader.h"
#include "Camera.h"
#include "Texture.h"
#include "Model.h"

using namespace std;
using namespace glm;

//Macros
#define VIEWPORTHEIGHT 720
#define VIEWPORTWIDTH 1280	

//Forward function declarations
/* Called Whenever user changes the size of the viewport*/
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
/* Handles player Input*/
void processInput(GLFWwindow* window);
/* Initialize OpenGL window as well as GLFW and glad libraries*/
void initWindow(GLFWwindow*& window);
/* Render polygon to screen */
void display(Shader shaderToUse, Model m);

void mouseCallback(GLFWwindow* window, double xPosition, double yPosition);

void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);

void loadHDRI(const char* path);

/* Bind cube to passed through VAO*/
void bindCubeToVAO(unsigned int& vao);

vec3 pointLightPositions[] = {
	vec3(0.7f,  0.2f,  2.0f),
	vec3(2.3f, -3.3f, -4.0f),
	vec3(-4.0f,  2.0f, -12.0f),
	vec3(0.0f,  0.0f, -3.0f)
};

vec3 pointLightColorss[] = {
	vec3(0.2f,  0.0f,  0.0f),
	vec3(0.0f, 1.0f, 0.0f),
	vec3(0.0f,  0.0f, 1.0f),
	vec3(1.0f,  0.0f, 1.0f)
};

// Vertex Array Objects for easy to access models
unsigned int cubeVAO; //3D cube vertices and texture coords
unsigned int vegetationVAO; //2D Square vertices and texture coords
unsigned int screenQuadVAO; //2D Square vertices that hold the final output
unsigned int skyboxVAO; // 3D cube vertices without texture coords

unsigned int textureColorbuffer; //Multi-sampled texture render buffer
unsigned int rbo; //Multi-sampled render buffer
unsigned int uboMatrices; //Holds matrices that are not changed througout the program

unique_ptr<Texture> grassTexture(new Texture()); //Holds texture for the window

unsigned int cubemapTexture; //Holds pre-mapped cubemap

//Model pointers
unique_ptr<Model> windowModel(new Model());
unique_ptr<Model> floorModel(new Model());
unique_ptr<Model> swordModel(new Model());
unique_ptr<Model> carModel(new Model());
unique_ptr<Model> samuraiSwordModel(new Model());

//Used with performance metrics
float deltaTime = 0.0f; //Time between current and last frame;
float lastFrame = 0.0f; //Time of last frame

//pointer to camera class
Camera* camera = new Camera(vec3(0.0, 0.0, 3.0), 45.f);

//Used with performance metrics
unsigned int fps = 0;
float frameTime = 0;
float cachedTime;
float delay = 1.f; //Time to delay for

//Shader class pointers
unique_ptr<Shader> PBRShader(new Shader());
unique_ptr<Shader> blurShader(new Shader());
unique_ptr<Shader> lightShader(new Shader());
unique_ptr<Shader> skyboxShader(new Shader());
unique_ptr<Shader> normalFaceShader(new Shader());
unique_ptr<Shader> newSkyboxShader(new Shader());
unique_ptr<Shader> convolutionShader(new Shader());
unique_ptr<Shader> filterShader(new Shader());
unique_ptr<Shader> BRDFshader(new Shader());
unique_ptr<Shader> shadowMapShader(new Shader());
unique_ptr<Shader> screenSpaceShader(new Shader());

map<float, vec3> sortedWindows; //Holds a sorted map of window positions so that they can be drawn in the correct order

//Framebuffers to convert multi-sample into single-sample
unsigned int framebuffer; //custom framebuffer delcaration
unsigned int intermediateFramebuffer; //custom framebuffer delcaration

unsigned int shadowMapFB; //shadow map framebuffer

unsigned int colorBuffer; //Normal output texture that gets passed to screen space quad

unsigned int depthCubemap; //Holds highest depth map values for shadow

unsigned int HDRIMap;  //Complete incoming texture before cubemapping

unsigned int captureFBO, captureRBO; //Frambuffers for converting HDRI to cubemap
unsigned int envCubemap; //Holds the converted HDRI into cubemap
unsigned int irradianceMap; //Holds a blurred version of the cubemap 

unsigned int prefilterMap; //Mip-Mapped cubemap for varying roughness values
unsigned int BRDFLUTtexture; //bidirectional reflectance distribution function which defined how light is reflected 
unsigned int bloomTexture; //Holds fragments which pass the bloom gate check
//Shadows
vector<mat4> shadowTransforms; //Holds direction vectors for each side of a point light 
const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024; //Resolution of the shadow map
//Cloest and furthest distance for shadows
float near = 1.0f;
float far = 25.f;
//Buffers for Guassian Blur implementation
unsigned int pingpongFBO[2];
unsigned int pingpongBuffers[2];

mat4 captureProjection; //Dictates the FOV of each cubemap face
vector<mat4> captureViews; //Holds direction vectors for each face of a cubemap
bool horizontal; //Whether Guassian Blur is moving vertically or horizontally

//Extraction functions
/* Call constructors for all shaders */
void SetupShaders();
//Setup Textures and load models that will be rendered
void SetupModels();
/* Setup and bind window meshes to VAO*/
void GenerateWindowVAO();
void SetupBlendedWindows();

/* Multisampled framebuffer for anti-aliasing*/
void GenerateMultisampledFramebuffer();
/* Convert Multisampled framebuffer to normal texture*/
void MultisampleToNormalFramebuffer();
/* Map incoming skybox textures to cubemap. Currently unused*/
void AssignSkyboxToCubeMap();
/* Allocate Uniform Buffer For view and projection matrices*/
void ReserveUniformBuffer();
void BindShadersToUniformBuffer();
/* Generate depth map framebuffer and store it inside a texture*/
void GenerateShadowMapFramebuffer();
/* Set up ping-pong to blur image in two directions*/
void SetupGuassianBlurFramebuffers();
/* Use a framebuffer to convert an incoming texture into a cubemap texture using trigonometry*/
void HDRItoCubemap();
/* Convert cubemap to a blur of colours which is then used to alter the base colour of a meshes fragment */
void SetupIrradianceMap();
/* Create lower resolution versions of the cubemap to be used with different incoming roughness values*/
void MipMapSkybox();
/* pre-filter the environment map to save on performance*/
void BRDFScene();
/* Slightly adjust the colour of a fragment in alternating directions for a given number of times*/
void GuassianBlurImplementation();
/* Render the scene and store the depth values in the shadow map framebuffer*/
void fillShadowBuffer(mat4 lightViewMatrix, Model* backpack);
/* Calculate and log the frames per second and frametime*/
void CalculatePerformanceMetrics();

int main() {

	//Initialize window and set it to main viewport
	GLFWwindow* window;
	initWindow(window);

	// Assign callback function for whenever the user adjusts the viewport size
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	//Bind Main Object cube to VAO
	bindCubeToVAO(cubeVAO);

	SetupShaders();

	cachedTime = glfwGetTime(); //Holds time of previous frame

	//Hide cursor and capture it inside the window
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	//Bind mouse function callback to mouseCallback()
	glfwSetCursorPosCallback(window, mouseCallback);

	//Scroll wheel callback
	glfwSetScrollCallback(window, scrollCallback);

	SetupModels();

	SetupBlendedWindows();

	GenerateMultisampledFramebuffer();

	MultisampleToNormalFramebuffer();

	//Assign incoming texture of the scene to the correct shader
	screenSpaceShader->use();
	screenSpaceShader->setInt("screenTexture", 0);

	AssignSkyboxToCubeMap();

	//Assign created skybox to skybox shader
	skyboxShader->use();
	skyboxShader->setInt("skybox", 0);

	ReserveUniformBuffer();

	GenerateShadowMapFramebuffer();

	SetupGuassianBlurFramebuffers();

	HDRItoCubemap();

	SetupIrradianceMap();

	//Setup irradiance map to main shader
	PBRShader->use();
	PBRShader->setInt("irradianceMap", 7);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);

	//Activate new texture to not accidentally affect the irradiance map
	glActiveTexture(GL_TEXTURE8);

	MipMapSkybox();

	BRDFScene();

	//Run the window until explicitly told to stop
	while (!glfwWindowShouldClose(window))  //Check if the window has been instructed to close
	{
		//calculate delta time
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window); //Process user inputs

		//Tell OpenGL to enable multisample buffers
		glEnable(GL_MULTISAMPLE);

		//glEnable(GL_FRAMEBUFFER_SRGB); //Using OpenGL's built in gamma correction sRGB tool

		//First render to shadow map

		glEnable(GL_DEPTH_TEST); //Tell OpenGL to use Z-Buffer
		glDepthFunc(GL_LEQUAL); //Needed for skybox otherwise it has z-conflict with normal background

		glEnable(GL_STENCIL_TEST); //Enable stencil testing to add outlines to lights

		glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE); //Decides what to do when a stencil buffer either passes or fails
		/* if the stencil test fails, do nothing. If the depth test fails, keep the stencil buffer object the same. This will
		* result in the outline staying as an outline when hidden behind other objects that are not in the buffer. If both the stencil
		* and depth test pass, then do the same as when the depth test fails except this time the original object will be in view.
		*/

		glEnable(GL_BLEND); //Allow for blending between colours with transparency
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO); //Only allow blending to affect alpha values

		glEnable(GL_PROGRAM_POINT_SIZE); //Vizualize vertex points

		//Clear colour buffer and depth buffer every frame before rendering
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		//Use orthographic projecton for shadows. 
		float near_plane = 1.0f, far_plane = 7.5f;
		mat4 lightProjection = ortho(-10.f, 10.f, -10.f, 10.f, near_plane, far_plane);
		//view matrix
		mat4 lightView = lookAt(vec3(1.2f, 1.0f, 2.0f), //Light position 
			vec3(0.0f, 0.0f, 0.0f), //center of scene
			vec3(0.0f, 1.0f, 0.0f)); //up vector
		mat4 lightViewMatrix = lightProjection * lightView;

		//cull front faces to deal with the peter panning effect
		glEnable(GL_CULL_FACE); //enable face culling
		glCullFace(GL_FRONT); //Cull front faces

		fillShadowBuffer(lightViewMatrix, samuraiSwordModel.get());
		
		//draw scene into offscreen frame buffer
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		//glBindTexture(GL_TEXTURE_2D, shadowMap);

		//Clear colour buffer and depth buffer every frame before rendering
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		glDisable(GL_CULL_FACE);
		glCullFace(GL_BACK); //Cull front faces
		
		//Draw scene as normal with PBR shader to "framebuffer" framebuffer
		PBRShader->use();
		PBRShader->setFloat("far_plane", far);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
		PBRShader->setInt("shadowMapCube", 6);
		display(*PBRShader, *samuraiSwordModel);

		//Blit multisampled frameburffer to default framebuffer seperating the colour attachments
		/* GL_COLOR_ATTACHMENT0 holds the normal output whereas GL_COLOR_ATTACHMENT1 holds fragments above a certain threshold for bloom*/
		glNamedFramebufferReadBuffer(framebuffer, GL_COLOR_ATTACHMENT0);
		glNamedFramebufferDrawBuffer(intermediateFramebuffer, GL_COLOR_ATTACHMENT0);
		glBlitNamedFramebuffer(framebuffer, intermediateFramebuffer, 0, 0, VIEWPORTWIDTH, VIEWPORTHEIGHT, 0, 0, VIEWPORTWIDTH, VIEWPORTHEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		glNamedFramebufferReadBuffer(framebuffer, GL_COLOR_ATTACHMENT1);
		glNamedFramebufferDrawBuffer(intermediateFramebuffer, GL_COLOR_ATTACHMENT1);
		glBlitNamedFramebuffer(framebuffer, intermediateFramebuffer, 0, 0, VIEWPORTWIDTH, VIEWPORTHEIGHT, 0, 0, VIEWPORTWIDTH, VIEWPORTHEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		GuassianBlurImplementation();

		//Go to default framebuffer and draw the final output texture to the viewport
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		screenSpaceShader->use(); //use screen space shader
		glClear(GL_COLOR_BUFFER_BIT); //clear color bit of original buffer
		glDisable(GL_DEPTH_TEST); //there will be no depth to destroy in 2D screen space
		glDisable(GL_CULL_FACE); //disable culling otherwise screenQuad will be automatically destroyed as it is too close to the viewer
		//render the quad to which the texture will be displayed
		glBindVertexArray(screenQuadVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, colorBuffer); //Holds the normal scene output with lighting calculations
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, pingpongBuffers[!horizontal]); //Holds the blurred bloom output
		screenSpaceShader->setInt("screenTexture", 0);
		screenSpaceShader->setInt("bloomBlur", 1);
		screenSpaceShader->setFloat("exposure", 1.0); //HDR exposure
		glDrawArrays(GL_TRIANGLES, 0, 6);

		CalculatePerformanceMetrics();

		glfwSwapBuffers(window); //Swap to the front/back buffer once the buffer has finished rendering
		glfwPollEvents(); //Check if any user events have been triggered

	}

	//Clean up GLFW resources as we now want to close the program
	glfwTerminate();
	return 0;
}

void initWindow(GLFWwindow*& window)
{
	//initialize GLFW 
	glfwInit();

	//Congifure GLFW window to use OpenGL 4.6
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	//Set OpenGL to core as opposed to compatibility mode
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//Tell GLFW we want to use a multisample buffer of 4 as opposed to the default 1
	glfwWindowHint(GLFW_SAMPLES, 4);

	//Create window object at the size of 800 x 600
	window = glfwCreateWindow(VIEWPORTWIDTH, VIEWPORTHEIGHT, "OpenGL_PBR", NULL, NULL);
	//Null check the window in case a parameter has failed
	if (window == NULL)
	{
		cout << " GLFW Failed to initialize" << endl;
		glfwTerminate();
		return;
	}

	//Set window to the current context in the thread
	glfwMakeContextCurrent(window);

	/*Check that GLAD has loaded properly using glfwGetProcAddress to pass through
	the correct path based on the current OS */
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		cout << "Failed to load GLAD" << endl;
		return;
	}

	glViewport(0, 0, VIEWPORTWIDTH, VIEWPORTHEIGHT);
}

void display(Shader shaderToUse, Model m)
{
	//Wireframe Mode
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	//Do not write to the stencil buffer for undesired objects (loaded models)
	glStencilMask(0x00); //0x00 just means that we cannot update the stencil buffer

	//Adjust uniform color value over time
	float systemTime = glfwGetTime();
	float greenValue = (sin(systemTime) / 2.0f) + 0.5f;

	mat4 view = mat4(1.0);
	mat4 projection = mat4(1.0);

	//model = rotate(model, (float)glfwGetTime() * radians(50.f), vec3(.5, 1.0, 0.0));
	projection = perspective(camera->GetFOV(), (float)VIEWPORTWIDTH / (float)VIEWPORTHEIGHT, 0.1f, 100.f);

	//Final parameter is just the up vector of the world in world space meaning it never changes
	view = camera->GetViewMatrix();

	//Basic Rendering
	shaderToUse.use();
	shaderToUse.setBool("bIsTransparent", false);
	//glUniform4f(vertexColorLocation, 0.0f, greenValue, 0.0f, 1.0f);
	glBindVertexArray(cubeVAO);

	for (int i = 0; i < 4; i++)
	{
		shaderToUse.setVec3("lightPositions[" + to_string(i) + "]", pointLightPositions[i]);
		shaderToUse.setVec3("lightColors[" + to_string(i) + "]", pointLightColorss[i]);
	}

	//adjust light colour over time
	vec3 lightColor = vec3(1.0, 1.0, 1.0);
	//lightColor.x = sin((glfwGetTime()) * 2.0f);
	//lightColor.y = sin((glfwGetTime()) * 0.7f);
	//lightColor.z = sin((glfwGetTime()) * 1.3f);

	vec3 diffuseColor = lightColor * vec3(0.8f);
	vec3 ambientColor = diffuseColor * vec3(0.7f);

	shaderToUse.setVec3("viewPos", camera->GetPosition());

	//Draw sword
	mat4 model = mat4(1.0);
	model = scale(model, vec3(3.0, 3.0, 3.0));
	model = rotate(model, (float)radians(90.f), vec3(1.0f, 0.0, 0.0));
	shaderToUse.setMat4("model", model);
	m.Draw(shaderToUse, 0);
	//Draw sheathe
	model = translate(model, vec3(0.2, 0.0, 0.0));
	shaderToUse.setMat4("model", model);
	m.Draw(shaderToUse, 1);

	//Draw Second Sword
	model = mat4(1.0);
	model = translate(model, vec3(-0.6, 0.0, 0.0));
	model = rotate(model, (float)radians(90.f), vec3(-1.0f, 0.0, 0.0));
	shaderToUse.setMat4("model", model);
	swordModel->Draw(shaderToUse);

	model = mat4(1.0);
	model = translate(model, vec3(2.0, -1.8, 0.0));
	model = scale(model, vec3(0.006, 0.006, 0.006));
	model = rotate(model, (float)radians(45.f), vec3(0.0f, -1.0, 0.0));
	shaderToUse.setMat4("model", model);
	carModel->Draw(shaderToUse);
	
	floorModel->Draw(shaderToUse, -1, true);

	//Draw sword again but this time with the geometry normal shader
	/*
	normalFaceShader->use();
	model = mat4(1.0);
	model = scale(model, vec3(3.0, 3.0, 3.0));
	model = rotate(model, (float)radians(90.f), vec3(1.0f, 0.0, 0.0));
	normalFaceShader->setMat4("model", model);
	m.Draw(*normalFaceShader, 0);
	model = translate(model, vec3(0.2, 0.0, 0.0));
	normalFaceShader->setMat4("model", model);
	m.Draw(*normalFaceShader, 1);
	*/

	//Use different shader for the source of the light
	lightShader->use();
	//Draw 4 cube lights at different positions with different colours
	for (unsigned int i = 0; i < 4; i++)
	{
		//Draw objects as normal but writing them to the stencil buffer

		//Add cubes to stencil buffer
		glStencilFunc(GL_ALWAYS, 1, 0xFF); //All stencils will pass the stencil test
		glStencilMask(0xFF); //enable writing to the stencil buffer

		//Scale light and move it in world space
		mat4 model(1.0f);
		model = translate(model, pointLightPositions[i]);
		model = scale(model, vec3(0.2f));
		lightShader->setMat4("model", model);
		//Set colour of the light object
		lightShader->setVec3("lightColor", pointLightColorss[i]);
		//call VAO that holds the buffer and draw it to the screen
		glBindVertexArray(cubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		//Draw scaled cube around object to act as an outline but not writing these to the stencil buffer
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF); //Pass if depth value is not equal to the stored depth
		glStencilMask(0x00); //disable writing to the stencil buffer
		glDisable(GL_DEPTH_TEST);
		// Set colour of outline
		lightShader->setVec3("lightColor", pointLightColorss[i] * vec3(0.5f)); //Get outline tint based on original colour
		//Increase size of the cube
		model = scale(model, vec3(1.2f));
		lightShader->setMat4("model", model);
		lightShader->use();
		//Draw cube
		glBindVertexArray(cubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		glStencilMask(0xFF);
		glStencilFunc(GL_ALWAYS, 0, 0xFF);
		glEnable(GL_DEPTH_TEST);
	}


	//Draw skybox as late as possible to minimize repeated calls
	newSkyboxShader->use();
	mat4 skyboxView = mat4(mat3(view));
	newSkyboxShader->setMat4("view", skyboxView);
	glBindVertexArray(skyboxVAO);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glDepthMask(GL_TRUE);

	//This fixes the skybox transparency issue but still results in repeated calls with the skybox
	//load multiple transparent grass
	shaderToUse.use();
	glBindVertexArray(vegetationVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, grassTexture->GetID());
	shaderToUse.setInt("grassTexture", 0);
	shaderToUse.setBool("bIsTransparent", true);
	//Loop through window position in the reverse order so that the furthest away windows are always drawn first
	for (map<float, vec3>::reverse_iterator it = sortedWindows.rbegin(); it != sortedWindows.rend(); ++it)
	{
		//This currently only works during loading, as the vector is never re-sorted during runtime, resulting in windows 
		//being in the wrong order once the player starts moving
		model = mat4(1.0f);
		model = translate(model, it->second);
		shaderToUse.setMat4("model", model);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		//This fixes the problem of windows not accounting for other windows that are behind them
	}
	shaderToUse.setBool("bIsTransparent", false);
}

void bindCubeToVAO(unsigned int& vao)
{
	//temporary place for object buffer data
	float Cubevertices[] = {
		// Back face
		   -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // Bottom-left
			0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
			0.5f, -0.5f, -0.5f,  1.0f, 0.0f, // bottom-right         
			0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
		   -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // bottom-left
		   -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // top-left
		   // Front face
		   -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
			0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-right
			0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // top-right
			0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // top-right
		   -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, // top-left
		   -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
		   // Left face
		   -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-right
		   -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-left
		   -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-left
		   -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-left
		   -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-right
		   -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-right
		   // Right face
			0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-left
			0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-right
			0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right         
			0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-right
			0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-left
			0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left     
			// Bottom face
			-0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // top-right
			 0.5f, -0.5f, -0.5f,  1.0f, 1.0f, // top-left
			 0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-left
			 0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-left
			-0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-right
			-0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // top-right
			// Top face
			-0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // top-left
			 0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // bottom-right
			 0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right     
			 0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // bottom-right
			-0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // top-left
			-0.5f,  0.5f,  0.5f,  0.0f, 0.0f  // bottom-left       
	};

	//Basic VAO 
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao); //Bind VAO to store any subsequent VBO & EBO calls

	//Basic VBO Setup for vertices
	unsigned int VBO;
	glGenBuffers(1, &VBO); //Create a single buffer for the VBO
	glBindBuffer(GL_ARRAY_BUFFER, VBO); // Bind Buffer to GL_ARRAY_BUFFER type
	glBufferData(GL_ARRAY_BUFFER, sizeof(Cubevertices), Cubevertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//storing texture coordinates into buffer
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	//normals
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);

	//Unbind VAO, EBO and VBO. MUST UNBIND VAO FIRST
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void scrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
	camera->AdjustFOV((float)yOffset, 45.0f);
	//Adjust Camera FOV inside the buffer
	mat4 projection = perspective(camera->GetFOV(), (float)VIEWPORTWIDTH / (float)VIEWPORTHEIGHT, 0.1f, 100.f);
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), value_ptr(projection));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void SetupShaders()
{
	//Bind light shader
	lightShader->LoadShader("shaders/lightShader.vert", "shaders/lightShader.frag");

	screenSpaceShader->LoadShader("shaders/screenSpaceShader.vert", "shaders/screenSpaceShader.frag");

	normalFaceShader->LoadShader("shaders/visibleNormals.vert", "shaders/visibleNormals.frag", "shaders/geometryShader.geom");

	shadowMapShader->LoadShader("shaders/shadowMap.vert", "shaders/shadowMap.frag", "shaders/shadowMap.geom");

	blurShader->LoadShader("shaders/gaussianBlur.vert", "shaders/gaussianBlur.frag");

	PBRShader->LoadShader("shaders/vertexShader.vert", "shaders/PBR.frag");

	loadHDRI("../textures/construction.hdr");

	newSkyboxShader->LoadShader("shaders/newSkybox.vert", "shaders/newSkybox.frag");

	convolutionShader->LoadShader("shaders/newSkybox.vert", "shaders/cubemapConvolution.frag");

	filterShader->LoadShader("shaders/newSkybox.vert", "shaders/preFilter.frag");

	BRDFshader->LoadShader("shaders/gaussianBlur.vert", "shaders/BRDF.frag");

	skyboxShader->LoadShader("shaders/skybox.vert", "shaders/skybox.frag");
}

void SetupModels()
{
	floorModel->setDiffuseDirectory("../textures/floor_diffuse.png");
	floorModel->bIsInstanced = true; //Tell Model class that this should be instanced
	floorModel->loadModel("../textures/floor.obj");

	swordModel->setDiffuseDirectory("../textures/chevaliar/textures/albedo.jpg");
	swordModel->setMetallicDirectory("../textures/chevaliar/textures/metallic.jpg");
	swordModel->setNormalDirectory("../textures/chevaliar/textures/normal.png");
	swordModel->setRoughnessDirectory("../textures/chevaliar/textures/roughness.jpg");
	swordModel->loadModel("../textures/chevaliar/model.dae");

	carModel->setDiffuseDirectory("../textures/carTextures/Vehicle_basecolor.png");
	carModel->setMetallicDirectory("../textures/carTextures/Vehicle_metallic.png");
	carModel->setNormalDirectory("../textures/carTextures/Vehicle_normal.png");
	carModel->setRoughnessDirectory("../textures/carTextures/Vehicle_roughness.png");
	carModel->setAODirectory("../textures/carTextures/Vehicle_ao.png");
	carModel->setEmissiveDirectory("../textures/carTextures/Vehicle_emissive.png");
	carModel->loadModel("../textures/cybercar.fbx");

	//load model textures
	samuraiSwordModel->setDiffuseDirectory("../textures/swordTextures/Albedo.png");
	samuraiSwordModel->setRoughnessDirectory("../textures/swordTextures/Roughness.png");
	samuraiSwordModel->setMetallicDirectory("../textures/swordTextures/Metallic.png");
	samuraiSwordModel->setNormalDirectory("../textures/swordTextures/Normal.png");
	//load model into buffers
	samuraiSwordModel->loadModel("../textures/Katana_export.fbx");
}

void SetupBlendedWindows()
{
	vector<vec3> vegetation; //Hold locations of the windows

	//Vertices and texture coordinates for each 2D square
	float transparentVertices[] = {
		// positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
		0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
		0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
		1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

		0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
		1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
		1.0f,  0.5f,  0.0f,  1.0f,  0.0f
	};

	//temporary grass locations
	vegetation.push_back(vec3(-1.5f, 0.0f, -0.48f));
	vegetation.push_back(vec3(1.5f, 0.0f, 0.51f));
	//vegetation.push_back(vec3(0.0f, 0.0f, 0.7f));
	vegetation.push_back(vec3(-0.3f, 0.0f, -2.3f));
	vegetation.push_back(vec3(0.5f, 0.0f, -0.6f));

	glGenVertexArrays(1, &vegetationVAO);
	glBindVertexArray(vegetationVAO); //Bind VAO to store any subsequent VBO & EBO calls

	unsigned int VBO;
	glGenBuffers(1, &VBO); //Create a single buffer for the VBO
	glBindBuffer(GL_ARRAY_BUFFER, VBO); // Bind Buffer to GL_ARRAY_BUFFER type

	glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//storing texture coordinates into buffer
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	grassTexture->LoadTexture("../textures/blending_transparent_window.png");

	//Sort locations into a sorted order form the location of the camera
	//NEED TO CHANGE:: CURRENTLY IS ONLY SET ONCE AT THE BEGINNING OF THE RENDER AND NEVER UPDATED. NEEDS MORE CHECKING AND PERFORMANCE
	//CONSIDERATIONS BEFORE PUTTING INSIDE THE RENDER LOOP AS MAP BECOMES A MEMORY HOG
	for (int i = 0; i < vegetation.size(); i++)
	{
		float distance = length(camera->GetPosition() - vegetation[i]); //get distance from camera to window position
		sortedWindows[distance] = vegetation[i];
	}
	//Generate VAO for easy access to objects
	GenerateWindowVAO();
}

void GenerateWindowVAO()
{
	float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
		/// positions   // texCoords
			-1.0f,  1.0f,  0.0f, 1.0f,
			-1.0f, -1.0f,  0.0f, 0.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,

			-1.0f,  1.0f,  0.0f, 1.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,
			 1.0f,  1.0f,  1.0f, 1.0f
	};
	//Generate VAO for screen space quad
	unsigned int quadVBO;
	glGenVertexArrays(1, &screenQuadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(screenQuadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GenerateMultisampledFramebuffer()
{
	//Create custom framebuffer
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	//Create attachment for framebuffer. Basically memory location buffer such as VAO

	//texture attachment 
	glGenTextures(1, &textureColorbuffer);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorbuffer);

	//allocate memory for texture but do not fill it. Also set texture to size of viewport
	//Using floating point lighitng values to exceed the LDR range
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA16F, VIEWPORTWIDTH, VIEWPORTHEIGHT, GL_TRUE);

	//attach color attachment to framebuffer from texture
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textureColorbuffer, 0);

	unsigned int bloomMTexture;
	glGenTextures(1, &bloomMTexture);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, bloomMTexture);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA16F, VIEWPORTWIDTH, VIEWPORTHEIGHT, GL_TRUE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D_MULTISAMPLE, bloomMTexture, 0);

	unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachments);

	//renderbuffer object attachment
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);

	//create depth and stencil renderbuffer object

	//Create render buffer with multisampling set to 4
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, VIEWPORTWIDTH, VIEWPORTHEIGHT);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	//Check that the current frambuffer checks the minimum requirements 
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete" << glCheckFramebufferStatus(GL_FRAMEBUFFER) << endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//End of framebuffer
}

void MultisampleToNormalFramebuffer()
{
	//Create intermediate framebuffer with only a color buffer
	glGenFramebuffers(1, &intermediateFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, intermediateFramebuffer);

	glGenTextures(1, &colorBuffer);
	glBindTexture(GL_TEXTURE_2D, colorBuffer);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, VIEWPORTWIDTH, VIEWPORTHEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);

	//Bind second texture for bloom
	glGenTextures(1, &bloomTexture);
	glBindTexture(GL_TEXTURE_2D, bloomTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, VIEWPORTWIDTH, VIEWPORTHEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, bloomTexture, 0);

	unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachments);

	glBindTexture(GL_TEXTURE_2D, 0);


	//Check that the current frambuffer checks the minimum requirements 
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		cout << "ERROR::FRAMEBUFFER:: Intermediate Framebuffer is not complete" << glCheckFramebufferStatus(GL_FRAMEBUFFER) << endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void AssignSkyboxToCubeMap()
{

	float skyboxVertices[] = {
		// positions          
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,


		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};

	//Create vector of cubemap locations in the order that OpenGL uses
	vector<string> textures_faces;
	textures_faces.push_back("../textures/anime-skybox/right.png");
	textures_faces.push_back("../textures/anime-skybox/left.png");
	textures_faces.push_back("../textures/anime-skybox/top.png");
	textures_faces.push_back("../textures/anime-skybox/bottom.png");
	textures_faces.push_back("../textures/anime-skybox/front.png");
	textures_faces.push_back("../textures/anime-skybox/back.png");

	//Create cubemap texture
	glGenTextures(1, &cubemapTexture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

	//Assign each texture location to its corresponding texture target
	int width, height, nrChannels;
	unsigned char* data;
	for (unsigned int i = 0; i < textures_faces.size(); i++)
	{
		data = stbi_load(textures_faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0,
				GL_RGBA, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << textures_faces[i] << std::endl;
			stbi_image_free(data);
		}
	}

	//assign cubemap texture parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	//Bind VAO to skybox vertices
	unsigned int skyboxVBO;
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	//Only using vertices for the skybox
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	//Unbind VAO and VBO
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void loadHDRI(const char* path)
{
	string filename = string(path);
	cout << filename << endl;
	stbi_set_flip_vertically_on_load(true);

	//unsigned int textureID;
	glGenTextures(1, &HDRIMap);

	int width, height, nrComponents;

	float* data = stbi_loadf(filename.c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{
		glBindTexture(GL_TEXTURE_2D, HDRIMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		cout << stbi_failure_reason() << endl;
		free(data);
	}
}

void ReserveUniformBuffer()
{
	//Create uniform buffer
	glGenBuffers(1, &uboMatrices);
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(mat4), NULL, GL_STATIC_DRAW); //128 Bytes equates to two full 4x4 matrices
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	//Bind uniform buffer object to binding point 0
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboMatrices, 0, 2 * sizeof(mat4));

	//Insert data into buffer
	mat4 projection = perspective(camera->GetFOV(), (float)VIEWPORTWIDTH / (float)VIEWPORTHEIGHT, 0.1f, 100.f);
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), value_ptr(projection));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	mat4 view = camera->GetViewMatrix();
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(mat4), sizeof(mat4), value_ptr(view));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	BindShadersToUniformBuffer();
}

void BindShadersToUniformBuffer()
{
	unsigned int matrices_index = 0;
	//Attach binding object to light and skybox shader
	lightShader->use();
	matrices_index = glGetUniformBlockIndex(lightShader->ID, "Matrices");
	glUniformBlockBinding(lightShader->ID, matrices_index, 0);

	skyboxShader->use();
	matrices_index = glGetUniformBlockIndex(skyboxShader->ID, "Matrices");
	glUniformBlockBinding(skyboxShader->ID, matrices_index, 0);

	normalFaceShader->use();
	matrices_index = glGetUniformBlockIndex(normalFaceShader->ID, "Matrices");
	glUniformBlockBinding(normalFaceShader->ID, matrices_index, 0);

	newSkyboxShader->use();
	matrices_index = glGetUniformBlockIndex(newSkyboxShader->ID, "Matrices");
	glUniformBlockBinding(newSkyboxShader->ID, matrices_index, 0);

}

void GenerateShadowMapFramebuffer()
{
	//Shadow map fraembuffer
	glGenFramebuffers(1, &shadowMapFB);

	unsigned int shadowMap;
	glGenTextures(1, &shadowMap);
	glBindTexture(GL_TEXTURE_2D, shadowMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
		GL_DEPTH_COMPONENT, GL_FLOAT, NULL); //Texture that only stores the depth value of each fragment
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFB);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
	//Tell OpenGL this framebuffer will not be used for any rendering
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//Omnidirectional shadows using a depth cubemap
	glGenTextures(1, &depthCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
	for (int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH,
			SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	//Override old framebuffer GL_DEPTH_ATTACHMENT value
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFB);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//Set direction for each cubemap plane
	float aspect = (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT;
	mat4 shadowProj = perspective(radians(90.f), aspect, near, far);

	shadowTransforms.push_back(shadowProj * lookAt(
		vec3(0.7f, 0.2f, 2.0f), vec3(0.7f, 0.2f, 2.0f) + vec3(1.0, 0.0, 0.0), vec3(0.0, -1.0, 0.0)));
	shadowTransforms.push_back(shadowProj * lookAt(
		vec3(0.7f, 0.2f, 2.0f), vec3(0.7f, 0.2f, 2.0f) + vec3(-1.0, 0.0, 0.0), vec3(0.0, -1.0, 0.0)));
	shadowTransforms.push_back(shadowProj * lookAt(
		vec3(0.7f, 0.2f, 2.0f), vec3(0.7f, 0.2f, 2.0f) + vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0)));
	shadowTransforms.push_back(shadowProj * lookAt(
		vec3(0.7f, 0.2f, 2.0f), vec3(0.7f, 0.2f, 2.0f) + vec3(0.0, -1.0, 0.0), vec3(0.0, 0.0, -1.0)));
	shadowTransforms.push_back(shadowProj * lookAt(
		vec3(0.7f, 0.2f, 2.0f), vec3(0.7f, 0.2f, 2.0f) + vec3(0.0, 0.0, 1.0), vec3(0.0, -1.0, 0.0)));
	shadowTransforms.push_back(shadowProj * lookAt(
		vec3(0.7f, 0.2f, 2.0f), vec3(0.7f, 0.2f, 2.0f) + vec3(0.0, 0.0, -1.0), vec3(0.0, -1.0, 0.0)));
}

void SetupGuassianBlurFramebuffers()
{
	glGenFramebuffers(2, pingpongFBO);
	glGenTextures(2, pingpongBuffers);
	for (int i = 0; i < 2; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
		glBindTexture(GL_TEXTURE_2D, pingpongBuffers[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, VIEWPORTWIDTH, VIEWPORTHEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongBuffers[i], 0);
	}
}

void HDRItoCubemap()
{
	//convert HDRI to cubemap
	glGenFramebuffers(1, &captureFBO);
	glGenRenderbuffers(1, &captureRBO);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512); //allocate storage to renderbuffer
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

	glGenTextures(1, &envCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	for (int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	//glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//capture 2D texture onto the cubemap faces
	captureProjection = perspective(radians(90.0), 1.0, 0.1, 10.0);
	//Each direction of the cube
	captureViews.push_back(lookAt(vec3(0.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), vec3(0.0, -1.0, 0.0)));
	captureViews.push_back(lookAt(vec3(0.0, 0.0, 0.0), vec3(-1.0, 0.0, 0.0), vec3(0.0, -1.0, 0.0)));
	captureViews.push_back(lookAt(vec3(0.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0)));
	captureViews.push_back(lookAt(vec3(0.0, 0.0, 0.0), vec3(0.0, -1.0, 0.0), vec3(0.0, 0.0, -1.0)));
	captureViews.push_back(lookAt(vec3(0.0, 0.0, 0.0), vec3(0.0, 0.0, 1.0), vec3(0.0, -1.0, 0.0)));
	captureViews.push_back(lookAt(vec3(0.0, 0.0, 0.0), vec3(0.0, 0.0, -1.0), vec3(0.0, -1.0, 0.0)));

	skyboxShader->use();
	skyboxShader->setInt("HDRImap", 0);
	skyboxShader->setMat4("projection", captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, HDRIMap);
	//Change viewport to size of incoming framebuffer
	glViewport(0, 0, 512, 512);
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	for (int i = 0; i < 6; ++i)
	{
		skyboxShader->setMat4("view", captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindVertexArray(skyboxVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//generate mipmap levels from first face
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

}

void SetupIrradianceMap()
{
	//Setup irradiance cubemap
	glGenTextures(1, &irradianceMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
	for (int i = 0; i < 6; ++i)
	{
		// Store irradiance map at a low resolution
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//Re-scale the render buffer to now accurately portray the new memory requirements
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);
	//render new cubemap to framebuffer
	convolutionShader->use();
	convolutionShader->setInt("skyboxMap", 0);
	convolutionShader->setMat4("projection", captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

	glViewport(0, 0, 32, 32);
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	for (int i = 0; i < 6; ++i)
	{
		convolutionShader->setMat4("view", captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindVertexArray(skyboxVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MipMapSkybox()
{
	//Pre-Filtering the HDR environment map
	glGenTextures(1, &prefilterMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
	for (int i = 0; i < 6; ++i)
	{
		//128 x 128 reflection resolution
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); //enable trilinear filtering
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	//Pre-Filter skybox into mipmap levels
	filterShader->use();
	filterShader->setInt("skyboxMap", 0);
	filterShader->setMat4("projection", captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	unsigned int maxMipLevels = 5;
	for (int mip = 0; mip < maxMipLevels; ++mip)
	{
		//resize framebuffer according to mip-level size
		unsigned int mipWidth = 128 * pow(0.5, mip);
		unsigned int mipHeight = 128 * pow(0.5, mip);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
		glViewport(0, 0, mipWidth, mipHeight);

		float roughness = (float)mip / (float)(maxMipLevels - 1);
		filterShader->setFloat("roughness", roughness);
		for (int i = 0; i < 6; ++i)
		{
			filterShader->setMat4("view", captureViews[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				prefilterMap, mip);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glBindVertexArray(skyboxVAO);
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//Filter cubemap faces to remove seams around the edges
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void BRDFScene()
{
	//Texture to store BRDF result
	glGenTextures(1, &BRDFLUTtexture);

	//pre-allocate memory for LUT texture
	glBindTexture(GL_TEXTURE_2D, BRDFLUTtexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//re-use framebuffer over screen-space quad
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, BRDFLUTtexture, 0);

	glViewport(0, 0, 512, 512);
	BRDFshader->use();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindVertexArray(screenQuadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	PBRShader->use();
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
	PBRShader->setInt("prefilterMap", 8);
	glBindTexture(GL_TEXTURE_2D, BRDFLUTtexture);
}

void GuassianBlurImplementation()
{
	//Gaussian Blur
	horizontal = true;
	bool first_iteration = true;
	int amount = 10;
	blurShader->use();
	for (int i = 0; i < amount; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
		blurShader->setInt("horizontal", horizontal);
		if (first_iteration)
		{
			glBindTexture(GL_TEXTURE_2D, bloomTexture);
			first_iteration = false;
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, pingpongBuffers[!horizontal]);
		}
		glBindVertexArray(screenQuadVAO);
		glDisable(GL_DEPTH_TEST);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		horizontal = !horizontal;
	}

}

void fillShadowBuffer(mat4 lightViewMatrix, Model* backpack)
{
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT); //Change viewport to size of shadow map
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFB);
	glClear(GL_DEPTH_BUFFER_BIT);
	shadowMapShader->use();
	//bind shadowTransforms vector to shader
	for (int i = 0; i < 6; ++i)
	{
		shadowMapShader->setMat4("shadowMatrices[" + to_string(i) + "]", shadowTransforms.at(i));
	}
	shadowMapShader->setFloat("far_plane", far);
	shadowMapShader->setVec3("lightPos", vec3(0.7f, 0.2f, 2.0f));
	shadowMapShader->setMat4("lightSpaceMatrix", lightViewMatrix);
	display(*shadowMapShader, *backpack);

	glViewport(0, 0, VIEWPORTWIDTH, VIEWPORTHEIGHT); //Reset viewport size
}

void CalculatePerformanceMetrics()
{
	//temp looping timer
	float now = glfwGetTime(); //Get time of current frame
	float delta = now - cachedTime; //difference between current and previous frame
	cachedTime = now; //Update cachedTime to hold new time

	//calculate frametime
	if (delta > frameTime) //Set frametime to the highest value
	{
		frameTime = delta;
		frameTime *= 1000; //Multiply by 1000 so we get the frametime in a much easir to read format
	}

	fps++;
	delay -= delta; //subtract difference from specified time
	if (delay <= 0.f) //Have we used up the alloted time
	{
		cout << fps << " fps" << endl;
		cout << "Frametime: " << frameTime << "ms" << endl;
		fps = 0; //Reset FPS counter
		delay = 1; //Reset timer
		frameTime = 0; //Reset framtime

	}
}

void mouseCallback(GLFWwindow* window, double xPosition, double yPosition)
{
	camera->CalculateMouseAdjustment(xPosition, yPosition);
	//adjust view matrix inside the uniform buffer
	mat4 view = camera->GetViewMatrix();
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(mat4), sizeof(mat4), value_ptr(view));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}

	//call KeyboardMovement for basic movement on the camera
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		camera->KeyboardMovement(EMovementDirection::FORWARD, deltaTime, uboMatrices);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		camera->KeyboardMovement(EMovementDirection::BACKWARD, deltaTime, uboMatrices);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		camera->KeyboardMovement(EMovementDirection::LEFT, deltaTime, uboMatrices);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		camera->KeyboardMovement(EMovementDirection::RIGHT, deltaTime, uboMatrices);
	}
}

/* Called whenever the user resizes the window containing the viewport*/
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
}