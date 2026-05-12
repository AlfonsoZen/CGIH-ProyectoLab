#version 330 core

#define NUMBER_OF_POINT_LIGHTS 4

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float     shininess;
};

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3  position;
    float constant;
    float linear;
    float quadratic;
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
};

struct SpotLight {
    vec3  position;
    vec3  direction;
    float cutOff;
    float outerCutOff;
    float constant;
    float linear;
    float quadratic;
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 color;

uniform vec3      viewPos;
uniform DirLight  dirLight;
uniform PointLight pointLights[NUMBER_OF_POINT_LIGHTS];
uniform SpotLight  spotLight;
uniform Material   material;
uniform int        transparency;
uniform vec3       baseColor;
uniform bool       useTexture;

// diffColor / specColor sampled once in main() and passed down — avoids
// redundant texture fetches inside each light function.
vec3 CalcDirLight  (DirLight   light, vec3 normal, vec3 viewDir, vec3 diffColor, vec3 specColor);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffColor, vec3 specColor);
vec3 CalcSpotLight (SpotLight  light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffColor, vec3 specColor);

void main()
{
    vec3  norm    = normalize(Normal);
    vec3  viewDir = normalize(viewPos - FragPos);

    // Sample textures exactly once per fragment
    vec4 diffSample = useTexture ? texture(material.diffuse,  TexCoords) : vec4(baseColor, 1.0);
    vec3 diffColor  = diffSample.rgb;
    float alpha     = diffSample.a;
    vec3 specColor  = useTexture ? vec3(texture(material.specular, TexCoords)) : vec3(0.5);

    vec3 result = CalcDirLight(dirLight, norm, viewDir, diffColor, specColor);

    // Point lights — skip entirely when disabled (avoids sqrt/pow per fragment)
    for (int i = 0; i < NUMBER_OF_POINT_LIGHTS; i++) {
        if (pointLights[i].diffuse.x + pointLights[i].ambient.x > 0.0)
            result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, diffColor, specColor);
    }

    // Spotlight — same guard
    if (spotLight.diffuse.x + spotLight.ambient.x > 0.0)
        result += CalcSpotLight(spotLight, norm, FragPos, viewDir, diffColor, specColor);

    color = vec4(result, alpha);

    if (color.a < 0.1 && transparency == 1)
        discard;
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 diffColor, vec3 specColor)
{
    vec3  lightDir   = normalize(-light.direction);
    float diff       = max(dot(normal, lightDir), 0.0);
    vec3  reflectDir = reflect(-lightDir, normal);
    float spec       = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    return light.ambient  * diffColor
         + light.diffuse  * diff * diffColor
         + light.specular * spec * specColor;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffColor, vec3 specColor)
{
    vec3  lightDir   = normalize(light.position - fragPos);
    float diff       = max(dot(normal, lightDir), 0.0);
    vec3  reflectDir = reflect(-lightDir, normal);
    float spec       = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    float dist       = length(light.position - fragPos);
    float att        = 1.0 / (light.constant + light.linear * dist + light.quadratic * dist * dist);

    return (light.ambient  * diffColor
          + light.diffuse  * diff * diffColor
          + light.specular * spec * specColor) * att;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffColor, vec3 specColor)
{
    vec3  lightDir   = normalize(light.position - fragPos);
    float diff       = max(dot(normal, lightDir), 0.0);
    vec3  reflectDir = reflect(-lightDir, normal);
    float spec       = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    float dist       = length(light.position - fragPos);
    float att        = 1.0 / (light.constant + light.linear * dist + light.quadratic * dist * dist);
    float theta      = dot(lightDir, normalize(-light.direction));
    float epsilon    = light.cutOff - light.outerCutOff;
    float intensity  = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    return (light.ambient  * diffColor
          + light.diffuse  * diff * diffColor
          + light.specular * spec * specColor) * att * intensity;
}
