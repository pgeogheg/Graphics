#version 330

in vec3 vertex_position;
in vec3 vertex_normal;

vec4 LightPosition = vec4 (0.0, -200.0, 0.0, 1.0); // Light position in world coords.

// fixed point light properties
vec3 Ls = vec3 (1.0, 0.0, 1.0); // white specular colour
vec3 Ld = vec3 (0.7, 0.0, 0.7); // dull white diffuse light colour
vec3 La = vec3 (0.2, 0.0, 0.2); // grey ambient colour
// surface reflectance
vec3 Ks = vec3 (1.0, 1.0, 1.0); // fully reflect specular light
vec3 Kd = vec3 (1.0, 1.0, 1.0); // orange diffuse surface reflectance
vec3 Ka = vec3 (1.0, 1.0, 1.0); // fully reflect ambient light
float specular_exponent = 100.0; // specular 'power'
out vec4 fragment_colour; // final colour of surface

uniform mat4 view;
uniform mat4 proj;
uniform mat4 model;

// Input vertex data, different for all executions of this shader.
//layout(location = 0) in vec3 vertexPosition_modelspace;
//layout(location = 1) in vec2 vertexUV;

// Output data ; will be interpolated for each fragment.
//out vec2 UV;

// Values that stay constant for the whole mesh.
//uniform mat4 MVP;

void main(){

  mat4 ModelViewMatrix = view * model;
  mat3 NormalMatrix =  mat3(ModelViewMatrix);
  // Convert normal and position to eye coords
  // Normal in view space
  vec3 tnorm = normalize( NormalMatrix * vertex_normal);
  // Position in view space
  vec4 eyeCoords = ModelViewMatrix * vec4(vertex_position,1.0);
  //normalised vector towards the light source
  vec3 s = normalize(vec3(LightPosition - eyeCoords));
  
  // ambient intensity
  vec3 Ia = La * Ka;

  // The diffuse shading equation, dot product gives us the cosine of angle between the vectors
  vec3 Id = Ld * Kd * max( dot( s, tnorm ), 0.0 );

  // specular intensity
  vec3 R = reflect (-s, tnorm);
  vec3 V = normalize(vec3(-eyeCoords));
  float dot_prod_specular = max(dot (R, V), 0.0);
  float specular_factor = pow (dot_prod_specular, specular_exponent);
  vec3 Is = Ls * Ks * specular_factor; // final specular intensity

  fragment_colour = vec4 (Is + Id + Ia, 1.0);
  
  // Convert position to clip coordinates and pass along
  // gl_Position = MVP * (proj * view * model * vec4(vertex_position,1.0));
  gl_Position = (proj * view * model * vec4(vertex_position,1.0));
  //UV = vertexUV;
}


  