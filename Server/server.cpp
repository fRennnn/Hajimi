#include"../thirdLib/httplib.h"

#include<mutex>
#include<string>
#include<fstream>

std::string str_text;		//ȫ�ֻ�����
std::mutex g_mutex;		//�ı�����

int progress_1 = -1;	//���1����
int progress_2 = -1;	//���2����
int main(int argc, char** argv) {
	std::ifstream file("text.txt");
	if (!file.good()) {
		MessageBox(nullptr, L"�޷����ı��ļ� text.txt", L"����ʧ��", MB_OK | MB_ICONERROR);
		return -1;
	}

	std::stringstream str_stream;
	str_stream << file.rdbuf();
	str_text = str_stream.str();

	file.close(); 
	httplib::Server server;

	server.Post("/login", [&](const httplib::Request& req, httplib::Response& res) {
		std::lock_guard<std::mutex> lock(g_mutex);
		if (progress_1 >= 0 && progress_2 >= 0) {
			res.set_content("-1", "text/plain");
			return;
		}
		res.set_content(progress_1 >= 0 ? "2" : "1", "text/plain");
		(progress_1 >= 0) ? (progress_2 = 0) : (progress_1 = 0);
		});

	server.Post("/query_text", [&](const httplib::Request& req, httplib::Response& res) {	
		res.set_content(str_text, "text/plain");	 
		});
	    
	//�����1�����2����Ϸ����ת�����Է���
	server.Post("/update_1", [&](const httplib::Request& req, httplib::Response& res) {
		
		std::lock_guard<std::mutex> lock(g_mutex);
		progress_1 = std::stoi(req.body);
		res.set_content(std::to_string(progress_2), "text/plain");

		});
	server.Post("/update_2", [&](const httplib::Request& req, httplib::Response& res) {
		
		std::lock_guard<std::mutex> lock(g_mutex);
		progress_2 = std::stoi(req.body);
		res.set_content(std::to_string(progress_1), "text/plain");

		});
	server.listen("0.0.0.0", 25565);
	return 0;
}