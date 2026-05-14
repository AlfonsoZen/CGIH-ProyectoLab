#include <iostream>
#include <cmath>
#include <vector>
#include <array>
#include <string>

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
#include "modelStatic.h"

// Function prototypes
void KeyCallback(GLFWwindow*, int, int, int, int);
void DoMovement();

// Window dimensions
int SCREEN_WIDTH, SCREEN_HEIGHT;

// =============================================================================
//   CONSTANTES DEL MUNDO — basadas en el proyecto del compañero
// =============================================================================

// Factor de escala global: los FBX de Blender vienen enormes; 0.3 los ajusta
// al mundo del juego (MAP_RADIUS ≈ 250 unidades).
const float SCENE_SCALE      = 0.3f;
// Altura fija de la cámara (nivel del suelo donde camina el jugador).
const float PLAYER_HEIGHT    = 70.0f;
// Radio del mapa circular; se recalcula en main() a partir del bbox del terreno.
float       MAP_RADIUS       = 250.0f;
// Hitbox de árboles/arbustos = 50 % del bbox de cada mesh (evita colisiones duras).
const float COLLISION_SHRINK = 0.5f;

// --- Constantes del juego adaptadas a la nueva escala ---
// Radio de órbita de los patos (dentro del mapa, sobre el área de pasto).
const float RADIO_ORBITA  = 200.0f;
// Altura Y de vuelo de los patos.
const float ALTURA_VUELO  =  90.0f;
// Radio del claro central donde el jugador puede caminar (área de fango).
// Ajustar visualmente si no coincide con el borde del fango.fbx.
const float RADIO_TIERRA  =  80.0f;

// =============================================================================
//   SISTEMA DE COLISIONES AABB (del compañero)
// =============================================================================
struct AABB { float minX, maxX, minZ, maxZ; };
std::vector<AABB> solidBoxes;

bool insideAny(float x, float z) {
    for (auto& b : solidBoxes)
        if (x > b.minX && x < b.maxX && z > b.minZ && z < b.maxZ) return true;
    return false;
}

// Convierte la caja local de un mesh al espacio mundo, con el mismo
// translate+scale que usa drawAt(), y aplica un margen de reducción.
AABB worldBoxOfMesh(const ModelStatic::MeshBBox& mbb, glm::vec3 pos, float scale, float shrink) {
    glm::vec3 mn = mbb.mn * scale + pos * scale;
    glm::vec3 mx = mbb.mx * scale + pos * scale;
    float sx = (mx.x - mn.x) * 0.5f * shrink;
    float sz = (mx.z - mn.z) * 0.5f * shrink;
    return { mn.x + sx, mx.x - sx, mn.z + sz, mx.z - sz };
}

void addSolidBoxesFor(const ModelStatic& m, glm::vec3 pos, float scale, float shrink) {
    for (auto& mbb : m.meshBoxes)
        solidBoxes.push_back(worldBoxOfMesh(mbb, pos, scale, shrink));
}

// =============================================================================
//   CÁMARA Y ENTRADA
// =============================================================================
Camera camera(glm::vec3(0.0f, PLAYER_HEIGHT, 60.0f));
bool keys[1024];

// Acumulador de delta del mouse (fix WSL2: el warp del cursor GLFW_CURSOR_DISABLED
// provoca efecto joystick al leer glfwGetCursorPos; el callback solo notifica
// movimiento real del usuario).
double mouseAccX      = 0.0;
double mouseAccY      = 0.0;
double mousePrevX     = 0.0;
double mousePrevY     = 0.0;
bool   mouseFirstMove = true;

// =============================================================================
//   SISTEMA DE PATOS
// =============================================================================
const int NUM_PATOS = 3;
enum EstadoPato { VOLANDO, CAYENDO, MUERTO };
struct Pato {
    float angulo;         // ángulo actual en la órbita (radianes)
    float velocidad;      // velocidad angular (rad/segundo)
    float altura;         // altura Y de vuelo
    EstadoPato estado;
    float velocidadCaida; // para la física de caída libre
};
std::array<Pato, NUM_PATOS> patos;

// =============================================================================
//   SISTEMA DE DISPARO
// =============================================================================
int  patosDerribados  = 0;
int  patosEscapados   = 0;
bool disparoRealizado = false;

// =============================================================================
//   SISTEMA DEL PERRO — máquina de estados
// =============================================================================
enum EstadoPerro { PERRO_BUSCANDO, PERRO_ENCONTRADO, PERRO_MOSTRANDO, PERRO_RIENDO };
EstadoPerro estadoPerro = PERRO_BUSCANDO;
float timerPerro        = 0.0f;
float animTime          = 0.0f;
int   patosEnRonda      = 0;

// Articulaciones del perro jerárquico (BUSCANDO y ENCONTRADO)
float articulacionPatas    = 0.0f;
float articulacionCola     = 0.0f;
float articulacionCabeza   = 0.0f;
float articulacionCuerpoY  = 0.0f;

glm::vec3 posicionActualPerro(0.0f, PLAYER_HEIGHT - 1.0f, 15.0f);
glm::vec3 posImpactoPato(0.0f);
float rotacionPerro = 180.0f;

// --- Constantes para la animación pop-up del perro con patos (MOSTRANDO) ---
// Mismos valores del compañero: sube, se queda visible, baja.
const float DOG_RISE_TIME  = 0.8f;
const float DOG_HOLD_TIME  = 1.4f;
const float DOG_FALL_TIME  = 0.8f;
const float DOG_TOTAL_TIME = DOG_RISE_TIME + DOG_HOLD_TIME + DOG_FALL_TIME; // 3.0 s
// Y escondido bajo el suelo; cuando dogWorldY == DOG_HIDE_Y el modelo
// está 200 unidades por debajo del terreno y no es visible.
const float DOG_HIDE_Y = -200.0f;

// =============================================================================
//   MODO DE CÁMARA Y ESTADO DEL JUEGO
// =============================================================================
bool camaraCenital = false;
bool gameStarted   = false;

// =============================================================================
//   EFECTOS DE DISPARO
// =============================================================================
float recoilTimer = 0.0f;
float flashTimer  = 0.0f;
const float RECOIL_DURATION = 0.15f;
const float FLASH_DURATION  = 0.08f;

// =============================================================================
//   EFECTO DE PLUMAS
// =============================================================================
struct EfectoPlumas {
    bool activo;
    glm::vec3 posicion;
    float timer;
    int   frame;
};
EfectoPlumas efectoPlumas = { false, glm::vec3(0.0f), 0.0f, 0 };
const float DURACION_PLUMAS = 0.6f;

// =============================================================================
//   SISTEMA DE RONDAS
// =============================================================================
int   rondaActual          = 1;
int   patosDerribadosRonda = 0;
int   patosEscapadosRonda  = 0;
float timerPausaRonda      = 0.0f;
bool  enPausaRonda         = false;
float velocidadBaseRonda   = 0.6f;
std::array<float, NUM_PATOS> angulosRecorridos = { 0.0f, 0.0f, 0.0f };

// =============================================================================
//   NUBES — 7 nubes del compañero con desplazamiento y wrap-around
// =============================================================================
const int NUM_NUBES = 7;
float nubeOffsets[NUM_NUBES] = { 0, 0, 0, 0, 0, 0, 0 };
float nubeSpeeds [NUM_NUBES] = { 8.0f, 6.0f, 9.0f, 7.0f, 6.5f, 8.5f, 7.5f };
const float CLOUD_X_RANGE   = 200.0f;
bool  cloudsMoving           = true;

// =============================================================================
//   INVITACIÓN SMASH CAYENDO (del compañero, activada con tecla I)
// =============================================================================
bool  inviteFalling = false;
float inviteTimer   = 0.0f;
const float INVITE_FALL_DURATION = 5.0f;
const float INVITE_SPAWN_Y       = 600.0f;
const float INVITE_TURNS         = 3.5f;
const float INVITE_TURBULENCE    = 12.0f;

// =============================================================================
//   FÍSICA
// =============================================================================
GLfloat deltaTime    = 0.0f;
GLfloat lastFrame    = 0.0f;
GLfloat velocityY    = 0.0f;
GLfloat gravity      = -30.0f;
GLfloat jumpForce    = 12.0f;
GLfloat groundHeight = PLAYER_HEIGHT;  // el salto regresa a esta altura
bool    isGrounded   = true;

// =============================================================================
//   FORWARD DECLARATIONS
// =============================================================================
void perroReaccionarDisparo(glm::vec3 posImpacto);
void iniciarNuevaRonda();

// =============================================================================
//   CROSSHAIR VERTICES (quad en screen-space)
// =============================================================================
float crosshairVertices[] = {
    -0.05f,  0.05f, -0.1f,   0.0f, 1.0f,
    -0.05f, -0.05f, -0.1f,   0.0f, 0.0f,
     0.05f, -0.05f, -0.1f,   1.0f, 0.0f,
    -0.05f,  0.05f, -0.1f,   0.0f, 1.0f,
     0.05f, -0.05f, -0.1f,   1.0f, 0.0f,
     0.05f,  0.05f, -0.1f,   1.0f, 1.0f,
};

// =============================================================================
//   CALLBACKS
// =============================================================================
void CursorPosCallback(GLFWwindow*, double x, double y) {
    if (mouseFirstMove) { mousePrevX = x; mousePrevY = y; mouseFirstMove = false; return; }
    mouseAccX += x - mousePrevX;
    mouseAccY += y - mousePrevY;
    mousePrevX = x;
    mousePrevY = y;
}

void MouseButtonCallback(GLFWwindow*, int button, int action, int) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        disparoRealizado = true;
}

// =============================================================================
//   MAIN
// =============================================================================
int main()
{
    // ---- Inicialización de GLFW ----
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWmonitor*       primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode           = glfwGetVideoMode(primaryMonitor);
    // Ventana invisible durante la carga: evita el overhead del compositor
    // WSLG/Wayland en glTexImage2D. Se hace visible justo antes del game loop.
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Duck Hunt 3D", nullptr, nullptr);
    if (!window) { std::cout << "Failed to create GLFW window\n"; glfwTerminate(); return EXIT_FAILURE; }

    glfwMakeContextCurrent(window);
    glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental = GL_TRUE;
    if (GLEW_OK != glewInit()) { std::cout << "Failed to initialize GLEW\n"; return EXIT_FAILURE; }

    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glEnable(GL_DEPTH_TEST);

    // ---- VAO del crosshair ----
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

    // ---- Shaders ----
    // sceneShader: todos los modelos estáticos del escenario del compañero (sin iluminación).
    Shader sceneShader  ("shaders/modelLoading.vs", "shaders/modelLoading.frag");
    // lightingShader: objetos del juego con iluminación Phong (perro, cazador, plumas, HUD 3D).
    Shader lightingShader("shaders/default.vert",   "shaders/default.frag");
    // hudShader: zapper, crosshair (screen-space, sin depth).
    Shader hudShader    ("shaders/shader.vs",        "shaders/shader.frag");
    // animShader: patos con skinning de huesos.
    Shader animShader   ("shaders/anim.vs",          "shaders/anim.frag");

    // ---- Modelos del escenario (del compañero — FBX de Blender) ----
    const std::string SCENE = "assets/models/scene/";
    auto _tload = [](const char* lbl){ std::cout << "  [t=" << (int)(glfwGetTime()*1000) << "ms] " << lbl << "\n"; };
    std::cout << "\n*** CARGA ESCENARIO ***\n";
    _tload("terreno...");    ModelStatic terreno (SCENE + "Terreno.assbin");
    _tload("pasto...");      ModelStatic pasto   (SCENE + "Pasto.assbin");
    _tload("fango...");      ModelStatic fango   (SCENE + "fango.assbin");
    _tload("arboles...");    ModelStatic arboles (SCENE + "arboles.assbin");
    _tload("arbustos...");   ModelStatic arbustos(SCENE + "arbustos.assbin");
    _tload("nubes...");
    ModelStatic nube1(SCENE + "nube1.assbin"); ModelStatic nube2(SCENE + "nube2.assbin");
    ModelStatic nube3(SCENE + "nube3.assbin"); ModelStatic nube4(SCENE + "nube4.assbin");
    ModelStatic nube5(SCENE + "nube5.assbin"); ModelStatic nube6(SCENE + "nube6.assbin");
    ModelStatic nube7(SCENE + "nube7.assbin");
    ModelStatic* nubes[NUM_NUBES] = { &nube1, &nube2, &nube3, &nube4, &nube5, &nube6, &nube7 };
    _tload("lago...");       ModelStatic lago      (SCENE + "lago.assbin");
    _tload("rocas...");      ModelStatic rocas     (SCENE + "rocas.assbin");
    _tload("tazon...");      ModelStatic tazon     (SCENE + "tazon de perro.assbin");
    _tload("invitacion..."); ModelStatic invitacion(SCENE + "invitacion smash.assbin");
    _tload("perroPatos..."); ModelStatic perroPatos(SCENE + "perro con patos.assbin");
    _tload("Escenario OK");
    std::cout << ">>> Escenario listo\n";

    // Calcular MAP_RADIUS a partir del bounding box del terreno:
    // radio = min(semi-eje X, semi-eje Z) * SCENE_SCALE * 0.95 (margen al borde).
    {
        float halfX = (terreno.bboxMax.x - terreno.bboxMin.x) * 0.5f;
        float halfZ = (terreno.bboxMax.z - terreno.bboxMin.z) * 0.5f;
        float half  = (halfX < halfZ) ? halfX : halfZ;
        MAP_RADIUS = half * SCENE_SCALE * 0.95f;
        std::cout << ">>> MAP_RADIUS = " << MAP_RADIUS << "\n";
    }

    // Generar cajas de colisión AABB por cada mesh de árboles y arbustos.
    // Los mismos offsets que usa drawAt() para colocar los modelos en escena.
    addSolidBoxesFor(arboles,  glm::vec3( 12.0f, 0.0f, -6.0f), SCENE_SCALE, COLLISION_SHRINK);
    addSolidBoxesFor(arbustos, glm::vec3( -8.0f, 0.0f,  6.0f), SCENE_SCALE, COLLISION_SHRINK);
    std::cout << ">>> Cajas colision: " << solidBoxes.size() << "\n";

    // ---- Modelos del juego (nuestros — OBJ/GLB) ----
    Model Skybox ((char*)"assets/models/Skybox.obj");
    Model Gun    ((char*)"assets/models/zapper.obj");

    // Perro jerárquico (partes separadas para animación de articulaciones)
    Model DogBody ((char*)"assets/models/dog/DogBody.obj");
    Model DogHead ((char*)"assets/models/dog/HeadDog.obj");
    Model DogTail ((char*)"assets/models/dog/TailDog.obj");
    Model DogLegFL((char*)"assets/models/dog/F_LeftLegDog.obj");
    Model DogLegFR((char*)"assets/models/dog/F_RightLegDog.obj");
    Model DogLegBL((char*)"assets/models/dog/B_LeftLegDog.obj");
    Model DogLegBR((char*)"assets/models/dog/B_RightLegDog.obj");

    // Cazador visible en la cámara cenital
    Model Cazador((char*)"assets/models/RedDog.obj");

    // Marcador HUD: dígitos 3D, barra de dificultad, logo y prompt de inicio
    Model Digito0((char*)"assets/models/Zero.glb");
    Model Digito1((char*)"assets/models/One.glb");
    Model Digito2((char*)"assets/models/StartGameText.002.glb");
    Model Digito3((char*)"assets/models/Three.glb");
    Model Digito4((char*)"assets/models/15368-mid.004.glb");
    Model Digito5((char*)"assets/models/Five.glb");
    Model Digito6((char*)"assets/models/15368-mid.006.glb");
    Model Digito7((char*)"assets/models/15368-mid.005.glb");
    Model Digito8((char*)"assets/models/Eight.glb");
    Model BarDificultad((char*)"assets/models/DifficulityBar.glb");
    Model LogoJuego    ((char*)"assets/models/DuckseasonLogo.glb");
    Model StartPrompt  ((char*)"assets/models/Start_ENG.glb");
    Model PerroRiendo  ((char*)"assets/models/perro_riendo/base_basic_shaded.glb");

    std::array<Model*, 9> digitoModels = {
        &Digito0, &Digito1, &Digito2, &Digito3, &Digito4,
        &Digito5, &Digito6, &Digito7, &Digito8
    };

    // Efecto de plumas al derribar un pato (5 frames)
    Model Plumas1((char*)"assets/models/feathers_1.glb");
    Model Plumas2((char*)"assets/models/feathers_2.glb");
    Model Plumas3((char*)"assets/models/feathers_3.glb");
    Model Plumas4((char*)"assets/models/feathers_4.glb");
    Model Plumas5((char*)"assets/models/feathers_5.glb");
    std::array<Model*, 5> framesPlumas = { &Plumas1, &Plumas2, &Plumas3, &Plumas4, &Plumas5 };

    // ---- Pato animado (FBX con skinning esquelético) ----
    std::cout << "*** CARGA PATO ***\n";
    ModelAnim PatoAnimado("assets/models/Pato.fbx");
    PatoAnimado.initShaders(animShader.Program);
    std::cout << ">>> Pato listo\n";

    // ---- Inicializar los 3 patos en órbita circular ----
    for (int i = 0; i < NUM_PATOS; i++) {
        patos[i].angulo         = glm::radians(120.0f * i); // 0°, 120°, 240°
        patos[i].velocidad      = 0.6f + i * 0.05f;
        patos[i].altura         = ALTURA_VUELO + i * 0.5f;
        patos[i].estado         = VOLANDO;
        patos[i].velocidadCaida = 0.0f;
    }

    // ---- Textura del crosshair ----
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
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    // ---- Textura del flash de disparo (gradiente radial generado en CPU) ----
    GLuint flashTexture;
    {
        const int SZ = 16;
        unsigned char data[SZ * SZ * 4];
        for (int y = 0; y < SZ; y++) {
            for (int x = 0; x < SZ; x++) {
                float dx = (x - SZ * 0.5f + 0.5f) / (SZ * 0.5f);
                float dy = (y - SZ * 0.5f + 0.5f) / (SZ * 0.5f);
                float a  = std::max(0.0f, 1.0f - sqrtf(dx*dx + dy*dy));
                a = a * a;
                int i = (y * SZ + x) * 4;
                data[i+0] = 255;
                data[i+1] = (unsigned char)(120 + 135 * a);
                data[i+2] = (unsigned char)( 20 +  80 * a);
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

    // ---- VAO del quad para el flash (unidad centrada, transformado por model matrix) ----
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

    // ---- Matriz de proyección ----
    glm::mat4 projection = glm::perspective(
        glm::radians(camera.GetZoom()),
        (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT,
        0.1f, 5000.0f   // far plane ampliado para el escenario grande
    );

    // ---- Cache de uniform locations ----
    // lu → lightingShader (Phong)
    struct {
        GLint projection, view, model, normalMatrix, viewPos;
        GLint transparency, useTexture, shininess, baseColor;
        GLint dirDirection, dirAmbient, dirDiffuse, dirSpecular;
        GLint plAmbient[4], plDiffuse[4], plSpecular[4];
        GLint plConstant[4], plLinear[4], plQuadratic[4];
        GLint slAmbient, slDiffuse, slSpecular;
        GLint slConstant, slLinear, slQuadratic;
    } lu;

    lu.projection   = glGetUniformLocation(lightingShader.Program, "projection");
    lu.view         = glGetUniformLocation(lightingShader.Program, "view");
    lu.model        = glGetUniformLocation(lightingShader.Program, "model");
    lu.normalMatrix = glGetUniformLocation(lightingShader.Program, "normalMatrix");
    lu.viewPos      = glGetUniformLocation(lightingShader.Program, "viewPos");
    lu.transparency = glGetUniformLocation(lightingShader.Program, "transparency");
    lu.useTexture   = glGetUniformLocation(lightingShader.Program, "useTexture");
    lu.baseColor    = glGetUniformLocation(lightingShader.Program, "baseColor");
    lu.shininess    = glGetUniformLocation(lightingShader.Program, "material.shininess");
    lu.dirDirection = glGetUniformLocation(lightingShader.Program, "dirLight.direction");
    lu.dirAmbient   = glGetUniformLocation(lightingShader.Program, "dirLight.ambient");
    lu.dirDiffuse   = glGetUniformLocation(lightingShader.Program, "dirLight.diffuse");
    lu.dirSpecular  = glGetUniformLocation(lightingShader.Program, "dirLight.specular");
    for (int i = 0; i < 4; i++) {
        std::string b = "pointLights[" + std::to_string(i) + "].";
        lu.plAmbient[i]   = glGetUniformLocation(lightingShader.Program, (b+"ambient").c_str());
        lu.plDiffuse[i]   = glGetUniformLocation(lightingShader.Program, (b+"diffuse").c_str());
        lu.plSpecular[i]  = glGetUniformLocation(lightingShader.Program, (b+"specular").c_str());
        lu.plConstant[i]  = glGetUniformLocation(lightingShader.Program, (b+"constant").c_str());
        lu.plLinear[i]    = glGetUniformLocation(lightingShader.Program, (b+"linear").c_str());
        lu.plQuadratic[i] = glGetUniformLocation(lightingShader.Program, (b+"quadratic").c_str());
    }
    lu.slAmbient   = glGetUniformLocation(lightingShader.Program, "spotLight.ambient");
    lu.slDiffuse   = glGetUniformLocation(lightingShader.Program, "spotLight.diffuse");
    lu.slSpecular  = glGetUniformLocation(lightingShader.Program, "spotLight.specular");
    lu.slConstant  = glGetUniformLocation(lightingShader.Program, "spotLight.constant");
    lu.slLinear    = glGetUniformLocation(lightingShader.Program, "spotLight.linear");
    lu.slQuadratic = glGetUniformLocation(lightingShader.Program, "spotLight.quadratic");

    // su → sceneShader (flat texture, sin iluminación)
    struct { GLint view, projection, model; } su;
    su.view       = glGetUniformLocation(sceneShader.Program, "view");
    su.projection = glGetUniformLocation(sceneShader.Program, "projection");
    su.model      = glGetUniformLocation(sceneShader.Program, "model");

    // hu → hudShader
    struct { GLint projection, view, model, uvScale, texture; } hu;
    hu.projection = glGetUniformLocation(hudShader.Program, "projection");
    hu.view       = glGetUniformLocation(hudShader.Program, "view");
    hu.model      = glGetUniformLocation(hudShader.Program, "model");
    hu.uvScale    = glGetUniformLocation(hudShader.Program, "uvScale");
    hu.texture    = glGetUniformLocation(hudShader.Program, "ourTexture");

    // au → animShader (patos con skinning)
    struct {
        GLint projection, view, model, viewPos;
        GLint lightDir, lightAmb, lightDiff, lightSpec;
        GLint matSpec, matShininess;
    } au;
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

    // ---- Lambda: dibuja un modelo del compañero con posición y SCENE_SCALE ----
    // La posición se multiplica por SCENE_SCALE para que sea coherente con las
    // unidades de Blender (misma fórmula que usaba el compañero en drawAt).
    auto drawAt = [&](ModelStatic& m, glm::vec3 pos) {
        glm::mat4 mdl(1.0f);
        mdl = glm::translate(mdl, pos * SCENE_SCALE);
        mdl = glm::scale(mdl, glm::vec3(SCENE_SCALE));
        glUniformMatrix4fv(su.model, 1, GL_FALSE, glm::value_ptr(mdl));
        m.Draw(sceneShader);
    };

    // ---- Lambda: renderiza un objeto 3D con lightingShader ----
    auto drawWorld = [&](Model& m, const glm::mat4& mat, const glm::mat3& norm, bool tex, bool transp) {
        glUniformMatrix4fv(lu.model,      1, GL_FALSE, glm::value_ptr(mat));
        glUniformMatrix3fv(lu.normalMatrix, 1, GL_FALSE, glm::value_ptr(norm));
        glUniform1i(lu.useTexture,   tex   ? 1 : 0);
        glUniform1i(lu.transparency, transp ? 1 : 0);
        m.Draw(lightingShader);
    };

    // ---- Lambda HUD: renderiza un modelo 3D en espacio de pantalla (NDC) ----
    auto renderizarHUD3D = [&](Model* modelo, float xPos, float yPos, float escala,
                               glm::vec3 colorHUD, float rotY = 0.0f)
    {
        lightingShader.Use();
        glUniform3f(lu.dirAmbient, 1.0f, 1.0f, 1.0f);
        glUniform3f(lu.dirDiffuse, 0.0f, 0.0f, 0.0f);
        glUniform3fv(lu.baseColor, 1, glm::value_ptr(colorHUD));
        glUniform1i(lu.useTexture, 0);

        glm::mat4 id = glm::mat4(1.0f);
        glUniformMatrix3fv(lu.normalMatrix, 1, GL_FALSE, glm::value_ptr(glm::mat3(1.0f)));
        glm::mat4 m = glm::translate(id, glm::vec3(xPos, yPos, -0.5f));
        float ar = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
        m = glm::scale(m, glm::vec3(escala / ar, escala, escala));
        if (rotY != 0.0f)
            m = glm::rotate(m, glm::radians(rotY), glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(lu.model,      1, GL_FALSE, glm::value_ptr(m));
        glUniformMatrix4fv(lu.view,       1, GL_FALSE, glm::value_ptr(id));
        glUniformMatrix4fv(lu.projection, 1, GL_FALSE, glm::value_ptr(id));
        glUniform1i(lu.transparency, 1);

        glDisable(GL_CULL_FACE);
        modelo->Draw(lightingShader);
        glEnable(GL_CULL_FACE);

        glUniform3f(lu.dirAmbient, 0.35f, 0.30f, 0.25f);
        glUniform3f(lu.dirDiffuse, 0.85f, 0.75f, 0.60f);
        glUniform1i(lu.useTexture, 1);
    };

    // ---- Uniforms estáticos (se setean UNA vez, no cambian durante el juego) ----
    lightingShader.Use();
    glUniformMatrix4fv(lu.projection, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1f(lu.shininess, 32.0f);
    glUniform3f(lu.dirDirection, -0.4f, -1.0f, -0.5f);
    glUniform3f(lu.dirAmbient,    0.35f, 0.30f, 0.25f);
    glUniform3f(lu.dirDiffuse,    0.85f, 0.75f, 0.60f);
    glUniform3f(lu.dirSpecular,   0.4f,  0.4f,  0.4f);
    for (int i = 0; i < 4; i++) {
        glUniform3f(lu.plAmbient[i],   0.0f, 0.0f, 0.0f);
        glUniform3f(lu.plDiffuse[i],   0.0f, 0.0f, 0.0f);
        glUniform3f(lu.plSpecular[i],  0.0f, 0.0f, 0.0f);
        glUniform1f(lu.plConstant[i],  1.0f);
        glUniform1f(lu.plLinear[i],    0.0f);
        glUniform1f(lu.plQuadratic[i], 0.0f);
    }
    glUniform3f(lu.slAmbient,   0.0f, 0.0f, 0.0f);
    glUniform3f(lu.slDiffuse,   0.0f, 0.0f, 0.0f);
    glUniform3f(lu.slSpecular,  0.0f, 0.0f, 0.0f);
    glUniform1f(lu.slConstant,  1.0f);
    glUniform1f(lu.slLinear,    0.0f);
    glUniform1f(lu.slQuadratic, 0.0f);

    animShader.Use();
    glUniformMatrix4fv(au.projection, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(au.lightDir,  -0.4f, -1.0f, -0.5f);
    glUniform3f(au.lightAmb,   0.35f, 0.30f, 0.25f);
    glUniform3f(au.lightDiff,  0.85f, 0.75f, 0.60f);
    glUniform3f(au.lightSpec,  0.4f,  0.4f,  0.4f);
    glUniform3f(au.matSpec,    0.5f,  0.5f,  0.5f);
    glUniform1f(au.matShininess, 32.0f);

    sceneShader.Use();
    glUniformMatrix4fv(su.projection, 1, GL_FALSE, glm::value_ptr(projection));

    // Mostrar ventana ahora que todos los assets están cargados en GPU.
    glfwShowWindow(window);
    glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    projection = glm::perspective(glm::radians(camera.GetZoom()),
        (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 5000.0f);
    lightingShader.Use();
    glUniformMatrix4fv(lu.projection, 1, GL_FALSE, glm::value_ptr(projection));
    animShader.Use();
    glUniformMatrix4fv(au.projection, 1, GL_FALSE, glm::value_ptr(projection));
    sceneShader.Use();
    glUniformMatrix4fv(su.projection, 1, GL_FALSE, glm::value_ptr(projection));

    // =========================================================================
    //   BUCLE PRINCIPAL
    // =========================================================================
    while (!glfwWindowShouldClose(window))
    {
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // ---- Física del salto ----
        velocityY += gravity * deltaTime;
        camera.position.y += velocityY * deltaTime;
        if (camera.position.y <= groundHeight) {
            camera.position.y = groundHeight;
            velocityY  = 0.0f;
            isGrounded = true;
        }

        // ---- Entrada ----
        glm::vec3 prevPos = camera.position;
        glfwPollEvents();
        if (!camaraCenital && gameStarted) DoMovement();

        // ---- Límite circular del mapa y colisiones AABB con sliding ----
        {
            glm::vec3 camPos = camera.position;
            float distXZ = sqrtf(camPos.x * camPos.x + camPos.z * camPos.z);
            if (distXZ > MAP_RADIUS) {
                float k = MAP_RADIUS / distXZ;
                camPos.x *= k;
                camPos.z *= k;
            }
            // Sliding: al chocar en diagonal conserva el componente libre.
            bool wasInside    = insideAny(prevPos.x, prevPos.z);
            bool willBeInside = insideAny(camPos.x, camPos.z);
            if (!wasInside && willBeInside) {
                if      (!insideAny(camPos.x, prevPos.z)) camPos.z = prevPos.z;
                else if (!insideAny(prevPos.x, camPos.z)) camPos.x = prevPos.x;
                else                                      camPos   = prevPos;
            }
            camera.position.x = camPos.x;
            camera.position.z = camPos.z;
        }

        // ---- Procesar disparo ----
        if (disparoRealizado && gameStarted) {
            disparoRealizado = false;

            if (!camaraCenital) {
                recoilTimer = RECOIL_DURATION;
                flashTimer  = FLASH_DURATION;

                glm::vec3 origenRayo    = camera.position;
                glm::vec3 direccionRayo = glm::normalize(camera.front);
                const float ANGULO_MAX  = glm::radians(5.0f);

                for (int i = 0; i < NUM_PATOS; i++) {
                    if (patos[i].estado != VOLANDO) continue;
                    float px = RADIO_ORBITA * cos(patos[i].angulo);
                    float pz = RADIO_ORBITA * sin(patos[i].angulo);
                    glm::vec3 posPato(px, patos[i].altura, pz);
                    glm::vec3 vecCP  = glm::normalize(posPato - origenRayo);
                    float angulo     = acos(glm::clamp(glm::dot(direccionRayo, vecCP), -1.0f, 1.0f));
                    float distancia  = glm::length(posPato - origenRayo);

                    if (angulo < ANGULO_MAX && distancia < 400.0f) {
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

        if (gameStarted) {
            // ---- Actualizar patos ----
            for (int i = 0; i < NUM_PATOS; i++) {
                if (patos[i].estado == VOLANDO) {
                    patos[i].angulo += patos[i].velocidad * deltaTime;
                    if (patos[i].angulo > glm::two_pi<float>())
                        patos[i].angulo -= glm::two_pi<float>();

                    angulosRecorridos[i] += patos[i].velocidad * deltaTime;
                    if (angulosRecorridos[i] >= glm::two_pi<float>() * 2.0f) {
                        patos[i].estado = MUERTO;
                        patosEscapadosRonda++;
                        estadoPerro = PERRO_RIENDO;
                        timerPerro  = 0.0f;
                    }
                } else if (patos[i].estado == CAYENDO) {
                    patos[i].velocidadCaida -= 15.0f * deltaTime;
                    patos[i].altura         += patos[i].velocidadCaida * deltaTime;
                    if (patos[i].altura <= groundHeight - 2.0f)
                        patos[i].estado = MUERTO;
                }
            }

            if (recoilTimer > 0.0f) recoilTimer = std::max(0.0f, recoilTimer - deltaTime);
            if (flashTimer  > 0.0f) flashTimer  = std::max(0.0f, flashTimer  - deltaTime);

            // ---- Detectar fin de ronda ----
            if (!enPausaRonda) {
                int resueltos = 0;
                for (int i = 0; i < NUM_PATOS; i++)
                    if (patos[i].estado == MUERTO) resueltos++;
                if (resueltos == NUM_PATOS) { enPausaRonda = true; timerPausaRonda = 0.0f; }
            }
            if (enPausaRonda) {
                timerPausaRonda += deltaTime;
                if (timerPausaRonda >= 3.0f) iniciarNuevaRonda();
            }

            // ---- Actualizar nubes ----
            for (int i = 0; i < NUM_NUBES; i++) {
                nubeOffsets[i] += nubeSpeeds[i] * deltaTime;
                if (nubeOffsets[i] >  CLOUD_X_RANGE) nubeOffsets[i] -= 2.0f * CLOUD_X_RANGE;
                if (nubeOffsets[i] < -CLOUD_X_RANGE) nubeOffsets[i] += 2.0f * CLOUD_X_RANGE;
            }

            // ---- Actualizar invitación cayendo ----
            if (inviteFalling) {
                inviteTimer += deltaTime;
                if (inviteTimer >= INVITE_FALL_DURATION) {
                    inviteFalling = false;
                    inviteTimer   = 0.0f;
                }
            }

            // ---- Máquina de estados del perro ----
            timerPerro += deltaTime;
            animTime   += deltaTime;

            switch (estadoPerro) {
                case PERRO_BUSCANDO:
                    // El perro camina detrás del jugador, fuera del ángulo de visión.
                    posicionActualPerro = camera.position
                        - glm::normalize(glm::vec3(camera.front.x, 0.0f, camera.front.z)) * 15.0f;
                    posicionActualPerro.y = groundHeight - 1.0f;
                    rotacionPerro = glm::degrees(atan2(camera.front.x, camera.front.z));
                    articulacionPatas    = sin(animTime *  8.0f) * 30.0f;
                    articulacionCola     = sin(animTime * 15.0f) * 20.0f;
                    articulacionCabeza   = sin(animTime *  2.0f) * 10.0f;
                    articulacionCuerpoY  = fabs(sin(animTime * 8.0f)) * 0.1f;
                    break;

                case PERRO_ENCONTRADO:
                    // El perro corre hacia donde cayó el pato.
                    posicionActualPerro = glm::normalize(glm::vec3(posImpactoPato.x, 0.0f, posImpactoPato.z)) * 60.0f;
                    posicionActualPerro.y = groundHeight - 1.0f;
                    rotacionPerro = glm::degrees(atan2(posicionActualPerro.x, posicionActualPerro.z));
                    articulacionPatas    = sin(animTime * 12.0f) * 40.0f;
                    articulacionCola     = sin(animTime * 30.0f) * 40.0f;
                    articulacionCabeza   = -15.0f;
                    articulacionCuerpoY  = fabs(sin(animTime * 12.0f)) * 0.2f;
                    if (timerPerro >= 1.5f) { estadoPerro = PERRO_MOSTRANDO; timerPerro = 0.0f; }
                    break;

                case PERRO_MOSTRANDO:
                    // El perro con patos aparece animado (RISE/HOLD/FALL).
                    // La duración total es DOG_TOTAL_TIME = 3.0 s.
                    rotacionPerro = glm::degrees(atan2(posicionActualPerro.x, posicionActualPerro.z));
                    articulacionPatas   = 0.0f;
                    articulacionCola    = sin(animTime * 5.0f) * 10.0f;
                    articulacionCabeza  = 20.0f;
                    articulacionCuerpoY = 0.0f;
                    if (timerPerro >= DOG_TOTAL_TIME) {
                        estadoPerro  = PERRO_BUSCANDO;
                        timerPerro   = 0.0f;
                        patosEnRonda = 0;
                    }
                    break;

                case PERRO_RIENDO: {
                    glm::vec3 dir = glm::normalize(glm::vec3(camera.front.x, 0.0f, camera.front.z));
                    posicionActualPerro = camera.position + dir * 60.0f;
                    float riseP = glm::clamp(timerPerro, 0.0f, 1.0f);
                    float shake = sinf(timerPerro * 30.0f) * 0.15f;
                    posicionActualPerro.y = groundHeight - 10.0f + riseP * 2.0f + shake;
                    rotacionPerro = glm::degrees(atan2(dir.x, dir.z)) + 180.0f;
                    articulacionPatas = articulacionCola = articulacionCabeza = articulacionCuerpoY = 0.0f;
                    if (timerPerro >= 3.5f) { estadoPerro = PERRO_BUSCANDO; timerPerro = 0.0f; }
                    break;
                }
            }
        } else {
            disparoRealizado = false;
        }

        // ---- Aplicar deltas acumulados del mouse ----
        if (!camaraCenital) {
            camera.ProcessMouseMovement(
                glm::clamp((GLfloat)mouseAccX, -30.0f, 30.0f),
                glm::clamp((GLfloat)-mouseAccY, -30.0f, 30.0f));
        }
        mouseAccX = 0.0;
        mouseAccY = 0.0;

        // ---- Limpiar pantalla (azul cielo del compañero) ----
        glClearColor(0.45f, 0.65f, 0.85f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ---- Calcular view matrix ----
        glm::mat4 view;
        if (camaraCenital) {
            view = glm::lookAt(glm::vec3(0.0f, 500.0f, 0.0f),
                               glm::vec3(0.0f,   0.0f, 0.0f),
                               glm::vec3(0.0f,   0.0f,-1.0f));
        } else {
            view = camera.GetViewMatrix();
        }

        // =====================================================================
        //   RENDER DE ESCENA ESTÁTICA (sceneShader — flat textured, del compañero)
        // =====================================================================
        sceneShader.Use();
        glUniformMatrix4fv(su.view, 1, GL_FALSE, glm::value_ptr(view));

        // Terreno base
        drawAt(terreno,  glm::vec3(-12.0f, 0.0f, -6.0f));
        drawAt(pasto,    glm::vec3( -4.0f, 0.0f, -6.0f));
        drawAt(fango,    glm::vec3(  4.0f, 0.0f, -6.0f));
        drawAt(arboles,  glm::vec3( 12.0f, 0.0f, -6.0f));
        drawAt(arbustos, glm::vec3( -8.0f, 0.0f,  6.0f));

        // Props decorativos (posicionados en su lugar desde Blender)
        drawAt(lago,  glm::vec3(0.0f, 0.0f, 0.0f));
        drawAt(rocas, glm::vec3(0.0f, 0.0f, 0.0f));
        drawAt(tazon, glm::vec3(0.0f, 0.0f, 0.0f));

        // ---- Invitación Smash cayendo (tecla I) ----
        if (inviteFalling) {
            float t    = inviteTimer / INVITE_FALL_DURATION;
            float fall = 1.0f - (1.0f - t) * (1.0f - t);        // ease-out cuadrático
            float dY   = INVITE_SPAWN_Y * (1.0f - fall);
            float dec  = 1.0f - t;
            float dX   = INVITE_TURBULENCE * dec * sinf(t * 9.0f);
            float dZ   = INVITE_TURBULENCE * dec * cosf(t * 7.0f);
            float yaw  = INVITE_TURNS * 6.2831853f * t;
            float tumble = 0.6f * sinf(t * 10.0f) * dec;

            glm::vec3 pivotLocal = (invitacion.bboxMin + invitacion.bboxMax) * 0.5f;
            glm::vec3 pivotW     = pivotLocal * SCENE_SCALE;

            glm::mat4 m(1.0f);
            m = glm::translate(m, glm::vec3(dX, dY, dZ));
            m = glm::translate(m,  pivotW);
            m = glm::rotate(m, yaw,    glm::vec3(0.0f, 1.0f, 0.0f));
            m = glm::rotate(m, tumble, glm::vec3(1.0f, 0.0f, 0.0f));
            m = glm::translate(m, -pivotW);
            m = glm::scale(m, glm::vec3(SCENE_SCALE));
            glUniformMatrix4fv(su.model, 1, GL_FALSE, glm::value_ptr(m));
            invitacion.Draw(sceneShader);
        }

        // ---- Perro con patos (siempre renderizado, DOG_HIDE_Y salvo en MOSTRANDO) ----
        // Cuando no está en MOSTRANDO el modelo queda 200 u bajo tierra = invisible.
        if (gameStarted) {
            float dogWorldY = DOG_HIDE_Y;
            if (estadoPerro == PERRO_MOSTRANDO) {
                float t = timerPerro;
                if (t < DOG_RISE_TIME) {
                    float r = t / DOG_RISE_TIME;
                    float e = r * r * (3.0f - 2.0f * r);          // smoothstep
                    dogWorldY = DOG_HIDE_Y + e * (0.0f - DOG_HIDE_Y);
                } else if (t < DOG_RISE_TIME + DOG_HOLD_TIME) {
                    dogWorldY = 0.0f;
                } else {
                    float r = (t - DOG_RISE_TIME - DOG_HOLD_TIME) / DOG_FALL_TIME;
                    float e = r * r * (3.0f - 2.0f * r);
                    dogWorldY = e * DOG_HIDE_Y;
                }
            }
            glm::mat4 dm(1.0f);
            dm = glm::translate(dm, glm::vec3(0.0f, dogWorldY, 0.0f));
            dm = glm::scale(dm, glm::vec3(SCENE_SCALE));
            glUniformMatrix4fv(su.model, 1, GL_FALSE, glm::value_ptr(dm));
            perroPatos.Draw(sceneShader);
        }

        // ---- Nubes (7, con wrap-around en X) ----
        for (int i = 0; i < NUM_NUBES; i++) {
            glm::mat4 mm(1.0f);
            mm = glm::translate(mm, glm::vec3(nubeOffsets[i], 0.0f, 0.0f));
            mm = glm::scale(mm, glm::vec3(SCENE_SCALE));
            glUniformMatrix4fv(su.model, 1, GL_FALSE, glm::value_ptr(mm));
            nubes[i]->Draw(sceneShader);
        }

        // =====================================================================
        //   RENDER DE PATOS ANIMADOS (animShader — skinning + Phong)
        // =====================================================================
        animShader.Use();
        glUniformMatrix4fv(au.view,  1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(au.viewPos,     1, glm::value_ptr(camera.position));

        for (int i = 0; i < NUM_PATOS; i++) {
            if (patos[i].estado == MUERTO) continue;
            float px  = RADIO_ORBITA * cos(patos[i].angulo);
            float pz  = RADIO_ORBITA * sin(patos[i].angulo);
            float py  = patos[i].altura;
            float yaw = patos[i].angulo + glm::half_pi<float>();

            glm::mat4 matPato = glm::translate(glm::mat4(1.0f), glm::vec3(px, py, pz));
            matPato = glm::rotate(matPato, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
            matPato = glm::scale(matPato, glm::vec3(0.30f));
            matPato = glm::translate(matPato, -PatoAnimado.boundingCenter);

            glUniformMatrix4fv(au.model, 1, GL_FALSE, glm::value_ptr(matPato));
            PatoAnimado.Draw(animShader);
        }

        // =====================================================================
        //   RENDER DE OBJETOS DEL JUEGO (lightingShader — Phong)
        // =====================================================================
        lightingShader.Use();
        glUniformMatrix4fv(lu.view,   1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(lu.viewPos,      1, glm::value_ptr(camera.position));

        // ---- Skybox ----
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        {
            glm::mat4 skyView = glm::mat4(glm::mat3(view)); // sin traslación
            glUniformMatrix4fv(lu.view, 1, GL_FALSE, glm::value_ptr(skyView));
            glm::mat4 skyModel = glm::scale(glm::mat4(1.0f), glm::vec3(600.0f));
            drawWorld(Skybox, skyModel, glm::mat3(1.0f), true, false);
        }
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
        glUniformMatrix4fv(lu.view, 1, GL_FALSE, glm::value_ptr(view));
        glUniform1i(lu.transparency, 1);

        // ---- Cazador (solo en cámara cenital) ----
        if (camaraCenital) {
            glm::mat4 matCaz = glm::translate(glm::mat4(1.0f),
                camera.position - glm::vec3(0.0f, 0.8f, 0.0f));
            float yawCaz = atan2(camera.front.x, camera.front.z);
            matCaz = glm::rotate(matCaz, yawCaz, glm::vec3(0.0f, 1.0f, 0.0f));
            matCaz = glm::scale(matCaz, glm::vec3(1.2f));
            drawWorld(Cazador, matCaz, glm::mat3(1.0f), true, true);
        }

        // ---- Perro jerárquico (BUSCANDO y ENCONTRADO) ----
        if (estadoPerro != PERRO_MOSTRANDO && estadoPerro != PERRO_RIENDO) {
            // Escala del perro ajustada al nuevo mundo (modelo OBJ pequeño en escena grande).
            // Ajustar visualmente si el perro se ve muy grande o pequeño.
            const float DOG_SCALE = 15.0f;
            glm::mat4 modelCuerpo = glm::translate(glm::mat4(1.0f),
                posicionActualPerro + glm::vec3(0.0f, articulacionCuerpoY, 0.0f));
            modelCuerpo = glm::rotate(modelCuerpo, glm::radians(rotacionPerro), glm::vec3(0.0f, 1.0f, 0.0f));
            modelCuerpo = glm::scale(modelCuerpo, glm::vec3(DOG_SCALE));
            drawWorld(DogBody, modelCuerpo, glm::mat3(1.0f), true, true);

            glm::mat4 modelCabeza = modelCuerpo;
            modelCabeza = glm::translate(modelCabeza, glm::vec3(0.0f, 0.093f, 0.208f));
            modelCabeza = glm::rotate(modelCabeza, glm::radians(articulacionCabeza), glm::vec3(1.0f, 0.0f, 0.0f));
            drawWorld(DogHead, modelCabeza, glm::mat3(1.0f), true, true);

            glm::mat4 modelCola = modelCuerpo;
            modelCola = glm::translate(modelCola, glm::vec3(0.0f, 0.026f, -0.288f));
            modelCola = glm::rotate(modelCola, glm::radians(articulacionCola), glm::vec3(0.0f, 1.0f, 0.0f));
            drawWorld(DogTail, modelCola, glm::mat3(1.0f), true, true);

            glm::mat4 modelFL = modelCuerpo;
            modelFL = glm::translate(modelFL, glm::vec3( 0.112f, -0.044f, 0.074f));
            modelFL = glm::rotate(modelFL, glm::radians( articulacionPatas), glm::vec3(1.0f, 0.0f, 0.0f));
            drawWorld(DogLegFL, modelFL, glm::mat3(1.0f), true, true);

            glm::mat4 modelFR = modelCuerpo;
            modelFR = glm::translate(modelFR, glm::vec3(-0.111f, -0.055f, 0.074f));
            modelFR = glm::rotate(modelFR, glm::radians(-articulacionPatas), glm::vec3(1.0f, 0.0f, 0.0f));
            drawWorld(DogLegFR, modelFR, glm::mat3(1.0f), true, true);

            glm::mat4 modelBL = modelCuerpo;
            modelBL = glm::translate(modelBL, glm::vec3( 0.082f, -0.046f,-0.218f));
            modelBL = glm::rotate(modelBL, glm::radians(-articulacionPatas), glm::vec3(1.0f, 0.0f, 0.0f));
            drawWorld(DogLegBL, modelBL, glm::mat3(1.0f), true, true);

            glm::mat4 modelBR = modelCuerpo;
            modelBR = glm::translate(modelBR, glm::vec3(-0.083f, -0.057f,-0.231f));
            modelBR = glm::rotate(modelBR, glm::radians( articulacionPatas), glm::vec3(1.0f, 0.0f, 0.0f));
            drawWorld(DogLegBR, modelBR, glm::mat3(1.0f), true, true);
        }

        // ---- Perro riendo (RIENDO) ----
        if (estadoPerro == PERRO_RIENDO) {
            glm::mat4 modelRisa = glm::translate(glm::mat4(1.0f), posicionActualPerro);
            modelRisa = glm::rotate(modelRisa, glm::radians(rotacionPerro), glm::vec3(0.0f, 1.0f, 0.0f));
            modelRisa = glm::scale(modelRisa, glm::vec3(15.0f));
            sceneShader.Use();
            glUniformMatrix4fv(su.view,  1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(su.model, 1, GL_FALSE, glm::value_ptr(modelRisa));
            glDisable(GL_CULL_FACE);
            PerroRiendo.Draw(sceneShader);
            glEnable(GL_CULL_FACE);
            lightingShader.Use();
        }

        // ---- Efecto de plumas ----
        if (efectoPlumas.activo) {
            efectoPlumas.timer += deltaTime;
            efectoPlumas.frame  = (int)(efectoPlumas.timer / (DURACION_PLUMAS / 5.0f));
            if (efectoPlumas.timer >= DURACION_PLUMAS) {
                efectoPlumas.activo = false;
            } else {
                glm::mat4 matPlumas = glm::translate(glm::mat4(1.0f), efectoPlumas.posicion);
                matPlumas = glm::rotate(matPlumas, efectoPlumas.timer * 3.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                matPlumas = glm::scale(matPlumas, glm::vec3(20.0f));
                int fi = glm::clamp(efectoPlumas.frame, 0, 4);
                drawWorld(*framesPlumas[fi], matPlumas, glm::mat3(1.0f), true, true);
            }
        }

        // =====================================================================
        //   HUD (hudShader — sin profundidad, screen-space)
        // =====================================================================
        if (!camaraCenital) {
            glClear(GL_DEPTH_BUFFER_BIT);
            hudShader.Use();
            glm::mat4 hudView = glm::mat4(1.0f);
            glUniformMatrix4fv(hu.projection, 1, GL_FALSE, glm::value_ptr(projection));
            glUniformMatrix4fv(hu.view,       1, GL_FALSE, glm::value_ptr(hudView));

            // Recoil: ease-out cuadrático (kick brusco, regreso suave)
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

            // Flash de disparo (additive blending, dos capas)
            if (flashTimer > 0.0f) {
                float t  = flashTimer / FLASH_DURATION;
                float ar = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
                glDisable(GL_DEPTH_TEST);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, flashTexture);
                glUniform1i(hu.texture, 0);
                const glm::vec3 BARREL_POS(0.5f, -0.68f, -1.0f);
                glm::vec3 bPos = BARREL_POS + glm::vec3(0.0f, -recoil * 0.04f, recoil * 0.1f);
                const float sizes[2]     = { t * 0.14f, t * 0.07f };
                const float rotSpeeds[2] = { 47.0f, 73.0f };
                for (int layer = 0; layer < 2; layer++) {
                    float angle = (float)glfwGetTime() * rotSpeeds[layer];
                    glm::mat4 mFlash = glm::translate(glm::mat4(1.0f), bPos);
                    mFlash = glm::rotate(mFlash, angle, glm::vec3(0.0f, 0.0f, 1.0f));
                    mFlash = glm::scale(mFlash, glm::vec3(sizes[layer] / ar, sizes[layer], 1.0f));
                    glUniformMatrix4fv(hu.model, 1, GL_FALSE, glm::value_ptr(mFlash));
                    glBindVertexArray(flashVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
                glBindVertexArray(0);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }

            // Crosshair
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            {
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
            }

            // Marcador 3D (ronda y puntaje)
            glDisable(GL_DEPTH_TEST);
            if (gameStarted) {
                renderizarHUD3D(digitoModels[glm::clamp(rondaActual,     0, 8)], -0.80f, 0.85f, 64.0f, glm::vec3(1.0f, 1.0f, 0.0f));
                renderizarHUD3D(digitoModels[glm::clamp(patosDerribados, 0, 8)],  0.75f, 0.85f, 64.0f, glm::vec3(1.0f, 1.0f, 0.0f));
                float escalaBarra = 32.0f + (rondaActual - 1) * 4.0f;
                renderizarHUD3D(&BarDificultad, -0.80f, 0.65f, escalaBarra, glm::vec3(1.0f, 0.2f, 0.2f));
            } else {
                renderizarHUD3D(&LogoJuego,  -0.20f,  0.25f, 250.0f, glm::vec3(1.0f, 1.0f, 1.0f));
                if (fmod(glfwGetTime(), 1.0) < 0.5)
                    renderizarHUD3D(&StartPrompt, 0.0f, -0.35f, 80.0f, glm::vec3(1.0f, 1.0f, 1.0f));
            }

            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
        }

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

// =============================================================================
//   MOVIMIENTO DEL JUGADOR
// =============================================================================
void DoMovement()
{
    // Factor de velocidad adaptado al nuevo mundo (radio 540 vs antiguo 22).
    // SHIFT duplica la velocidad para cruzar el mapa rápido.
    const float speed = keys[GLFW_KEY_LEFT_SHIFT] ? deltaTime * 30.0f
                                                   : deltaTime * 15.0f;
    if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])    camera.ProcessKeyboard(FORWARD,  speed);
    if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN])  camera.ProcessKeyboard(BACKWARD, speed);
    if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])  camera.ProcessKeyboard(LEFT,     speed);
    if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT]) camera.ProcessKeyboard(RIGHT,    speed);

    if (keys[GLFW_KEY_SPACE] && isGrounded) {
        velocityY  = jumpForce;
        isGrounded = false;
    }

    // El límite exterior (MAP_RADIUS) se aplica en el game loop.
    // No hay restricción interna: el jugador puede recorrer todo el mapa.
}

// =============================================================================
//   CALLBACKS DE TECLADO
// =============================================================================
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Iniciar el juego al presionar ENTER o cualquier tecla de movimiento.
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ENTER || key == GLFW_KEY_W || key == GLFW_KEY_A
            || key == GLFW_KEY_S  || key == GLFW_KEY_D)
            gameStarted = true;
    }

    // C: alternar entre cámara FPS y cenital.
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
        camaraCenital = !camaraCenital;

    // N: toggle movimiento de nubes.
    if (key == GLFW_KEY_N && action == GLFW_PRESS)
        cloudsMoving = !cloudsMoving;

    // I: lanzar la invitación de Smash desde el cielo.
    if (key == GLFW_KEY_I && action == GLFW_PRESS && !inviteFalling) {
        inviteFalling = true;
        inviteTimer   = 0.0f;
    }

    // R: [DEBUG] forzar perro riendo para validar animación.
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        estadoPerro = PERRO_RIENDO;
        timerPerro  = 0.0f;
        gameStarted = true;
        std::cout << ">>> [DEBUG] PERRO_RIENDO activado\n";
    }

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS)   keys[key] = true;
        if (action == GLFW_RELEASE) keys[key] = false;
    }
}

// =============================================================================
//   LÓGICA DEL JUEGO
// =============================================================================
void perroReaccionarDisparo(glm::vec3 posImpacto) {
    estadoPerro    = PERRO_ENCONTRADO;
    timerPerro     = 0.0f;
    posImpactoPato = posImpacto;
    patosEnRonda++;
    patosDerribadosRonda++;
}

void iniciarNuevaRonda() {
    rondaActual++;
    patosDerribadosRonda = 0;
    patosEscapadosRonda  = 0;
    enPausaRonda         = false;
    timerPausaRonda      = 0.0f;

    velocidadBaseRonda = glm::min(0.6f + (rondaActual - 1) * 0.15f, 1.5f);

    for (int i = 0; i < NUM_PATOS; i++) {
        patos[i].angulo         = glm::radians(120.0f * i);
        patos[i].velocidad      = velocidadBaseRonda + i * 0.05f;
        patos[i].altura         = ALTURA_VUELO + i * 0.5f;
        patos[i].estado         = VOLANDO;
        patos[i].velocidadCaida = 0.0f;
        angulosRecorridos[i]    = 0.0f;
    }

    estadoPerro  = PERRO_BUSCANDO;
    timerPerro   = 0.0f;
    patosEnRonda = 0;
}
