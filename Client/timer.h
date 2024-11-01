#pragma once
#pragma once
#include<functional>
class Timer {
public:
	Timer() = default;
	~Timer() = default;

	void restart() {
		pass_time = 0;
		shotted = false;
	}
	//test


	void set_wait_time(float val) {
		wait_time = val;
	}
	/*void set_wait_time(int val) {
		wait_time = val;
	}*/
	//6666666666666666666666666666 int + float =???

	void set_one_shot(bool flag) {
		one_shoot = flag;
	}

	void set_on_timeout(std::function<void()> on_timeout) {
		this->on_timeout = on_timeout;
	}

	void pause() {
		paused = true;
	}

	void resume() {
		paused = false;
	}
	/*时间到达后执行回调函数*/
	void on_update(float delta) {
		if (paused)
			return;

		pass_time += delta;
		if (pass_time >= wait_time) {
			bool can_shot = (!one_shoot || (one_shoot && !shotted));
			shotted = true;
			if (can_shot && on_timeout)
				on_timeout();
			pass_time -= wait_time;
		}
	}
private:
	//int pass_time = 0;						666
	//int wait_time = 0;						666
	float pass_time = 0;						//已过时间
	float wait_time = 0;						// 等待时间
	bool paused = false;					//是否暂停
	bool shotted = false;					//是否触发
	bool one_shoot = false;					//单次触发
	std::function<void()> on_timeout;		//触发回调
};