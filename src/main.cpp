#include <iostream>
#include <cmath>

// GLEW
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// Other Libs
#include "../lib/SOIL2/SOIL2.h"
#include "../lib/stb_image.h"

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Project Includes
#include "shader.h"
#include "camera.h"
#include "model.h"

// Function prototypes
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mode);
void DoMovement();

// Window dimensions
int SCREEN_WIDTH, SCREEN_HEIGHT;

// Camera
Camera  camera(glm::vec3(0.0f, 0.0f, 3.0f));
GLfloat lastX = 0.0f;
GLfloat lastY = 0.0f;
bool keys[1024];

// Crosshair vertices
float crosshairVertices[] = {
	-0.05f,  0.05f, -0.1f,   0.0f, 1.0f,
	-0.05f, -0.05f, -0.1f,   0.0f, 0.0f,
	 0.05f, -0.05f, -0.1f,   1.0f, 0.0f,
	-0.05f,  0.05f, -0.1f,   0.0f, 1.0f,
	 0.05f, -0.05f, -0.1f,   1.0f, 0.0f,
	 0.05f,  0.05f, -0.1f,   1.0f, 1.0f
};

// Deltatime
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// Physics
GLfloat velocityY    = 0.0f;
GLfloat gravity      = -30.0f;
GLfloat jumpForce    = 12.0f;
GLfloat groundHeight = 1.0f;
bool    isGrounded   = true;

int main()
{
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWmonitor*       primaryMonitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode           = glfwGetVideoMode(primaryMonitor);
	glfwWindowHint(GLFW_DECORATED, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Duck Hunt 3D", nullptr, nullptr);
	if (!window) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwMakeContextCurrent(window);
	glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);

	glfwSetKeyCallback(window, KeyCallback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glewExperimental = GL_TRUE;
	if (GLEW_OK != glewInit()) {
		std::cout << "Failed to initialize GLEW" << std::endl;
		return EXIT_FAILURE;
	}

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glEnable(GL_DEPTH_TEST);

	// --- Crosshair VAO ---
	GLuint VAO, crosshairVBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &crosshairVBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// --- Shaders ---
	Shader lightingShader("shaders/default.vert", "shaders/default.frag");
	Shader hudShader("shaders/shader.vs", "shaders/shader.frag");

	// --- Models ---
	Model Skybox((char*)"assets/models/Skybox.obj");
	Model Piso((char*)"assets/models/piso.obj");
	Model Gun((char*)"assets/models/zapper.obj");

	// --- Crosshair texture ---
	GLuint crosshairTexture;
	glGenTextures(1, &crosshairTexture);
	glBindTexture(GL_TEXTURE_2D, crosshairTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	{
		int w, h, ch;
		stbi_set_flip_vertically_on_load(true);
		unsigned char* img = stbi_load("assets/textures/crosshair.png", &w, &h, &ch, STBI_rgb_alpha);
		if (img) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
			glGenerateMipmap(GL_TEXTURE_2D);
			stbi_image_free(img);
		} else {
			std::cout << "Failed to load crosshair texture" << std::endl;
		}
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	glm::mat4 projection = glm::perspective(
		glm::radians(camera.GetZoom()),
		(float)SCREEN_WIDTH / (float)SCREEN_HEIGHT,
		0.1f, 1000.0f
	);

	// ---------------------------------------------------------------
	// Cache uniform locations — glGetUniformLocation is a string lookup
	// and must NOT be called every frame.
	// ---------------------------------------------------------------
	struct {
		GLint projection, view, model, normalMatrix, viewPos;
		GLint transparency, useTexture, shininess;
		GLint dirDirection, dirAmbient, dirDiffuse, dirSpecular;
		GLint plAmbient[4], plDiffuse[4], plSpecular[4];
		GLint plConstant[4], plLinear[4], plQuadratic[4];
		GLint slAmbient, slDiffuse, slSpecular;
		GLint slConstant, slLinear, slQuadratic;
	} lu;

	lu.projection    = glGetUniformLocation(lightingShader.Program, "projection");
	lu.view          = glGetUniformLocation(lightingShader.Program, "view");
	lu.model         = glGetUniformLocation(lightingShader.Program, "model");
	lu.normalMatrix  = glGetUniformLocation(lightingShader.Program, "normalMatrix");
	lu.viewPos      = glGetUniformLocation(lightingShader.Program, "viewPos");
	lu.transparency = glGetUniformLocation(lightingShader.Program, "transparency");
	lu.useTexture   = glGetUniformLocation(lightingShader.Program, "useTexture");
	lu.shininess    = glGetUniformLocation(lightingShader.Program, "material.shininess");
	lu.dirDirection = glGetUniformLocation(lightingShader.Program, "dirLight.direction");
	lu.dirAmbient   = glGetUniformLocation(lightingShader.Program, "dirLight.ambient");
	lu.dirDiffuse   = glGetUniformLocation(lightingShader.Program, "dirLight.diffuse");
	lu.dirSpecular  = glGetUniformLocation(lightingShader.Program, "dirLight.specular");
	for (int i = 0; i < 4; i++) {
		std::string b = "pointLights[" + std::to_string(i) + "].";
		lu.plAmbient[i]   = glGetUniformLocation(lightingShader.Program, (b + "ambient").c_str());
		lu.plDiffuse[i]   = glGetUniformLocation(lightingShader.Program, (b + "diffuse").c_str());
		lu.plSpecular[i]  = glGetUniformLocation(lightingShader.Program, (b + "specular").c_str());
		lu.plConstant[i]  = glGetUniformLocation(lightingShader.Program, (b + "constant").c_str());
		lu.plLinear[i]    = glGetUniformLocation(lightingShader.Program, (b + "linear").c_str());
		lu.plQuadratic[i] = glGetUniformLocation(lightingShader.Program, (b + "quadratic").c_str());
	}
	lu.slAmbient   = glGetUniformLocation(lightingShader.Program, "spotLight.ambient");
	lu.slDiffuse   = glGetUniformLocation(lightingShader.Program, "spotLight.diffuse");
	lu.slSpecular  = glGetUniformLocation(lightingShader.Program, "spotLight.specular");
	lu.slConstant  = glGetUniformLocation(lightingShader.Program, "spotLight.constant");
	lu.slLinear    = glGetUniformLocation(lightingShader.Program, "spotLight.linear");
	lu.slQuadratic = glGetUniformLocation(lightingShader.Program, "spotLight.quadratic");

	struct {
		GLint projection, view, model, uvScale, texture;
	} hu;

	hu.projection = glGetUniformLocation(hudShader.Program, "projection");
	hu.view       = glGetUniformLocation(hudShader.Program, "view");
	hu.model      = glGetUniformLocation(hudShader.Program, "model");
	hu.uvScale    = glGetUniformLocation(hudShader.Program, "uvScale");
	hu.texture    = glGetUniformLocation(hudShader.Program, "ourTexture");

	// ---------------------------------------------------------------
	// Static uniforms — set once, never change during the game
	// ---------------------------------------------------------------
	lightingShader.Use();
	glUniformMatrix4fv(lu.projection, 1, GL_FALSE, glm::value_ptr(projection));
	glUniform1f(lu.shininess, 32.0f);

	// Sun (directional light)
	glUniform3f(lu.dirDirection, -0.4f, -1.0f, -0.5f);
	glUniform3f(lu.dirAmbient,    0.35f, 0.30f, 0.25f);
	glUniform3f(lu.dirDiffuse,    0.85f, 0.75f, 0.60f);
	glUniform3f(lu.dirSpecular,   0.4f,  0.4f,  0.4f);

	// Point lights disabled (outdoor scene)
	for (int i = 0; i < 4; i++) {
		glUniform3f(lu.plAmbient[i],   0.0f, 0.0f, 0.0f);
		glUniform3f(lu.plDiffuse[i],   0.0f, 0.0f, 0.0f);
		glUniform3f(lu.plSpecular[i],  0.0f, 0.0f, 0.0f);
		glUniform1f(lu.plConstant[i],  1.0f);
		glUniform1f(lu.plLinear[i],    0.0f);
		glUniform1f(lu.plQuadratic[i], 0.0f);
	}

	// Spotlight disabled
	glUniform3f(lu.slAmbient,   0.0f, 0.0f, 0.0f);
	glUniform3f(lu.slDiffuse,   0.0f, 0.0f, 0.0f);
	glUniform3f(lu.slSpecular,  0.0f, 0.0f, 0.0f);
	glUniform1f(lu.slConstant,  1.0f);
	glUniform1f(lu.slLinear,    0.0f);
	glUniform1f(lu.slQuadratic, 0.0f);

	// Seed mouse position
	{
		double initX, initY;
		glfwGetCursorPos(window, &initX, &initY);
		lastX = (GLfloat)initX;
		lastY = (GLfloat)initY;
	}

	// ---------------------------------------------------------------
	// Game loop
	// ---------------------------------------------------------------
	while (!glfwWindowShouldClose(window))
	{
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Physics
		velocityY += gravity * deltaTime;
		camera.position.y += velocityY * deltaTime;
		if (camera.position.y <= groundHeight) {
			camera.position.y = groundHeight;
			velocityY  = 0.0f;
			isGrounded = true;
		}

		// Input
		glfwPollEvents();
		DoMovement();

		{
			double rawX, rawY;
			glfwGetCursorPos(window, &rawX, &rawY);
			GLfloat xOffset = glm::clamp((GLfloat)(rawX - lastX), -30.0f, 30.0f);
			GLfloat yOffset = glm::clamp((GLfloat)(lastY - rawY), -30.0f, 30.0f);
			lastX = (GLfloat)rawX;
			lastY = (GLfloat)rawY;
			camera.ProcessMouseMovement(xOffset, yOffset);
		}

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// ---- World render (lightingShader) ----
		lightingShader.Use();
		glm::mat4 view = camera.GetViewMatrix();
		glUniformMatrix4fv(lu.view,   1, GL_FALSE, glm::value_ptr(view));
		glUniform3fv(lu.viewPos,      1, glm::value_ptr(camera.position));

		// Helper lambda: upload model + precomputed normal matrix, then draw
		auto drawWorld = [&](Model& m, const glm::mat4& mat, bool tex, bool transp) {
			glUniformMatrix4fv(lu.model, 1, GL_FALSE, glm::value_ptr(mat));
			glm::mat3 nm = glm::mat3(glm::transpose(glm::inverse(mat)));
			glUniformMatrix3fv(lu.normalMatrix, 1, GL_FALSE, glm::value_ptr(nm));
			glUniform1i(lu.useTexture,   tex   ? 1 : 0);
			glUniform1i(lu.transparency, transp ? 1 : 0);
			m.Draw(lightingShader);
		};

		// Skybox — strip translation from view so it stays "infinitely far"
		glDepthMask(GL_FALSE);
		glDisable(GL_CULL_FACE);
		glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
		glUniformMatrix4fv(lu.view, 1, GL_FALSE, glm::value_ptr(skyboxView));
		drawWorld(Skybox, glm::scale(glm::mat4(1.0f), glm::vec3(2.0f)), true, false);
		glDepthMask(GL_TRUE);
		glEnable(GL_CULL_FACE);

		// Restore full view for world objects
		glUniformMatrix4fv(lu.view, 1, GL_FALSE, glm::value_ptr(view));
		glUniform1i(lu.transparency, 1);

		// Floor
		glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(100.0f, 1.0f, 100.0f));
		drawWorld(Piso, model, true, true);

		// ---- HUD (hudShader, no depth test against world) ----
		glClear(GL_DEPTH_BUFFER_BIT);
		hudShader.Use();
		glm::mat4 hudView = glm::mat4(1.0f);
		glUniformMatrix4fv(hu.projection, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(hu.view,       1, GL_FALSE, glm::value_ptr(hudView));

		model = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, -0.8f, -1.0f));
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.3f));
		glUniformMatrix4fv(hu.model, 1, GL_FALSE, glm::value_ptr(model));
		glUniform1f(hu.uvScale, 1.0f);
		Gun.Draw(hudShader);

		// Crosshair (screen-space quad)
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glm::mat4 identity = glm::mat4(1.0f);
		glUniformMatrix4fv(hu.view,       1, GL_FALSE, glm::value_ptr(identity));
		glUniformMatrix4fv(hu.projection, 1, GL_FALSE, glm::value_ptr(identity));
		float ar = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
		model = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f / ar, 1.0f, 1.0f));
		glUniformMatrix4fv(hu.model, 1, GL_FALSE, glm::value_ptr(model));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, crosshairTexture);
		glUniform1i(hu.texture, 0);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return 0;
}

void DoMovement()
{
	if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN])
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT])
		camera.ProcessKeyboard(RIGHT, deltaTime);
	if (keys[GLFW_KEY_SPACE] && isGrounded) {
		velocityY  = jumpForce;
		isGrounded = false;
	}
}

void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key >= 0 && key < 1024) {
		if (action == GLFW_PRESS)   keys[key] = true;
		if (action == GLFW_RELEASE) keys[key] = false;
	}
}
