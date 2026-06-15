#include<iostream>
#include<thread>
#include<mutex>
#include<condition_variable>
#include<queue>
#include<functional>
#include<vector>
#include<string>
#include <chrono>
class ThreadPool {
	public:
	ThreadPool(int numThreads): stop(false) {
		for (int i = 0; i < numThreads; ++i) {
			threads.emplace_back([this] {
				while (true) {
					std::function<void()> task;
					{
						std::unique_lock<std::mutex> lock(mtx);
						condition.wait(lock, [this] { return stop || !tasks.empty(); });
						if (stop && tasks.empty()) return;
						task = std::move(tasks.front());
						tasks.pop();
					}
					task();
				}
			});
		}
	}
	template<class F, class... Args>
	void enqueue(F&& f, Args&&... args) {
		{
			std::unique_lock<std::mutex> lock(mtx);
			tasks.emplace(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
		}
		condition.notify_one();
	}
	~ThreadPool() {
		{
			std::unique_lock<std::mutex> lock(mtx);
			stop = true;
		}
		condition.notify_all();
		for (std::thread &worker : threads) {
			worker.join();
		}
	}
private:
	std::vector<std::thread> threads;
	std::queue<std::function<void()>> tasks;
	std::mutex mtx;
	std::condition_variable condition;
	bool stop = false;
};

int main() {
	ThreadPool pool(4);
	for (int i = 0; i < 10; ++i) {
		pool.enqueue([i] {
			std::cout << "Task " << i << " is running " << std::this_thread::get_id() << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			std::cout << "Task " << i << " is done " << std::this_thread::get_id() << std::endl;
			
		});
	}
	return 0;
}