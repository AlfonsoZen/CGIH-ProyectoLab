// =============================================================================
//   SNIPPET PATO 2 VOLADOR
//   Copiar / pegar las secciones marcadas en el .cpp principal del proyecto
// =============================================================================
//
//   ESTE ARCHIVO NO ES COMPILABLE POR SI SOLO. NO agregarlo al .vcxproj ni
//   intentar compilarlo aparte. Son 4 bloques de codigo que se pegan dentro
//   del .cpp que tiene int main() y el game loop.
//
//   Animacion: el pato2 vuela en TRAYECTORIA CUADRADA en el plano XZ. En
//   cada esquina hace un giro de 90 grados con derrape suave (curva bezier)
//   y banking lateral como un avion. Siempre mira hacia adelante. Loop
//   infinito.
//
//   La textura esta EMBEBIDA en el .fbx, asi que no hay que copiar archivos
//   de textura sueltos. modelAnim.h v1.1+ la extrae automaticamente.
//
// =============================================================================


// ---------- 1) INCLUDE EN EL HEADER DEL ARCHIVO -----------------------------
#include "modelAnim.h"


// ---------- 2) VARIABLES GLOBALES (junto a las demas globales) --------------
// Parametros principales
float p2_sideLength      = 16.0f;   // Tamanio del cuadrado (lado, en metros)
float p2_centerY         = 6.0f;    // Altura de vuelo
float p2_speed           = 5.0f;    // Velocidad de avance (m/s)
float p2_scale           = 0.01f;   // Tamanio del modelo

// Parametros de giro y derrape
float p2_spinTime        = 0.7f;    // Segundos del giro de 90 en cada esquina
float p2_cornerDrift     = 1.8f;    // Metros de "redondeo" en cada esquina
float p2_maxBankDeg      = 22.0f;   // Inclinacion lateral maxima al girar

// Ajustes del modelo (normalmente no hace falta tocar)
float p2_yawOffset       = 0.0f;
float p2_axisFixYaw      = 0.0f;    // Probar 90/180/270 si vuela de lado
float p2_axisFixPitch    = 0.0f;
float p2_axisFixRoll     = 0.0f;
int   p2_rollSign        = -1;
glm::vec3 p2_pivotOverride(0.0f);

// Centro de la trayectoria (X y Z donde queda el centro del cuadrado)
glm::vec3 p2_trajectoryCenter(0.0f, 0.0f, 0.0f);

// Estado interno (no tocar)
float p2_cycleTime = 0.0f;


// ---------- 3) CARGA DEL MODELO Y SHADER (dentro de main(), -----------------
//             despues de init GLEW, junto a los otros modelos y shaders)
//
// NOTA: si el proyecto ya tiene integrado el modulo del Pato o del Aguila,
// NO redeclarar `animShader` — ya existe y se reusa. Solo cargar el Pato2 y
// pasarlo al shader existente: `Pato2.initShaders(animShader.Program);`
Shader animShader("Shader/anim.vs", "Shader/anim.frag");

std::cout << ">>> Cargando Pato2.fbx ..." << std::endl;
ModelAnim Pato2((char*)"Models/Pato2.fbx");
Pato2.initShaders(animShader.Program);
std::cout << ">>> Pato2 listo" << std::endl;


// ---------- 4) DENTRO DEL GAME LOOP, ANTES DE glfwSwapBuffers --------------
//             (asume que ya estan definidos: view, projection, camera, deltaTime)
{
    const float PI = 3.14159265f;

    // -- Esquinas del cuadrado (sentido horario visto desde arriba) --
    float S = p2_sideLength * 0.5f;
    glm::vec3 corners[4] = {
        p2_trajectoryCenter + glm::vec3(-S, p2_centerY, -S),
        p2_trajectoryCenter + glm::vec3(+S, p2_centerY, -S),
        p2_trajectoryCenter + glm::vec3(+S, p2_centerY, +S),
        p2_trajectoryCenter + glm::vec3(-S, p2_centerY, +S)
    };
    // Yaw que orienta al pato2 en cada lado
    float yawForLeg[4] = { PI * 0.5f, 0.0f, -PI * 0.5f, -PI };
    // Direcciones unitarias de cada lado
    glm::vec3 legDir[4];
    for (int k = 0; k < 4; k++) {
        legDir[k] = glm::normalize(corners[(k + 1) % 4] - corners[k]);
    }

    // -- Tiempo de cada fase del ciclo --
    float legTime    = (p2_sideLength - 2.0f * p2_cornerDrift) / p2_speed;
    float totalCycle = 4.0f * legTime + 4.0f * p2_spinTime;
    p2_cycleTime     = fmodf(p2_cycleTime + deltaTime, totalCycle);

    // -- Resolver fase actual (recta o esquina) --
    glm::vec3 pos(0.0f);
    float yaw  = 0.0f;
    float roll = 0.0f;
    float tau  = p2_cycleTime;
    for (int k = 0; k < 4; k++) {
        // Fase recta
        if (tau < legTime) {
            float r = tau / legTime;
            glm::vec3 from = corners[k] + legDir[k] * p2_cornerDrift;
            glm::vec3 to   = corners[(k + 1) % 4] - legDir[k] * p2_cornerDrift;
            pos  = glm::mix(from, to, r);
            yaw  = yawForLeg[k];
            roll = 0.0f;
            break;
        }
        tau -= legTime;
        // Fase esquina (bezier + giro + banking)
        if (tau < p2_spinTime) {
            float r = tau / p2_spinTime;
            float eased = r * r * (3.0f - 2.0f * r);
            glm::vec3 corner = corners[(k + 1) % 4];
            glm::vec3 P0 = corner - legDir[k] * p2_cornerDrift;
            glm::vec3 P1 = corner;
            glm::vec3 P2 = corner + legDir[(k + 1) % 4] * p2_cornerDrift;
            float u = 1.0f - r;
            pos  = u * u * P0 + 2.0f * u * r * P1 + r * r * P2;
            yaw  = yawForLeg[k] + eased * (-PI * 0.5f);
            roll = (float)p2_rollSign * glm::radians(p2_maxBankDeg) * sinf(PI * r);
            break;
        }
        tau -= p2_spinTime;
    }
    yaw += glm::radians(p2_yawOffset);

    // -- Activar shader animado y pasar uniforms --
    animShader.Use();
    GLint modelLoc = glGetUniformLocation(animShader.Program, "model");
    glUniformMatrix4fv(glGetUniformLocation(animShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(animShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glUniform3f(glGetUniformLocation(animShader.Program, "viewPos"),
                camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z);
    glUniform3f(glGetUniformLocation(animShader.Program, "light.direction"), -0.3f, -1.0f, -0.4f);
    glUniform3f(glGetUniformLocation(animShader.Program, "light.ambient"),   0.85f, 0.85f, 0.85f);
    glUniform3f(glGetUniformLocation(animShader.Program, "light.diffuse"),   0.9f,  0.9f,  0.9f);
    glUniform3f(glGetUniformLocation(animShader.Program, "light.specular"),  0.4f,  0.4f,  0.4f);
    glUniform3f(glGetUniformLocation(animShader.Program, "material.specular"), 0.5f, 0.5f, 0.5f);
    glUniform1f(glGetUniformLocation(animShader.Program, "material.shininess"), 32.0f);

    // -- Construir matriz model --
    glm::vec3 pivot = (p2_pivotOverride == glm::vec3(0.0f)) ? Pato2.boundingCenter : p2_pivotOverride;
    glm::mat4 model(1.0f);
    model = glm::translate(model, pos);
    model = glm::rotate(model, yaw,  glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, roll, glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(p2_scale));
    model = glm::rotate(model, glm::radians(p2_axisFixYaw),   glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(p2_axisFixPitch), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(p2_axisFixRoll),  glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::translate(model, -pivot);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    Pato2.Draw(animShader);
}
