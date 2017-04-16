#ifndef BENCHMARK_LOCKFREE_HT
#define BENCHMARK_LOCKFREE_HT

#include <unordered_map>
#include <iostream>
#include <random>
#include <algorithm>
#include <pthread.h>

#include "../common/cycle_timer.h"
#include "../lockfree_hash_table.h"
#include "thread_service.h"

#define NUM_ITERS 3
#define MAX_THREADS 24

class BenchmarkLockFreeHT
{
  public:
    BenchmarkLockFreeHT(int op_count, int capacity, 
                        int rweight, int idweight,
                        int thread_count,
                        double load_factor);

    void benchmark_all();
    void run();

  private:
    int    m_rweight;
    int    m_idweight;

    int    m_thread_count;
    int    m_op_count;
    int    m_capacity;
    double m_load_factor;
};

BenchmarkLockFreeHT::BenchmarkLockFreeHT(int op_count, int capacity, 
                                         int rweight, int idweight,
                                         int thread_count, double load_factor)
{
  std::cout << "*** BENCHMARKING LockFreeHT ***" << std::endl;
  m_op_count     = op_count;
  m_load_factor  = load_factor; 
  m_capacity     = capacity;
  m_thread_count = thread_count;

  m_rweight      = rweight;
  m_idweight     = idweight;
}

void BenchmarkLockFreeHT::benchmark_all()
{
    Lockfree_hash_table ht(m_capacity);

    std::random_device                 rd;
    std::mt19937                       mt(rd());
    std::uniform_int_distribution<int> rng;

    std::array<int, 3> weights;
    weights[0] = m_rweight;
    weights[1] = m_idweight;
    weights[2] = m_idweight;

    std::default_random_engine         g;
    std::discrete_distribution<int>    drng(weights.begin(), weights.end());

    // Warm-up table to load factor
    int num_warmup = static_cast<int>(static_cast<double>(m_capacity) * m_load_factor);
    for (int i = 0; i < num_warmup; i++)
    {
      int k = rng(mt); 
      int v = rng(mt);

      ht.insert(k, v);
    }

    // Run benchmark
    std::vector<double> results;
    for (int iter = 0; iter < NUM_ITERS; iter++)
    {
      int num_elems = m_op_count / m_thread_count;
      pthread_t  workers[MAX_THREADS];
      WorkerArgs args[MAX_THREADS];

      double start = CycleTimer::currentSeconds();
      for (int i = 0; i < m_thread_count; i++)
      {
        args[i].num_elems = num_elems;
        args[i].rweight   = m_rweight;
        args[i].iweight   = m_idweight;
        args[i].dweight   = m_idweight;
        args[i].ht_p      = (void*)&ht;
        pthread_create(&workers[i], NULL, thread_service<Lockfree_hash_table>, (void*)&args[i]);
      }

      for (int i = 0; i < m_thread_count; i++)
      {
        pthread_join(workers[i], NULL);
      }
      double time  = CycleTimer::currentSeconds() - start;
      results.push_back(time);
    }

    // Publish Results
    double best_time = *std::min_element(results.begin(), results.end());
    double avg_time  = std::accumulate(results.begin(), results.end(), 0.0) / static_cast<double>(results.size());
    std::cout << "\t" << "Max Throughput: " << m_op_count / best_time * 1000.0 << " ops/ms" << std::endl;
    std::cout << "\t" << "Avg Throughput: " << m_op_count / avg_time  * 1000.0 << " ops/ms" << std::endl;
}

void BenchmarkLockFreeHT::run()
{
  benchmark_all();
}

#endif
