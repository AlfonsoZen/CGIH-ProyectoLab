// =================================================================
//  SNIPPET PATO VOLADOR
//  Copiar / pegar las secciones marcadas en su archivo .cpp principal
// =================================================================
//
//  ⚠️  ESTE ARCHIVO NO ES COMPILABLE POR SÍ SOLO. ⚠️
//  No agregarlo al .vcxproj ni intentar compilarlo aparte — es código
//  de ejemplo organizado en 4 bloques que se pegan en su .cpp principal
//  (el que tiene int main() y el game loop del juego).
//
//  Animación: el pato avanza recto en +Z, se detiene al final del recorrido,
//  gira 180° sobre su propio eje (con easing suave), avanza de regreso en -Z,
//  gira otros 180° y vuelve a empezar — loop infinito, siempre de frente.
//
// =================================================================


// ---------- 1) AGREGAR ESTE INCLUDE EN EL HEADER DEL ARCHIVO ----------
#include "modelAnim.h"


// ---------- 2) VARIABLES GLOBALES (junto a las demás globales) ----------
// Trayectoria del pato (ajustables a su escenario)
float cycleTime          = 0.0f;                       // tiempo dentro del ciclo
float duckSpeed          = 1.5f;                       // metros / segundo
float zStart             = -5.0f;                      // extremo cercano del recorrido
float zEnd               =  5.0f;                      // extremo lejano del recorrido
float spinTime           = 1.8f;                       // segundos del giro de 180°
float duckYawOffset      = 0.0f;                       // 0 si el modelo apunta a +Z
glm::vec3 pivotOverride(0.0f, 0.0f, 0.0f);             // (0,0,0) = pivote automático
glm::vec3 trajectoryCenter(0.0f, 1.6f, 0.0f);          // X y altura Y donde vuela el pato


// ---------- 3) DENTRO DE main(), DESPUES DE INIT GLEW ----------
//             Y JUNTO A LA CARGA DE OTROS MODELOS Y SHADERS
Shader animShader("Shader/anim.vs", "Shader/anim.frag");

std::cout << ">>> Cargando Pato.fbx ..." << std::endl;
ModelAnim Pato((char*)"Models/Pato.fbx");
Pato.initShaders(animShader.Program);
std::cout << ">>> Pato listo" << std::endl;


// ---------- 4) DENTRO DEL game loop, ANTES DE glfwSwapBuffers ----------
//             (asume que ya tienen `view`, `projection`, `camera` definidos)
{
    // Avance del ciclo (recta + giro + recta + giro)
    const float PI = 3.14159265f;
    float tStraight  = (zEnd - zStart) / duckSpeed;
    float totalCycle = 2.0f * tStraight + 2.0f * spinTime;
    cycleTime = fmodf(cycleTime + deltaTime, totalCycle);

    // Calcular posición y orientación según la fase
    float px = trajectoryCenter.x;
    float py = trajectoryCenter.y;
    float pz, yaw;

    if (cycleTime < tStraight) {
        // FASE 1: avanza recto +Z, mirando +Z
        float t = cycleTime / tStraight;
        pz = zStart + t * (zEnd - zStart);
        yaw = 0.0f;
    }
    else if (cycleTime < tStraight + spinTime) {
        // FASE 2: gira 180° en zEnd (easing smoothstep)
        float t = (cycleTime - tStraight) / spinTime;
        float eased = t * t * (3.0f - 2.0f * t);
        pz = zEnd;
        yaw = eased * PI;
    }
    else if (cycleTime < 2.0f * tStraight + spinTime) {
        // FASE 3: avanza recto -Z, mirando -Z
        float t = (cycleTime - tStraight - spinTime) / tStraight;
        pz = zEnd - t * (zEnd - zStart);
        yaw = PI;
    }
    else {
        // FASE 4: gira 180° en zStart (easing)
        float t = (cycleTime - 2.0f * tStraight - spinTime) / spinTime;
        float eased = t * t * (3.0f - 2.0f * t);
        pz = zStart;
        yaw = PI + eased * PI;
    }

    yaw += glm::radians(duckYawOffset);

    // Activar el shader animado y pasar uniforms
    animShader.Use();
    GLint modelLoc = glGetUniformLocation(animShader.Program, "model");
    GLint viewLoc  = glGetUniformLocation(animShader.Program, "view");
    GLint projLoc  = glGetUniformLocation(animShader.Program, "projection");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Iluminación direccional simple (ajustar a la luz de su escenario)
    glUniform3f(glGetUniformLocation(animShader.Program, "viewPos"),
                camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z);
    glUniform3f(glGetUniformLocation(animShader.Program, "light.direction"), -0.3f, -1.0f, -0.4f);
    glUniform3f(glGetUniformLocation(animShader.Program, "light.ambient"),   0.85f, 0.85f, 0.85f);
    glUniform3f(glGetUniformLocation(animShader.Program, "light.diffuse"),   0.9f,  0.9f,  0.9f);
    glUniform3f(glGetUniformLocation(animShader.Program, "light.specular"),  0.4f,  0.4f,  0.4f);
    glUniform3f(glGetUniformLocation(animShader.Program, "material.specular"), 0.5f, 0.5f, 0.5f);
    glUniform1f(glGetUniformLocation(animShader.Program, "material.shininess"), 32.0f);

    // Construir matriz model con pivote al centro real del cuerpo animado
    glm::vec3 pivot = (pivotOverride == glm::vec3(0.0f)) ? Pato.boundingCenter : pivotOverride;
    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(px, py, pz));
    model = glm::rotate(model, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.012f));      // tamaño "pajarón"
    model = glm::translate(model, -pivot);             // gira sobre su eje real
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    Pato.Draw(animShader);
}
