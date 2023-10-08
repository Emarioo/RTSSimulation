#pragma once

static const char* vs_CubeShaderGLSL = R"(
#shader vertex
#version 330 core

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNormal;

layout(location = 2) in vec3 iPos;
layout(location = 3) in vec3 iSize;
layout(location = 4) in vec3 iColor;

out vec3 fColor;
out vec3 fPos;
flat out vec3 fNormal;

uniform mat4 uProj;

void main() {
	//fPos = vPos;
	fPos = vPos * iSize + iPos; // vPos * iSize should be component-wise
	gl_Position = uProj * vec4(fPos,1);

    fColor = iColor;
	//fColor = vec3(1,1,1);
	
	fNormal = fPos-vec3(iPos + iSize * vec3(vPos-vNormal));
	//fNormal = vNormal;

	// fNormal = fPos-vec3(iMat * vec4(vPos-vNormal, 1));
}

#shader fragment
#version 330 core
layout(location = 0) out vec4 oColor;

in vec3 fColor;
flat in vec3 fNormal;
in vec3 fPos;

uniform vec3 uLightPos;

void main() {
	//vec3 ambientColor = vec3(0.1,0.5,0.33);
	vec3 ambientColor = vec3(0.1,0.1,0.1);
	vec3 lightColor = vec3(1.0, 1.0, 1.0);
	float ambientStrength = 0.3;
    
    vec3 sunLightColor = vec3(1.0,1.0,1.0);
    vec3 sunLightDir = normalize(vec3(0.5,-1.0,0.5));

	vec3 ambient = ambientStrength * ambientColor;
    
    vec3 result = vec3(0,0,0);
    {
        result += ambient;
    }
    {
        vec3 norm = normalize(fNormal);
        float diff = max(dot(norm, -sunLightDir), 0.0);
        vec3 diffuse = diff * sunLightColor;
        result += diffuse;
    }
    /*{
        vec3 norm = normalize(fNormal);
        vec3 lightDir = normalize(uLightPos - fPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;
        result += diffuse;
    }*/
	result = result * fColor;

	oColor = vec4(result, 1.0);
	//oColor = vec4(lightDir, 1.0);
	//oColor = vec4(uLightPos, 1.0);
    //oColor = vec4(1,1,1,1);
	//oColor = fColor;
}
)";