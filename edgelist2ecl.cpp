#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <climits>
#include <cstring>
#include <vector>
#include <algorithm>
#include <string>
#include <type_traits>
#include <inttypes.h>

#include "ECLgraph.h"

static bool is_comment_or_blank(const char* s) {
  // skip leading spaces/tabs
  while (*s == ' ' || *s == '\t') s++;
  if (*s == '#') return true;
  if (*s == '\0' || *s == '\n' || *s == '\r') return true;
  return false;
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr, "USAGE: %s input.edgelist output.egr [--undirected]\n", argv[0]);
    return 1;
  }

  const char* in_path  = argv[1];
  const char* out_path = argv[2];
  const bool make_undirected = (argc >= 4 && std::string(argv[3]) == "--undirected");

  FILE* f = std::fopen(in_path, "rt");
  if (!f) {
    std::perror("fopen(input)");
    return 2;
  }

  std::vector<std::pair<int, int>> edges;
  edges.reserve(1 << 20);

  long long u_ll = 0, v_ll = 0;
  int min_id = INT_MAX;
  int max_id = -1;

  char line[1024];
  while (std::fgets(line, sizeof(line), f)) {
    // check if the line data is valid
    if (is_comment_or_blank(line)) continue;

    // Read 2 non-negative values
    if (std::sscanf(line, "%lld %lld", &u_ll, &v_ll) != 2) continue;

    if (u_ll < 0LL || v_ll < 0LL) {
      std::fprintf(stderr, "ERROR: negative node id detected in line: %s", line);
      std::fclose(f);
      return 3;
    }
    if (u_ll > INT_MAX || v_ll > INT_MAX) {
      std::fprintf(stderr, "ERROR: node id exceeds INT_MAX in line: %s", line);
      std::fclose(f);
      return 4;
    }

    // remove self-loops
    if (u_ll == v_ll) continue;

    int u = (int)u_ll;
    int v = (int)v_ll;

    min_id = std::min(min_id, std::min(u, v));
    max_id = std::max(max_id, std::max(u, v));

    edges.emplace_back(u, v);
    if (make_undirected) edges.emplace_back(v, u);
  }
  std::fclose(f);

  if (edges.empty()) {
    std::fprintf(stderr, "ERROR: no edges read from %s\n", in_path);
    return 5;
  }

  // Heuristic: if min id == 1, shift to 0-based
  if (min_id == 1) {
    for (auto& e : edges) {
      e.first -= 1;
      e.second -= 1;
    }
    max_id -= 1;
    min_id = 0;
  }

  if (min_id < 0) {
    std::fprintf(stderr, "ERROR: negative node id after shifting (min=%d)\n", min_id);
    return 6;
  }

  // sort + unique
  std::sort(edges.begin(), edges.end());
  edges.erase(std::unique(edges.begin(), edges.end()), edges.end());

  const int nodes = max_id + 1;

  // edges count in 64-bit (no INT_MAX limit)
  uint64_t e64 = (uint64_t)edges.size();

  std::fprintf(stdout, "Input:  %s\n", in_path);
  std::fprintf(stdout, "Output: %s\n", out_path);
  std::fprintf(stdout, "Nodes:  %d\n", nodes);
  std::fprintf(stdout, "Edges:  %" PRIu64 "\n", e64);
  if (make_undirected) std::fprintf(stdout, "Mode:   undirected (symmetrized)\n");
  else std::fprintf(stdout, "Mode:   as-is\n");

  ECLgraph g;
  g.nodes = nodes;

  // Use the real type of g.edges from ECLgraph.h (could be int/long/long long)
  using EdgesT = decltype(g.edges);
  g.edges = (EdgesT)e64;

  g.eweight = nullptr;

  // Auto-match the pointer element types from ECLgraph.h (int* vs long* etc.)
  using IndexT = std::remove_pointer_t<decltype(g.nindex)>;
  using AdjT   = std::remove_pointer_t<decltype(g.nlist)>;

  // Allocate CSR arrays
  g.nindex = (decltype(g.nindex))std::calloc((size_t)nodes + 1, sizeof(IndexT));
  g.nlist  = (decltype(g.nlist)) std::malloc((size_t)e64 * sizeof(AdjT));

  if (!g.nindex || !g.nlist) {
    std::fprintf(stderr, "ERROR: memory allocation failed (nodes=%d edges=%" PRIu64 ")\n", nodes, e64);
    if (g.nindex) std::free(g.nindex);
    if (g.nlist)  std::free(g.nlist);
    return 8;
  }

  // Degree count
  for (const auto& pr : edges) {
    const int src = pr.first;
    if (src < 0 || src >= nodes) {
      std::fprintf(stderr, "ERROR: src out of range: %d\n", src);
      freeECLgraph(g);
      return 9;
    }
    g.nindex[src + 1] += 1;
  }

  // Prefix sum
  for (int i = 1; i <= nodes; i++) {
    g.nindex[i] += g.nindex[i - 1];
  }

  // Optional sanity: nindex[nodes] should equal edges count (in IndexT)
  // (May overflow if IndexT is smaller than edges count)
  // std::fprintf(stdout, "CSR total = %lld\n", (long long)g.nindex[nodes]);

  // Fill adjacency (cursor per node)
  std::vector<IndexT> cur((size_t)nodes);
  for (int i = 0; i < nodes; i++) cur[i] = g.nindex[i];

  for (const auto& pr : edges) {
    const int src = pr.first;
    const int dst = pr.second;
    if (dst < 0 || dst >= nodes) {
      std::fprintf(stderr, "ERROR: dst out of range: %d\n", dst);
      freeECLgraph(g);
      return 10;
    }
    IndexT pos = cur[src]++;
    g.nlist[pos] = (AdjT)dst;
  }

  // Write .egr
  writeECLgraph(g, out_path);
  freeECLgraph(g);

  std::fprintf(stdout, "Done.\n");
  return 0;
}
