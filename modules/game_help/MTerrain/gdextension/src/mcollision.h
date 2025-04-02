#ifndef MCOLLISION
#define MCOLLISION

#include "core/math/vector3.h"
#include "core/object/ref_counted.h"

class MCollision : public RefCounted {
	GDCLASS(MCollision, RefCounted);

protected:
	static void _bind_methods();

public:
	bool collided = false;
	Vector3 collision_position;

	bool is_collided();
	Vector3 get_collision_position();
};

#endif