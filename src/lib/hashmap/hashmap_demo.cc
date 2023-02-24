#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <queue>
#include <sys/resource.h>
#include <thread>

#include "src/app/mapblock_data.h"
#include "src/lib/hashmap/hashmap.h"

size_t kThreads = 2;
static constexpr auto kProgressInterval = std::chrono::milliseconds(1000);

using Matrix = HashMap<MapBlockPos, MapBlockData, MapBlockPosHashFunc>;

static Matrix m;
struct rusage base_usage;

static constexpr int64_t kTombstone = std::numeric_limits<int64_t>::max();
std::queue<int64_t> input_queue;
std::mutex input_mutex;
std::condition_variable input_cv;

void LoadInputData(const char *filename) {
  std::unique_lock<std::mutex> lock(input_mutex);
  std::ifstream ifs(filename);
  if (ifs.is_open()) {
    std::string line;
    while (std::getline(ifs, line)) {
      if (line.empty() || (line.at(0) == '#')) {
        continue;
      }

      input_queue.push(strtol(line.c_str(), nullptr, 10));
    }
  }

  std::cout << "Input count: " << input_queue.size() << "\n";
  input_queue.push(kTombstone);
}

template <class Rep, class Period>
bool IdleWait(const std::chrono::duration<Rep, Period> &timeout) {
  std::unique_lock<std::mutex> lock(input_mutex);
  return !input_cv.wait_for(lock, timeout, []() {
    return !input_queue.empty() && (input_queue.front() == kTombstone);
  });
}

void PrintStats(const Matrix &m) {
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  const size_t rss = usage.ru_maxrss - base_usage.ru_maxrss;

  const size_t rss_mb = rss / 1024;
  const size_t items = m.size();
  const double item_per_mb =
      static_cast<double>(items) / (static_cast<double>(rss) / 1024.0);
  const double bytes_per_item =
      (static_cast<double>(rss) * 1024.0) / static_cast<double>(items);

  std::cout << m.size() << " " << rss_mb << " " << item_per_mb << " "
            << bytes_per_item << "\n";
}

void WorkerThread() {
  while (true) {
    std::unique_lock<std::mutex> lock(input_mutex);
    int64_t id = input_queue.front();
    if (id == kTombstone) {
      break;
    }
    input_queue.pop();
    input_cv.notify_one();
    lock.unlock();

    MapBlockData &item = m.Ref(MapBlockPos(static_cast<int64_t>(id)));
    item.preserve = true;
    item.uniform = id;
  }
}

int main(int argc, char *argv[]) {
  std::cerr << "sizeof(MapBlockPos) = " << sizeof(MapBlockPos) << "\n";
  std::cerr << "sizeof(MapBlockData) = " << sizeof(MapBlockData) << "\n";
  std::cerr << "sizeof(pair<>) = "
            << sizeof(std::pair<MapBlockPos, MapBlockData>) << "\n";

  LoadInputData("raw-pos-list.txt");
  getrusage(RUSAGE_SELF, &base_usage);

  std::vector<std::thread> threads;
  for (size_t i = 0; i < kThreads; ++i) {
    threads.emplace_back(std::thread(&WorkerThread));
  }

  while (IdleWait(kProgressInterval)) {
    PrintStats(m);
    std::this_thread::sleep_for(kProgressInterval);
  }

  for (auto &t : threads) {
    t.join();
  }

  PrintStats(m);
  std::cout << "\n";

#if 0
  // Print dump of per-bucket load-factors.
  for (auto foo: m.GetStats()) {
    std::cout << foo.size << " " << foo.load_factor << "\n";
  }
#endif

  return 0;
}
