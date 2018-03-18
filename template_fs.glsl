#version 430
layout(location = 1) uniform int passV;
layout(location = 2) uniform sampler2D diffuse_color;
layout(location = 3) uniform sampler2D fish_color;
layout(location = 5) uniform int isBoundary;
layout(location = 7) uniform int index_fish;

layout(location = 0) out vec4 fragcolor;   
layout(location = 1) out vec4 picktex;
       
in vec2 tex_coord;
flat in int InstanceID;
in vec3 normals;
      
void main(void)
{   

	vec3 viewVector = vec3(0.0, 0.0, 1.0);

	if(passV == 1)
	{
		if(InstanceID == index_fish)
		{
			vec3 tempColor = vec3(0.0f, 1.0f, 2.0f)* (1.0-dot(normals,viewVector));
			float fresnel = dot(normals,viewVector);

			fragcolor = vec4(tempColor, 0.5f);
			//fragcolor = vec4(normals.z);
		}
		else
		{
			fragcolor = texture(fish_color, tex_coord);
		}
		
		//fragcolor = vec4(1.0f, 1.0f, 0.0f, 1.0f);
		picktex = vec4(InstanceID/255.0f, InstanceID/255.0f, 0.0f, 1.0f);
	}

	else if(passV == 2)
	{
		vec4 left = texelFetch(diffuse_color,ivec2(gl_FragCoord) + ivec2(-1 ,0.0),0);
		vec4 right = texelFetch(diffuse_color,ivec2(gl_FragCoord) + ivec2(1 ,0.0),0);
		vec4 above = texelFetch(diffuse_color,ivec2(gl_FragCoord) + ivec2(0.0,1),0);
		vec4 below = texelFetch(diffuse_color,ivec2(gl_FragCoord) + ivec2(0.0,-1),0);

		vec4 c = texelFetch(diffuse_color,ivec2(gl_FragCoord) ,0);

		vec4 col = (left-right)*(left-right) + (above-below)*(above-below);
	
		if(isBoundary == 1 && gl_FragCoord.x <= 1279 && gl_FragCoord.y <= 719
						   && gl_FragCoord.x >=1 && gl_FragCoord.y >=1)
		{
			fragcolor = col;
		}
		else
		{
			fragcolor = texture(diffuse_color, tex_coord);
		}
	
	}
	
}




















