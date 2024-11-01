
#pragma once

#include "vector2.h"
#include"timer.h"
#include<iostream>
class Camera {
public:
	enum Direction {
		Front = 0,
		Back
	};
	Camera() = default;
	~Camera() = default;

	void set_size(const Vector2& size) {
		this->size = size;
	}
	const Vector2& get_position() const {
		return position;
	}

	void on_update(int delta) {
		//std::cout << "Main camera's position : { " << position.x << " , " << position.y << " }\n";
	}


	void set_position(float x, float y) {
		position.x = x;
		position.y = y;
	}

	//True -> Front False -> Back
	void set_direction(bool Front) {
		this->position = position;
	}

	float lerp(float src, float dis, float delta) {
		return src + (dis - src) * delta;
	}

	void look_at(const Vector2& target) {
		position = target - size / 2.0f;
	}
private:
	Vector2 position;
	Vector2 size;
};