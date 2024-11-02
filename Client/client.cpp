#include"../thirdLib/httplib.h"

#include"path.h"
#include"player.h"

#include<chrono>
#include<string>
#include<vector>
#include<thread>
#include<codecvt>
#include<fstream>
#include<sstream>

enum class Stage {
	Waiting,		//等待玩家加入
	Ready,			//准备起跑倒计时
	Racing			//正在比赛中
};

int val_countdown = 4;
Stage stage = Stage::Waiting;

int id_player = 0;
std::atomic<int> progress_1 = -1;
std::atomic<int> progress_2 = -1;
int num_total_char = 0;

Path path = Path({
	{842,842},{1322,842},{1322,442},
	{2762,442},{2762,842},{3162,842},
	{3162,1722},{2122,1722},{2122,1562},
	{842,1562},{842,842}
	});

int idx_line = 0;
int idx_char = 0;
int loop = 0;
Timer timer_waiting;
std::string str_text;						//文本内容
std::vector<std::string> str_line_list;		//行文本列表

Atlas atlas_1P_idle_up;
Atlas atlas_1P_idle_down;
Atlas atlas_1P_idle_left;
Atlas atlas_1P_idle_right;
Atlas atlas_1P_run_up;
Atlas atlas_1P_run_down;
Atlas atlas_1P_run_left;
Atlas atlas_1P_run_right;
Atlas atlas_2P_idle_up;
Atlas atlas_2P_idle_down;
Atlas atlas_2P_idle_left;
Atlas atlas_2P_idle_right;
Atlas atlas_2P_run_up;
Atlas atlas_2P_run_down;
Atlas atlas_2P_run_left;
Atlas atlas_2P_run_right;

IMAGE img_ui_1;
IMAGE img_ui_2;
IMAGE img_ui_3;
IMAGE img_ui_fight;
IMAGE img_ui_textbox;
IMAGE img_background;

std::string str_address;			//服务器地址
httplib::Client* client = nullptr;	//HTTP客户端对象

void load_resources(HWND hwnd) {
	AddFontResourceEx(_T("resources/IPix.ttf"), FR_PRIVATE, NULL);

	atlas_1P_idle_up.load(_T("resources/hajimi_idle_back_%d.png"), 4);
	atlas_1P_idle_down.load(_T("resources/hajimi_idle_front_%d.png"), 4);
	atlas_1P_idle_left.load(_T("resources/hajimi_idle_left_%d.png"), 4);
	atlas_1P_idle_right.load(_T("resources/hajimi_idle_right_%d.png"), 4);
	atlas_1P_run_up.load(_T("resources/hajimi_run_back_%d.png"), 4);
	atlas_1P_run_down.load(_T("resources/hajimi_run_front_%d.png"), 4);
	atlas_1P_run_left.load(_T("resources/hajimi_run_left_%d.png"), 4);
	atlas_1P_run_right.load(_T("resources/hajimi_run_right_%d.png"), 4);
	atlas_2P_idle_up.load(_T("resources/manbo_idle_back_%d.png"), 4);
	atlas_2P_idle_down.load(_T("resources/manbo_idle_front_%d.png"), 4);
	atlas_2P_idle_left.load(_T("resources/manbo_idle_left_%d.png"), 4);
	atlas_2P_idle_right.load(_T("resources/manbo_idle_right_%d.png"), 4);
	atlas_2P_run_up.load(_T("resources/manbo_run_back_%d.png"), 4);
	atlas_2P_run_down.load(_T("resources/manbo_run_front_%d.png"), 4);
	atlas_2P_run_left.load(_T("resources/manbo_run_left_%d.png"), 4);
	atlas_2P_run_right.load(_T("resources/manbo_run_right_%d.png"), 4);

	loadimage(&img_ui_1, _T("resources/ui_1.png"));
	loadimage(&img_ui_2, _T("resources/ui_2.png"));
	loadimage(&img_ui_3, _T("resources/ui_3.png"));
	loadimage(&img_ui_fight, _T("resources/ui_fight.png"));
	loadimage(&img_ui_textbox, _T("resources/ui_textbox.png"));
	loadimage(&img_background, _T("resources/background.png"));

	load_audio(_T("resources/bgm.mp3"), _T("bgm"));
	load_audio(_T("resources/1p_win.mp3"), _T("1p_win"));
	load_audio(_T("resources/2p_win.mp3"), _T("2p_win"));
	load_audio(_T("resources/click_1.mp3"), _T("click_1"));
	load_audio(_T("resources/click_2.mp3"), _T("click_2"));
	load_audio(_T("resources/click_3.mp3"), _T("click_3"));
	load_audio(_T("resources/click_4.mp3"), _T("click_4"));
	load_audio(_T("resources/ui_1.mp3"), _T("ui_2"));
	load_audio(_T("resources/ui_2.mp3"), _T("ui_2"));
	load_audio(_T("resources/ui_3.mp3"), _T("ui_3"));
	load_audio(_T("resources/ui_fight.mp3"), _T("ui_fight"));

	std::ifstream file("config.cfg");
	if (!file.good()) {
		MessageBox(hwnd, L"无法打开配置文件 config.cfg", L"启动失败", MB_OK | MB_ICONERROR);
		exit(-1);
	}

	std::stringstream str_stream;
	str_stream << file.rdbuf();
	str_address = str_stream.str();

	file.close();
}

void login_to_server(HWND hwnd) {
	client = new httplib::Client(str_address);
	client->set_keep_alive(true);

	httplib::Result result = client->Post("/login");

	if (!result || result->status != 200) {
		MessageBox(hwnd, _T("无法连接到服务器!"), _T("启动失败"), MB_OK | MB_ICONERROR);
		exit(-1);
	}

	id_player = std::stoi(result->body);


	if (id_player <= 0) {
		MessageBox(hwnd, _T("比赛已经开始辣!"), _T("拒绝加入"), MB_OK | MB_ICONERROR);
		exit(-1);
	}
	
	(id_player == 1) ? (progress_1 = 0) : (progress_2 = 0);
	str_text = client->Post("/query_text")->body;

	std::stringstream str_stream(str_text);
	std::string str_line;
	while (std::getline(str_stream, str_line)) {
		str_line_list.push_back(str_line);
		num_total_char += (int)str_line.length();
	}

	//数据同步线程
	std::thread([&]() {
		while (true) {
			using namespace std::chrono;

			std::string route = (id_player == 1) ? "/update_1" : "/update_2";
			std::string body = std::to_string((id_player == 1) ? progress_1 : progress_2);
			httplib::Result  result = client->Post(route, body, "text/plain");

			if (result && result->status == 200) {
				int progress = std::stoi(result->body);
				(id_player == 1) ? (progress_2 = progress) : (progress_1 = progress);
			}		
			std::this_thread::sleep_for(nanoseconds(1000000000 / 10)); //等待0.1秒后执行下一次同步循环
		}
		}).detach();
}
int main(int argc, char** argv) {
	
	// init
	using namespace std::chrono;

	HWND hwnd = initgraph(1280, 720);
	SetWindowText(hwnd, _T("哈基米大冒险!"));
	settextstyle(28, 0, _T("IPix"));
	setbkmode(TRANSPARENT);

	load_resources(hwnd);
	login_to_server(hwnd);

	ExMessage msg;
	Timer timer_countdown;
	Camera camera_ui, camera_scene;
	Player player_1(&atlas_1P_idle_up, &atlas_1P_idle_down, &atlas_1P_idle_left, &atlas_1P_idle_right,
		 &atlas_1P_run_up, &atlas_1P_run_down, &atlas_1P_run_left, &atlas_1P_run_right);
	Player player_2(&atlas_2P_idle_up, &atlas_2P_idle_down, &atlas_2P_idle_left, &atlas_2P_idle_right,
		&atlas_2P_run_up, &atlas_2P_run_down, &atlas_2P_run_left, &atlas_2P_run_right);

	camera_ui.set_size({ 1280,720 });
	camera_scene.set_size({ 1280,720 });

	player_1.set_position({ 842,842 });
	player_2.set_position({ 842,842 });

	timer_waiting.set_on_timeout([&]() {
		++loop;
		});
	timer_waiting.set_one_shot(false);
	timer_waiting.set_wait_time(0.5f);
	timer_countdown.set_one_shot(false);
	timer_countdown.set_wait_time(1.0f);
	timer_countdown.set_on_timeout([&]() {
		val_countdown--;

		switch (val_countdown)
		{
		case 3:play_audio(_T("ui_3")); break;
		case 2:play_audio(_T("ui_2")); break;
		case 1:play_audio(_T("ui_1")); break;
		case 0:play_audio(_T("ui_0")); break;
		case -1:
			stage = Stage::Racing;
			play_audio(_T("bgm"), true);
			break;
		}
		});

	const nanoseconds frame_duration(1000000000 / 144);
	steady_clock::time_point last_tick = steady_clock::now();
	BeginBatchDraw();
	while (true) {
		// handle intput
		while (peekmessage(&msg)) {
			if (stage != Stage::Racing)
				continue;
			if (msg.message == WM_CHAR && idx_line < str_line_list.size()) {
				const std::string& str_line = str_line_list[idx_line];
				if (str_line[idx_char] == msg.ch) {//输入正确就播放对应音效
					switch (rand()%4)
					{
					case 0:play_audio(_T("click_1")); break;
					case 1:play_audio(_T("click_2")); break;
					case 2:play_audio(_T("click_3")); break;
					case 3:play_audio(_T("click_4")); break;
					}

					(id_player == 1) ? progress_1++ : progress_2++;
					idx_char++;
					if (idx_char >= str_line.length()) {
						idx_char = 0;
						idx_line++;
					}
				}
			}
		 }
		// handle update
		steady_clock::time_point frame_start = steady_clock::now();
		duration<float> delta = duration<float>(frame_start - last_tick);

		if (stage == Stage::Waiting) {
			timer_waiting.on_update(delta.count());
			if (progress_1 >= 0 && progress_2 >= 0)
				stage = Stage::Ready;
		}
		else {
			if (stage == Stage::Ready)
				timer_countdown.on_update(delta.count());

			//胜负检测
			if ((id_player == 1 && progress_1 >= num_total_char)
				||
				(id_player == 2 && progress_2 >= num_total_char)) {
				stop_audio(_T("bgm"));
				play_audio((id_player == 1) ? _T("1p_win") : _T("2p_win"));
				MessageBox(hwnd, _T("赢麻辣!"), _T("游戏结束"), MB_OK | MB_ICONINFORMATION);
				exit(0);
			}
			else if((id_player == 1 && progress_2 >= num_total_char)
				||
				(id_player == 2 && progress_1 >= num_total_char)) {
				stop_audio(_T("bgm"));
				MessageBox(hwnd, _T("战败惹~"), _T("游戏结束"), MB_OK | MB_ICONINFORMATION);
				exit(0);
			}

			player_1.set_target(path.get_position_at_progress((float)progress_1 / num_total_char));
			player_2.set_target(path.get_position_at_progress((float)progress_2 / num_total_char));

			player_1.on_update(delta.count());
			player_2.on_update(delta.count());

			camera_scene.look_at((id_player == 1)
				? player_1.get_position() : player_2.get_position());
		}
		// handle draw

		setbkcolor(RGB(0, 0, 0));
		cleardevice();

		if (stage == Stage::Waiting) {
			settextcolor(RGB(195, 195, 195));
			switch (loop)
			{
			case 0:outtextxy(15, 675, _T("比赛即将开始，等待其他玩家加入")); break;
			case 1:outtextxy(15, 675, _T("比赛即将开始，等待其他玩家加入.")); break;
			case 2:outtextxy(15, 675, _T("比赛即将开始，等待其他玩家加入..")); break;
			case 3: {
				outtextxy(15, 675, _T("比赛即将开始，等待其他玩家加入...")); 
				break;
			}
			case 4: {
				outtextxy(15, 675, _T("比赛即将开始，等待其他玩家加入")); 
				loop = 0; 
				break; 
			}
			default:
				break;
			}
		}
		else {
			static const Rect rect_bg = {
				0,0,
				img_background.getwidth(),
				img_background.getheight()
			};
			putimage_ex(camera_scene, &img_background, &rect_bg);

			if (player_1.get_position().y > player_2.get_position().y) {
				player_2.on_render(camera_scene);
				player_1.on_render(camera_scene);
			}
			else {
				player_1.on_render(camera_scene);
				player_2.on_render(camera_scene);
			}

			switch (val_countdown)
			{
			case 3: {
				static const Rect rect_ui_3 = {
					1280 / 2 - img_ui_3.getwidth() / 2,
					720 / 2 - img_ui_3.getheight() / 2,
					img_ui_3.getwidth(),img_ui_3.getheight()
				};
				putimage_ex(camera_ui, &img_ui_3, &rect_ui_3);
			}
				  break;
			case 2: {
				static const Rect rect_ui_2 = {
					1280 / 2 - img_ui_3.getwidth() / 2,
					720 / 2 - img_ui_3.getheight() / 2,
					img_ui_2.getwidth(),img_ui_2.getheight()
				};
				putimage_ex(camera_ui, &img_ui_2, &rect_ui_2);
			}
				  break;
			case 1: {
				static const Rect rect_ui_1 = {
					1280 / 2 - img_ui_1.getwidth() / 2,
					720 / 2 - img_ui_1.getheight() / 2,
					img_ui_1.getwidth(),img_ui_1.getheight()
				};
				putimage_ex(camera_ui, &img_ui_1, &rect_ui_1);
			}
				  break;
			case 0: {
				static const Rect rect_ui_fight = {
					1280 / 2 - img_ui_fight.getwidth() / 2,
					720 / 2 - img_ui_fight.getheight() / 2,
					img_ui_fight.getwidth(),img_ui_fight.getheight()
				};
				putimage_ex(camera_ui, &img_ui_fight, &rect_ui_fight);
			}
				  break;
			default:
				break;
			}

			if (stage == Stage::Racing) {
				static const Rect rect_textbox = {
					0,
					720 - img_ui_textbox.getheight(),
					img_ui_textbox.getwidth(),
					img_ui_textbox.getheight()
				};
				static std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convert;
				std::wstring wstr_line = convert.from_bytes(str_line_list[idx_line]);
				std::wstring wstr_completed = convert.from_bytes(str_line_list[idx_line].substr(0, idx_char));
				putimage_ex(camera_ui, &img_ui_textbox, &rect_textbox);
				settextcolor(RGB(125, 125, 125));
				outtextxy(185 + 2, rect_textbox.y + 65 + 2, wstr_line.c_str());
				settextcolor(RGB(25, 25, 25));
				outtextxy(185 , rect_textbox.y + 65 , wstr_line.c_str());
				settextcolor(RGB(0, 149, 217));
				outtextxy(185 , rect_textbox.y + 65, wstr_completed.c_str());
			}
		}
		FlushBatchDraw();
		last_tick = frame_start;
		nanoseconds sleep_duration = frame_duration - (steady_clock::now() - frame_start);
		if (sleep_duration > nanoseconds(0))
			std::this_thread::sleep_for(sleep_duration);
	}
	return 0;
}   
