shader_type spatial;
render_mode unshaded,cull_disabled;

const vec3 inner_color=vec3(0.0,0.0,0.9);
const vec3 outer_color=vec3(0.9,0.0,0.0);

varying vec3 pos_local;
varying vec3 normal_local;
varying vec3 color;

void vertex() {
	normal_local = abs(NORMAL);
	if(dot(VERTEX,NORMAL) < 0.0){ // inner BOX
		color = inner_color;
		pos_local = VERTEX + TIME/8.0;
	} else { // outer BOX
		color = outer_color;
		pos_local = VERTEX + TIME/6.0;
	}
}

void fragment() {
	vec3 val = mod(pos_local,vec3(1.0));
	float f = val.y * (normal_local.x + normal_local.z);
	f += val.z * normal_local.y;
	f = smoothstep(0.5,0.501,f);
	ALPHA = mix(0.0,0.3,f);
	ALBEDO = color * f;
}
