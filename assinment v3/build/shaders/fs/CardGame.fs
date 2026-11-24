#version 460 core
out vec4 FragColor;

in vec3 fragPos;
in vec3 fragNormal;
in vec2 texCoord;

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
};

uniform vec3 objectColor;
uniform vec3 tintColor;
uniform sampler2D texture_diffuse1;
uniform bool useTexture;
uniform bool useVertexNormals;
uniform bool isEmissive;
uniform float shininess;
uniform vec3 viewPos;

uniform int numLights;
uniform PointLight lights[2];

void main()
{
    vec3 matColor = objectColor * tintColor;
    vec3 baseColor = useTexture
        ? texture(texture_diffuse1, texCoord).rgb * matColor
        : matColor;

    if (isEmissive)
    {
        FragColor = vec4(baseColor, 1.0);
        return;
    }

    vec3 viewDir = normalize(viewPos - fragPos);

    vec3 normal;
    if (useVertexNormals)
    {
        normal = normalize(fragNormal);
    }
    else
    {
        vec3 dx = dFdx(fragPos);
        vec3 dy = dFdy(fragPos);
        vec3 c = cross(dx, dy);
        normal = (length(c) > 1e-6) ? normalize(c) : vec3(0.0, 1.0, 0.0);
        if (dot(normal, viewDir) < 0.0)
            normal = -normal;
    }

    // Warm indoor ambient fill
    vec3 result = 0.30 * baseColor;

    for (int i = 0; i < numLights; ++i)
    {
        vec3 lightVec = lights[i].position - fragPos;
        float dist = length(lightVec);
        vec3 lightDir = lightVec / max(dist, 0.0001);

        float attenuation = 1.0 / (1.0 + 0.045 * dist + 0.0075 * dist * dist);

        // Diffuse lighting
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * lights[i].color * baseColor;

        // Blinn-Phong specular lighting
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfwayDir), 0.0), max(shininess, 2.0));
        vec3 specular = 0.22 * spec * lights[i].color;

        result += (diffuse + specular) * lights[i].intensity * attenuation;
    }

    FragColor = vec4(clamp(result, 0.0, 1.0), 1.0);
}
