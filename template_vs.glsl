#version 430            
layout(location = 0) uniform mat4 PV;
layout(location = 1) uniform int passV;
layout(location = 4) uniform float time;
layout(location = 6) uniform mat4 M;
layout(location = 7) uniform int index_fish;


layout(location = 0) in vec3 pos_attrib;
layout(location = 1) in vec2 tex_coord_attrib;
layout(location = 2) in vec3 normal_attrib;
layout(location = 3) in mat4 offset;


out vec2 tex_coord;  
out vec3 normals;

flat out int InstanceID;

void main(void)
{
	if(passV ==1)
	{
		if(gl_InstanceID == index_fish)
		{
			//float posOffset = pos_attrib.x * sin(5*time);
			gl_Position = PV*offset*M*vec4(vec3(pos_attrib.x, pos_attrib.y, pos_attrib.z + 0.06 * sin(20*pos_attrib.x + 6*time)) , 1.0);
		}
		else
		{
			
			gl_Position = PV*offset*M*vec4(vec3(pos_attrib.x, pos_attrib.y, pos_attrib.z + 0.02 * sin(10*pos_attrib.x + 3*time)) , 1.0);
		}
		
	    tex_coord = tex_coord_attrib;
		InstanceID = gl_InstanceID;
		normals = normal_attrib;

	}
	else if(passV ==2)
	{
		gl_Position = vec4(pos_attrib, 1.0);
		tex_coord = 0.5* pos_attrib.xy + vec2(0.5);
	}
}