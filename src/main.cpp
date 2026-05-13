#include <iostream>
#include <cmath>
#include <vector>
#include <array>

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
#include "modelAnim.h"

// Function prototypes
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mode);
void DoMovement();

// Window dimensions
int SCREEN_WIDTH, SCREEN_HEIGHT;

// Camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
bool keys[1024];

// Mouse look — accumulated per-event deltas from the cursor callback.
// Using a callback (not glfwGetCursorPos polling) is required on WSL2 because
// GLFW's internal center-warp for CURSOR_DISABLED mode happens before polling
// returns, making the polled position ≈ center every frame (joystick effect).
// The callback fires for real user motion only; GLFW suppresses warp-triggered events.
double mouseAccX      = 0.0;
double mouseAccY      = 0.0;
double mousePrevX     = 0.0;
double mousePrevY     = 0.0;
bool   mouseFirstMove = true;

// ---- Sistema de patos ----
const int NUM_PATOS = 3;
enum EstadoPato { VOLANDO, CAYENDO, MUERTO };
struct Pato {
	float angulo;         // Ángulo actual en la órbita (radianes)
	float velocidad;      // Velocidad angular (rad/segundo)
	float altura;         // Altura Y de vuelo
	EstadoPato estado;
	float velocidadCaida; // Para la física de caída libre
};
const float RADIO_ORBITA = 20.0f; // Radio del anillo de pasto
const float ALTURA_VUELO =  6.0f;
std::array<Pato, NUM_PATOS> patos;

// ---- Sistema de disparo ----
int patosDerribados = 0;   // Contador global de patos eliminados
int patosEscapados  = 0;   // Contador de patos que llegaron a escapar
bool disparoRealizado = false; // Flag para el callback del mouse

// ---- Sistema del perro ----
enum EstadoPerro { PERRO_BUSCANDO, PERRO_ENCONTRADO, PERRO_MOSTRANDO, PERRO_RIENDO };
EstadoPerro estadoPerro = PERRO_BUSCANDO;
float timerPerro        = 0.0f;  
float animTime          = 0.0f; // Para funciones seno/coseno
int   patosEnRonda      = 0;     

// Variables de animación jerárquica
float articulacionPatas = 0.0f;
float articulacionCola  = 0.0f;
float articulacionCabeza = 0.0f;
float articulacionCuerpoY = 0.0f;

glm::vec3 posicionActualPerro(0.0f, -1.0f, 15.0f);
glm::vec3 posImpactoPato(0.0f); 
float rotacionPerro = 180.0f;
bool camaraCenital = false; // false = FPS, true = cenital

// ---- Efectos de disparo ----
float recoilTimer = 0.0f;
float flashTimer  = 0.0f;
const float RECOIL_DURATION = 0.15f;
const float FLASH_DURATION  = 0.08f;

// ---- Efecto de plumas ----
struct EfectoPlumas {
	bool activo;
	glm::vec3 posicion;
	float timer;
	int   frame;       // 0-4, ciclo de los 5 modelos de plumas
};
EfectoPlumas efectoPlumas = { false, glm::vec3(0.0f), 0.0f, 0 };
const float DURACION_PLUMAS = 0.6f;

// Forward declarations
void perroReaccionarDisparo(glm::vec3 posImpacto);

void CursorPosCallback(GLFWwindow*, double x, double y) {
    if (mouseFirstMove) {
        mousePrevX     = x;
        mousePrevY     = y;
        mouseFirstMove = false;
        return;
    }
    mouseAccX += x - mousePrevX;
    mouseAccY += y - mousePrevY;
    mousePrevX = x;
    mousePrevY = y;
}

// Callback invocado por GLFW al hacer click con el mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		disparoRealizado = true;
	}
}

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
	glfwSetCursorPosCallback(window, CursorPosCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
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
	Shader animShader("shaders/anim.vs", "shaders/anim.frag");

	// --- Models ---
	Model Skybox((char*)"assets/models/Skybox.obj");
	Model Piso((char*)"assets/models/piso.obj");
	Model Gun((char*)"assets/models/zapper.obj");

	Model Arbusto((char*)"assets/models/Bush.glb");
	Model Nube1((char*)"assets/models/Nube_1.glb");
	Model Nube2((char*)"assets/models/Nube_2.glb");
	Model Nube3((char*)"assets/models/Nube_3.glb");
	Model PastoAnillo((char*)"assets/models/E_Grass.glb");
	Model PastoAnilloRev((char*)"assets/models/E_GrassBackwards.glb");

	// Jerarquía del perro
	Model DogBody((char*)"assets/models/dog/DogBody.obj");
	Model DogHead((char*)"assets/models/dog/HeadDog.obj");
	Model DogTail((char*)"assets/models/dog/TailDog.obj");
	Model DogLegFL((char*)"assets/models/dog/F_LeftLegDog.obj");
	Model DogLegFR((char*)"assets/models/dog/F_RightLegDog.obj");
	Model DogLegBL((char*)"assets/models/dog/B_LeftLegDog.obj");
	Model DogLegBR((char*)"assets/models/dog/B_RightLegDog.obj");

	// Modelo del cazador (jugador visible en cenital)
	Model Cazador((char*)"assets/models/RedDog.obj");

	// Efecto de plumas
	Model Plumas1((char*)"assets/models/feathers_1.glb");
	Model Plumas2((char*)"assets/models/feathers_2.glb");
	Model Plumas3((char*)"assets/models/feathers_3.glb");
	Model Plumas4((char*)"assets/models/feathers_4.glb");
	Model Plumas5((char*)"assets/models/feathers_5.glb");
	std::array<Model*, 5> framesPlumas = { &Plumas1, &Plumas2, &Plumas3, &Plumas4, &Plumas5 };

	// --- Precomputar matrices de vegetación estática ---
	struct InstanceData {
		glm::mat4 model;
		glm::mat3 normal;
		int type; // 0 para normal, 1 para rev (solo para pasto)
	};

	const int NUM_ARBUSTOS = 8;
	std::array<InstanceData, NUM_ARBUSTOS> arbustosData;
	for (int i = 0; i < NUM_ARBUSTOS; i++) {
		float ang = glm::radians(360.0f / NUM_ARBUSTOS * i);
		glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(22.0f * cos(ang), -0.5f, 22.0f * sin(ang)));
		m = glm::rotate(m, glm::radians(45.0f * i), glm::vec3(0.0f, 1.0f, 0.0f));
		m = glm::scale(m, glm::vec3(2.5f));
		arbustosData[i].model  = m;
		arbustosData[i].normal = glm::mat3(glm::transpose(glm::inverse(m)));
	}

	const int SEGMENTOS_PASTO = 12;
	std::array<InstanceData, SEGMENTOS_PASTO> pastoData;
	for (int i = 0; i < SEGMENTOS_PASTO; i++) {
		float ang = glm::radians(360.0f / SEGMENTOS_PASTO * i);
		glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(18.0f * cos(ang), -1.0f, 18.0f * sin(ang)));
		m = glm::rotate(m, ang, glm::vec3(0.0f, 1.0f, 0.0f));
		m = glm::scale(m, glm::vec3(3.0f));
		pastoData[i].model  = m;
		pastoData[i].normal = glm::mat3(glm::transpose(glm::inverse(m)));
		pastoData[i].type   = i % 2;
	}

	std::array<glm::vec3, 3> nubesBasePos = {{
		{ 15.0f, 18.0f,  5.0f},
		{-10.0f, 20.0f, 12.0f},
		{  5.0f, 16.0f,-15.0f}
	}};

	std::cout << ">>> Cargando Pato..." << std::endl;
	ModelAnim PatoAnimado("assets/models/Pato.fbx");
	PatoAnimado.initShaders(animShader.Program);
	std::cout << ">>> Pato listo" << std::endl;

	// Distribuir los 3 patos equitativamente en la órbita
	for (int i = 0; i < NUM_PATOS; i++) {
		patos[i].angulo         = glm::radians(120.0f * i); // 0°, 120°, 240°
		patos[i].velocidad      = 0.6f + i * 0.05f;
		patos[i].altura         = ALTURA_VUELO + i * 0.5f;
		patos[i].estado         = VOLANDO;
		patos[i].velocidadCaida = 0.0f;
	}

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

	// --- Flash texture: 16x16 radial gradient, orange-white center ---
	GLuint flashTexture;
	{
		const int SZ = 16;
		unsigned char data[SZ * SZ * 4];
		for (int y = 0; y < SZ; y++) {
			for (int x = 0; x < SZ; x++) {
				float dx   = (x - SZ * 0.5f + 0.5f) / (SZ * 0.5f);
				float dy   = (y - SZ * 0.5f + 0.5f) / (SZ * 0.5f);
				float dist = sqrtf(dx*dx + dy*dy);
				float a    = std::max(0.0f, 1.0f - dist);
				a = a * a; // sharpen falloff
				int i = (y * SZ + x) * 4;
				data[i+0] = 255;
				data[i+1] = (unsigned char)(120 + 135 * a); // orange→white center
				data[i+2] = (unsigned char)(20  +  80 * a);
				data[i+3] = (unsigned char)(255 * a);
			}
		}
		glGenTextures(1, &flashTexture);
		glBindTexture(GL_TEXTURE_2D, flashTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SZ, SZ, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// --- Flash quad VAO (unit square, z=0, transformed via model matrix) ---
	GLuint flashVAO, flashVBO;
	{
		float q[] = {
			-0.5f,  0.5f, 0.0f,  0.0f, 1.0f,
			-0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
			 0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
			-0.5f,  0.5f, 0.0f,  0.0f, 1.0f,
			 0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
			 0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
		};
		glGenVertexArrays(1, &flashVAO);
		glGenBuffers(1, &flashVBO);
		glBindVertexArray(flashVAO);
		glBindBuffer(GL_ARRAY_BUFFER, flashVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(q), q, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

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

	struct {
		GLint projection, view, model, viewPos;
		GLint lightDir, lightAmb, lightDiff, lightSpec;
		GLint matSpec, matShininess;
	} au;

	hu.projection = glGetUniformLocation(hudShader.Program, "projection");
	hu.view       = glGetUniformLocation(hudShader.Program, "view");
	hu.model      = glGetUniformLocation(hudShader.Program, "model");
	hu.uvScale    = glGetUniformLocation(hudShader.Program, "uvScale");
	hu.texture    = glGetUniformLocation(hudShader.Program, "ourTexture");

	au.projection   = glGetUniformLocation(animShader.Program, "projection");
	au.view         = glGetUniformLocation(animShader.Program, "view");
	au.model        = glGetUniformLocation(animShader.Program, "model");
	au.viewPos      = glGetUniformLocation(animShader.Program, "viewPos");
	au.lightDir     = glGetUniformLocation(animShader.Program, "light.direction");
	au.lightAmb     = glGetUniformLocation(animShader.Program, "light.ambient");
	au.lightDiff    = glGetUniformLocation(animShader.Program, "light.diffuse");
	au.lightSpec    = glGetUniformLocation(animShader.Program, "light.specular");
	au.matSpec      = glGetUniformLocation(animShader.Program, "material.specular");
	au.matShininess = glGetUniformLocation(animShader.Program, "material.shininess");

	// Cache de uniformes adicionales para lightingShader (Cazador, Perro, Plumas)
	// Reutilizamos la estructura 'lu' añadiendo lo necesario si hiciera falta, 
	// pero los nombres de uniformes son los mismos.
	
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

	animShader.Use();
	glUniformMatrix4fv(au.projection, 1, GL_FALSE, glm::value_ptr(projection));
	glUniform3f(au.lightDir,       -0.4f, -1.0f, -0.5f);
	glUniform3f(au.lightAmb,        0.35f, 0.30f, 0.25f);
	glUniform3f(au.lightDiff,       0.85f, 0.75f, 0.60f);
	glUniform3f(au.lightSpec,       0.4f,  0.4f,  0.4f);
	glUniform3f(au.matSpec,         0.5f,  0.5f,  0.5f);
	glUniform1f(au.matShininess,    32.0f);

	// Point lights and spotlight must be set on lightingShader, not animShader
	lightingShader.Use();

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
		if (!camaraCenital) {
			DoMovement();
		}

		// ---- Procesar disparo ----
		if (disparoRealizado) {
			disparoRealizado = false; // always consume — prevents ghost shots when exiting cenital mode

			if (!camaraCenital) {
				recoilTimer = RECOIL_DURATION; // kick on every shot
				flashTimer  = FLASH_DURATION;

				glm::vec3 origenRayo    = camera.position;
				glm::vec3 direccionRayo = glm::normalize(camera.front);
				const float ANGULO_MAXIMO_IMPACTO = glm::radians(5.0f);

				for (int i = 0; i < NUM_PATOS; i++) {
					if (patos[i].estado != VOLANDO) continue;

					float px = RADIO_ORBITA * cos(patos[i].angulo);
					float pz = RADIO_ORBITA * sin(patos[i].angulo);
					glm::vec3 posPato(px, patos[i].altura, pz);

					glm::vec3 vecCamaraPato = glm::normalize(posPato - origenRayo);
					float cosAngulo = glm::dot(direccionRayo, vecCamaraPato);
					float angulo    = acos(glm::clamp(cosAngulo, -1.0f, 1.0f));
					float distancia = glm::length(posPato - origenRayo);

					if (angulo < ANGULO_MAXIMO_IMPACTO && distancia < 40.0f) {
						patos[i].estado         = CAYENDO;
						patos[i].velocidadCaida = 0.0f;
						patosDerribados++;
						perroReaccionarDisparo(posPato);

						efectoPlumas.activo   = true;
						efectoPlumas.posicion = posPato;
						efectoPlumas.timer    = 0.0f;
						efectoPlumas.frame    = 0;
						break;
					}
				}
			}
		}

		// ---- Actualizar posición y estado de cada pato ----
		for (int i = 0; i < NUM_PATOS; i++) {
			if (patos[i].estado == VOLANDO) {
				// Avanzar en la órbita circular
				patos[i].angulo += patos[i].velocidad * deltaTime;
				if (patos[i].angulo > glm::two_pi<float>())
					patos[i].angulo -= glm::two_pi<float>();
			} else if (patos[i].estado == CAYENDO) {
				// Caída libre con gravedad
				patos[i].velocidadCaida -= 15.0f * deltaTime;
				patos[i].altura += patos[i].velocidadCaida * deltaTime;
				if (patos[i].altura <= -1.0f)
					patos[i].estado = MUERTO;
			}
		}

		if (recoilTimer > 0.0f) recoilTimer = std::max(0.0f, recoilTimer - deltaTime);
		if (flashTimer  > 0.0f) flashTimer  = std::max(0.0f, flashTimer  - deltaTime);

		// ---- Actualizar máquina de estados del perro ----
		timerPerro += deltaTime;
		animTime   += deltaTime;

		switch (estadoPerro) {
			case PERRO_BUSCANDO:
				// Mantenerse siempre a 10 unidades detrás de la cámara
				posicionActualPerro = camera.position - (glm::normalize(glm::vec3(camera.front.x, 0.0f, camera.front.z)) * 10.0f);
				posicionActualPerro.y = -1.0f; // Nivel del suelo
				// Mirar en la misma dirección que el jugador para estar "de espaldas"
				rotacionPerro = glm::degrees(atan2(camera.front.x, camera.front.z));
				
				articulacionPatas = sin(animTime * 8.0f) * 30.0f;
				articulacionCola  = sin(animTime * 15.0f) * 20.0f;
				articulacionCabeza = sin(animTime * 2.0f) * 10.0f;
				articulacionCuerpoY = abs(sin(animTime * 8.0f)) * 0.1f;
				break;

			case PERRO_ENCONTRADO:
				// Aparecer en el borde del claro (radio 7.5) en dirección al impacto
				posicionActualPerro = glm::normalize(glm::vec3(posImpactoPato.x, 0.0f, posImpactoPato.z)) * 7.5f;
				posicionActualPerro.y = -1.0f;
				// Mirar hacia el centro del mapa (donde está el jugador)
				rotacionPerro = glm::degrees(atan2(posicionActualPerro.x, posicionActualPerro.z));

				articulacionPatas = sin(animTime * 12.0f) * 40.0f; // Animación de correr más rápida
				articulacionCola  = sin(animTime * 30.0f) * 40.0f;
				articulacionCabeza = -15.0f;
				articulacionCuerpoY = abs(sin(animTime * 12.0f)) * 0.2f;
				
				if (timerPerro >= 1.5f) {
					estadoPerro = PERRO_MOSTRANDO;
					timerPerro = 0.0f;
				}
				break;

			case PERRO_MOSTRANDO:
				// Misma posición que ENCONTRADO
				rotacionPerro = glm::degrees(atan2(posicionActualPerro.x, posicionActualPerro.z));
				articulacionPatas = 0.0f;
				articulacionCola  = sin(animTime * 5.0f) * 10.0f;
				articulacionCabeza = 20.0f;
				articulacionCuerpoY = 0.0f;

				if (timerPerro >= 2.0f) {
					estadoPerro = PERRO_BUSCANDO;
					timerPerro = 0.0f;
					patosEnRonda = 0;
				}
				break;

			case PERRO_RIENDO:
				// El perro se ríe frente al jugador
				posicionActualPerro = glm::normalize(glm::vec3(camera.front.x, 0.0f, camera.front.z)) * 7.5f;
				posicionActualPerro.y = -1.0f;
				rotacionPerro = glm::degrees(atan2(posicionActualPerro.x, posicionActualPerro.z));

				articulacionPatas = 0.0f;
				articulacionCola  = 0.0f;
				articulacionCabeza = sin(animTime * 25.0f) * 15.0f;
				articulacionCuerpoY = sin(animTime * 30.0f) * 0.05f;
				
				if (timerPerro >= 2.0f) {
					estadoPerro = PERRO_BUSCANDO;
					timerPerro = 0.0f;
				}
				break;
		}

		// Apply accumulated mouse deltas from CursorPosCallback.
		// Always drain the accumulator (even in cenital) so stale deltas don't
		// fire when the player switches back to FPS mode.
		if (!camaraCenital) {
			GLfloat xOffset = glm::clamp((GLfloat) mouseAccX, -30.0f, 30.0f);
			GLfloat yOffset = glm::clamp((GLfloat)-mouseAccY, -30.0f, 30.0f);
			camera.ProcessMouseMovement(xOffset, yOffset);
		}
		mouseAccX = 0.0;
		mouseAccY = 0.0;

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// ---- World render (lightingShader) ----
		lightingShader.Use();
		glm::mat4 view;
		if (camaraCenital) {
			// Cámara cenital: posicionada arriba del mapa mirando hacia abajo
			glm::vec3 ojoCenital(0.0f, 45.0f, 0.0f);
			glm::vec3 objetivoCenital(0.0f, 0.0f, 0.0f);
			glm::vec3 arribasCenital(0.0f, 0.0f, -1.0f);
			view = glm::lookAt(ojoCenital, objetivoCenital, arribasCenital);
		} else {
			view = camera.GetViewMatrix();
		}
		
		glUniformMatrix4fv(lu.view,   1, GL_FALSE, glm::value_ptr(view));
		glUniform3fv(lu.viewPos,      1, glm::value_ptr(camera.position));

		// Helper lambda: upload model + normal matrix, then draw
		auto drawWorld = [&](Model& m, const glm::mat4& mat, const glm::mat3& norm, bool tex, bool transp) {
			glUniformMatrix4fv(lu.model, 1, GL_FALSE, glm::value_ptr(mat));
			glUniformMatrix3fv(lu.normalMatrix, 1, GL_FALSE, glm::value_ptr(norm));
			glUniform1i(lu.useTexture,   tex   ? 1 : 0);
			glUniform1i(lu.transparency, transp ? 1 : 0);
			m.Draw(lightingShader);
		};

		// Skybox — strip translation from view so it stays "infinitely far"
		glDepthMask(GL_FALSE);
		glDisable(GL_CULL_FACE);
		glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
		glUniformMatrix4fv(lu.view, 1, GL_FALSE, glm::value_ptr(skyboxView));
		glm::mat4 skyModel = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));
		drawWorld(Skybox, skyModel, glm::mat3(1.0f), true, false);
		glDepthMask(GL_TRUE);
		glEnable(GL_CULL_FACE);

		// Restore full view for world objects
		glUniformMatrix4fv(lu.view, 1, GL_FALSE, glm::value_ptr(view));
		glUniform1i(lu.transparency, 1);

		// Floor
		glm::mat4 floorModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
		floorModel = glm::scale(floorModel, glm::vec3(100.0f, 1.0f, 100.0f));
		drawWorld(Piso, floorModel, glm::mat3(1.0f), true, true);

		// ---- Dibujar patos (Shader Animado) ----
		animShader.Use();
		glUniformMatrix4fv(au.view,    1, GL_FALSE, glm::value_ptr(view));
		glUniform3fv(au.viewPos,       1, glm::value_ptr(camera.position));
		
		for (int i = 0; i < NUM_PATOS; i++) {
			if (patos[i].estado == MUERTO) continue;

			float px = RADIO_ORBITA * cos(patos[i].angulo);
			float pz = RADIO_ORBITA * sin(patos[i].angulo);
			float py = patos[i].altura;

			float yaw = patos[i].angulo + glm::half_pi<float>();

			glm::mat4 matPato = glm::mat4(1.0f);
			matPato = glm::translate(matPato, glm::vec3(px, py, pz));
			matPato = glm::rotate(matPato, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
			matPato = glm::scale(matPato, glm::vec3(0.012f)); 
			matPato = glm::translate(matPato, -PatoAnimado.boundingCenter);

			glUniformMatrix4fv(au.model, 1, GL_FALSE, glm::value_ptr(matPato));
			PatoAnimado.Draw(animShader);
		}

		// ---- Dibujar Vegetación (Volver al lightingShader) ----
		lightingShader.Use();
		glUniformMatrix4fv(lu.view, 1, GL_FALSE, glm::value_ptr(view));

		// Arbustos estáticos
		for (const auto& data : arbustosData) {
			drawWorld(Arbusto, data.model, data.normal, true, true);
		}

		// Nubes (dinámicas pero simplificadas)
		Model* nubes[3] = { &Nube1, &Nube2, &Nube3 };
		for (int i = 0; i < 3; i++) {
			float offsetX = sin(currentFrame * 0.05f + i * 2.0f) * 3.0f;
			glm::mat4 m = glm::translate(glm::mat4(1.0f), nubesBasePos[i] + glm::vec3(offsetX, 0.0f, 0.0f));
			m = glm::scale(m, glm::vec3(4.0f));
			// Para nubes, la normal matrix es simple porque el escalado es uniforme
			drawWorld(*nubes[i], m, glm::mat3(1.0f), true, true);
		}

		// Anillo de pasto estático
		for (const auto& data : pastoData) {
			if (data.type == 0)
				drawWorld(PastoAnillo, data.model, data.normal, true, true);
			else
				drawWorld(PastoAnilloRev, data.model, data.normal, true, true);
		}

		// ---- Dibujar cazador (solo visible en cámara cenital) ----
		if (camaraCenital) {
			glm::mat4 matCazador = glm::translate(glm::mat4(1.0f), camera.position - glm::vec3(0.0f, 0.8f, 0.0f));
			float yawCazador = atan2(camera.front.x, camera.front.z);
			matCazador = glm::rotate(matCazador, yawCazador, glm::vec3(0.0f, 1.0f, 0.0f));
			matCazador = glm::scale(matCazador, glm::vec3(1.2f));

			drawWorld(Cazador, matCazador, glm::mat3(1.0f), true, true);
		}

		// ---- Dibujar perro (Jerárquico) ----
		{
			// Cuerpo (Raíz)
			glm::mat4 modelCuerpo = glm::translate(glm::mat4(1.0f), posicionActualPerro + glm::vec3(0.0f, articulacionCuerpoY, 0.0f));
			modelCuerpo = glm::rotate(modelCuerpo, glm::radians(rotacionPerro), glm::vec3(0.0f, 1.0f, 0.0f));
			modelCuerpo = glm::scale(modelCuerpo, glm::vec3(1.5f));
			
			drawWorld(DogBody, modelCuerpo, glm::mat3(1.0f), true, true);

			// Cabeza (Hijo del Cuerpo)
			glm::mat4 modelCabeza = modelCuerpo;
			modelCabeza = glm::translate(modelCabeza, glm::vec3(0.0f, 0.093f, 0.208f));
			modelCabeza = glm::rotate(modelCabeza, glm::radians(articulacionCabeza), glm::vec3(1.0f, 0.0f, 0.0f));
			drawWorld(DogHead, modelCabeza, glm::mat3(1.0f), true, true);

			// Cola (Hijo del Cuerpo)
			glm::mat4 modelCola = modelCuerpo;
			modelCola = glm::translate(modelCola, glm::vec3(0.0f, 0.026f, -0.288f));
			modelCola = glm::rotate(modelCola, glm::radians(articulacionCola), glm::vec3(0.0f, 1.0f, 0.0f));
			drawWorld(DogTail, modelCola, glm::mat3(1.0f), true, true);

			// Patas Delanteras
			glm::mat4 modelFL = modelCuerpo;
			modelFL = glm::translate(modelFL, glm::vec3(0.112f, -0.044f, 0.074f));
			modelFL = glm::rotate(modelFL, glm::radians(articulacionPatas), glm::vec3(1.0f, 0.0f, 0.0f));
			drawWorld(DogLegFL, modelFL, glm::mat3(1.0f), true, true);

			glm::mat4 modelFR = modelCuerpo;
			modelFR = glm::translate(modelFR, glm::vec3(-0.111f, -0.055f, 0.074f));
			modelFR = glm::rotate(modelFR, glm::radians(-articulacionPatas), glm::vec3(1.0f, 0.0f, 0.0f));
			drawWorld(DogLegFR, modelFR, glm::mat3(1.0f), true, true);

			// Patas Traseras
			glm::mat4 modelBL = modelCuerpo;
			modelBL = glm::translate(modelBL, glm::vec3(0.082f, -0.046f, -0.218f));
			modelBL = glm::rotate(modelBL, glm::radians(-articulacionPatas), glm::vec3(1.0f, 0.0f, 0.0f));
			drawWorld(DogLegBL, modelBL, glm::mat3(1.0f), true, true);

			glm::mat4 modelBR = modelCuerpo;
			modelBR = glm::translate(modelBR, glm::vec3(-0.083f, -0.057f, -0.231f));
			modelBR = glm::rotate(modelBR, glm::radians(articulacionPatas), glm::vec3(1.0f, 0.0f, 0.0f));
			drawWorld(DogLegBR, modelBR, glm::mat3(1.0f), true, true);
		}

		// ---- Actualizar y Dibujar efecto de plumas ----
		if (efectoPlumas.activo) {
			efectoPlumas.timer += deltaTime;
			efectoPlumas.frame = (int)(efectoPlumas.timer / (DURACION_PLUMAS / 5.0f));

			if (efectoPlumas.timer >= DURACION_PLUMAS) {
				efectoPlumas.activo = false;
			} else {
				glm::mat4 matPlumas = glm::translate(glm::mat4(1.0f), efectoPlumas.posicion);
				matPlumas = glm::rotate(matPlumas, efectoPlumas.timer * 3.0f, glm::vec3(0.0f, 1.0f, 0.0f));
				matPlumas = glm::scale(matPlumas, glm::vec3(2.0f));

				int frameIdx = glm::clamp(efectoPlumas.frame, 0, 4);
				drawWorld(*framesPlumas[frameIdx], matPlumas, glm::mat3(1.0f), true, true);
			}
		}

		// ---- HUD (hudShader, no depth test against world) ----
		if (!camaraCenital) {
			glClear(GL_DEPTH_BUFFER_BIT);
			hudShader.Use();
			glm::mat4 hudView = glm::mat4(1.0f);
			glUniformMatrix4fv(hu.projection, 1, GL_FALSE, glm::value_ptr(projection));
			glUniformMatrix4fv(hu.view,       1, GL_FALSE, glm::value_ptr(hudView));

			// Recoil: quadratic ease-out (sharp kick, smooth return)
			float recoilT = recoilTimer / RECOIL_DURATION;
			float recoil  = recoilT * recoilT;

			glm::mat4 model = glm::translate(glm::mat4(1.0f),
				glm::vec3(0.5f, -0.8f - recoil * 0.04f, -1.0f + recoil * 0.1f));
			model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::rotate(model, glm::radians(recoil * 12.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::scale(model, glm::vec3(0.3f));
			glUniformMatrix4fv(hu.model, 1, GL_FALSE, glm::value_ptr(model));
			glUniform1f(hu.uvScale, 1.0f);
			Gun.Draw(hudShader);

			// Muzzle flash — additive blending, two layers (outer glow + inner core)
			if (flashTimer > 0.0f) {
				float t  = flashTimer / FLASH_DURATION; // 1→0
				float ar = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;

				glDisable(GL_DEPTH_TEST);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive: brightens whatever is behind

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, flashTexture);
				glUniform1i(hu.texture, 0);

				// Barrel tip position in view space (adjust x/y if it looks off)
				const glm::vec3 BARREL_POS(0.5f, -0.68f, -1.0f);
				glm::vec3 barrelPos = BARREL_POS + glm::vec3(0.0f, -recoil * 0.04f, recoil * 0.1f);

				// Two layers: outer (large, dim) and inner (small, bright)
				const float sizes[2]     = { t * 0.14f, t * 0.07f };
				const float rotSpeeds[2] = { 47.0f, 73.0f };

				for (int layer = 0; layer < 2; layer++) {
					float angle = (float)glfwGetTime() * rotSpeeds[layer];
					glm::mat4 mFlash = glm::translate(glm::mat4(1.0f), barrelPos);
					mFlash = glm::rotate(mFlash, angle, glm::vec3(0.0f, 0.0f, 1.0f));
					mFlash = glm::scale(mFlash, glm::vec3(sizes[layer] / ar, sizes[layer], 1.0f));
					glUniformMatrix4fv(hu.model, 1, GL_FALSE, glm::value_ptr(mFlash));
					glBindVertexArray(flashVAO);
					glDrawArrays(GL_TRIANGLES, 0, 6);
				}

				glBindVertexArray(0);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // restore for crosshair
			}

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
		}

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

	// Restringir al jugador al círculo central de tierra (radio = 8 unidades)
	const float RADIO_TIERRA = 8.0f;
	glm::vec2 posXZ(camera.position.x, camera.position.z);
	float distanciaAlCentro = glm::length(posXZ);
	if (distanciaAlCentro > RADIO_TIERRA) {
		posXZ = glm::normalize(posXZ) * RADIO_TIERRA;
		camera.position.x = posXZ.x;
		camera.position.z = posXZ.y;
	}
}

void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_C && action == GLFW_PRESS) {
		camaraCenital = !camaraCenital;
	}

	if (key >= 0 && key < 1024) {
		if (action == GLFW_PRESS)   keys[key] = true;
		if (action == GLFW_RELEASE) keys[key] = false;
	}
}

void perroReaccionarDisparo(glm::vec3 posImpacto) {
	estadoPerro = PERRO_ENCONTRADO;
	timerPerro  = 0.0f;
	posImpactoPato = posImpacto;
	patosEnRonda++;
}
