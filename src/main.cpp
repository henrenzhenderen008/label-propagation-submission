#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <string>
#include <cstring>

using namespace std;

struct Node {
    int id;
    string label;
    vector<size_t> neighbour_indices; 
};


vector<string> split_csv(const string &line) {
    vector<string> fields;
    fields.reserve(3);
    string current;
    current.reserve(64);
    bool in_quote = false;
    
    for (char c : line) {
        if (c == '"') {
            in_quote = !in_quote;
        } else if (c == ',' && !in_quote) {
            fields.push_back(std::move(current));
            current.clear();
        } else {
            current += c;
        }
    }
    fields.push_back(std::move(current));
    return fields;
}


inline string strip_quotes(const string &s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        return s.substr(1, s.size() - 2);
    }
    return s;
}


vector<int> parse_neighbours_raw(string s) {
    vector<int> res;
    res.reserve(10);
    
    if (!s.empty() && s.front() == '[') s = s.substr(1);
    if (!s.empty() && s.back() == ']') s.pop_back();
    
    size_t start = 0;
    size_t end = s.find(',');
    while (end != string::npos) {
        string part = s.substr(start, end - start);
        size_t first = part.find_first_not_of(" \t");
        size_t last = part.find_last_not_of(" \t");
        if (first != string::npos) {
            res.push_back(stoi(part.substr(first, last - first + 1)));
        }
        start = end + 1;
        end = s.find(',', start);
    }
    string part = s.substr(start);
    size_t first = part.find_first_not_of(" \t");
    size_t last = part.find_last_not_of(" \t");
    if (first != string::npos) {
        res.push_back(stoi(part.substr(first, last - first + 1)));
    }
    return res;
}

int main(int argc, char *argv[]) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input.csv>" << endl;
        return 1;
    }

    ifstream fin(argv[1], ios::binary);
    if (!fin) {
        cerr << "Error: Cannot open file " << argv[1] << endl;
        return 1;
    }

    
    string line;
    getline(fin, line); // 跳过表头
    vector<int> all_ids;
    unordered_map<int, size_t> id_to_idx;
    all_ids.reserve(100000); // 预分配（可根据数据集调整）
    id_to_idx.reserve(100000);

    while (getline(fin, line)) {
        auto fields = split_csv(line);
        if (fields.size() >= 3) {
            int id = stoi(fields[0]);
            if (id_to_idx.find(id) == id_to_idx.end()) {
                id_to_idx[id] = all_ids.size();
                all_ids.push_back(id);
            }
        }
    }

    // 重置文件指针
    fin.clear();
    fin.seekg(0);
    getline(fin, line); // 再次跳过表头

    
    vector<Node> nodes(all_ids.size());
    vector<string> raw_labels(all_ids.size());

    
    while (getline(fin, line)) {
        auto fields = split_csv(line);
        if (fields.size() < 3) continue;
        
        int id = stoi(fields[0]);
        string label = strip_quotes(fields[1]);
        string neigh_str = strip_quotes(fields[2]);
        vector<int> raw_neighbours = parse_neighbours_raw(neigh_str);

        size_t idx = id_to_idx[id];
        nodes[idx].id = id;
        raw_labels[idx] = label;
        nodes[idx].neighbour_indices.reserve(raw_neighbours.size());
        
        
        for (int neigh_id : raw_neighbours) {
            auto it = id_to_idx.find(neigh_id);
            if (it != id_to_idx.end()) {
                nodes[idx].neighbour_indices.push_back(it->second);
            }
        }
    }

    if (all_ids.empty()) {
        cerr << "Error: No nodes read from file" << endl;
        return 1;
    }

    
    bool updated;
    int iter = 0;
    vector<string> new_labels(all_ids.size());
    unordered_map<string, int> label_cnt;
    label_cnt.reserve(10);

    
    vector<string> current_labels = raw_labels;

    do {
        updated = false;

        // 计算新标签
        for (size_t i = 0; i < nodes.size(); ++i) {
            Node &node = nodes[i];
            label_cnt.clear();

            // 统计邻居标签
            for (size_t neigh_idx : node.neighbour_indices) {
                label_cnt[current_labels[neigh_idx]]++;
            }

            // 找最频繁标签
            string best_label;
            int max_cnt = -1;
            for (const auto &p : label_cnt) {
                if (p.second > max_cnt) {
                    max_cnt = p.second;
                    best_label = p.first;
                } else if (p.second == max_cnt && p.first < best_label) {
                    best_label = p.first;
                }
            }
            new_labels[i] = best_label;
        }

        // 同步更新
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (current_labels[i] != new_labels[i]) {
                current_labels[i] = new_labels[i];
                updated = true;
            }
        }
        iter++;
    } while (updated);

    // 输出结果（按原始ID升序）
    // 先对all_ids排序，确保输出顺序正确
    vector<pair<int, size_t>> id_idx_pairs;
    id_idx_pairs.reserve(all_ids.size());
    for (size_t i = 0; i < all_ids.size(); ++i) {
        id_idx_pairs.emplace_back(all_ids[i], i);
    }
    sort(id_idx_pairs.begin(), id_idx_pairs.end());

    ofstream fout("output.csv", ios::binary);
    if (!fout) {
        cerr << "Error: Cannot create output.csv" << endl;
        return 1;
    }
    fout << "Node_id,Final_label\n";
    for (auto &p : id_idx_pairs) {
        fout << p.first << "," << current_labels[p.second] << "\n";
    }
    fout.close();

    return 0;
}